/*
*	Printservice
*
* Copyright 2012-2013  Samsung Electronics Co., Ltd

* Licensed under the Flora License, Version 1.1 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

* http://floralicense.org/license/

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#include <stdio.h>
#include <ctype.h>
#include <alloca.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pt_debug.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>

#include <pt_db.h>

#define TEMPLATE "/tmp/mobileprint/printXXXXXX"
#define FILE_NAME "printXXXXXX"
#define FILE_BASE_DIR "mobileprint/"
#define MASK_MODE 022
#define BUFF_SIZE 512
#define STR_PAR "Invalid parameters: "
#define STR_PTR "Invalid pointers: "

enum {
	PIPE_READ = 0,
	PIPE_WRITE = 1
};

typedef enum {
	PT_MODELNAME = 0,
	PT_PRODUCTNAME
} pt_search_key;

typedef enum {
	PT_FILE_TMP = 0
} pt_file_type;

typedef enum {
	PT_FREE_FILE_CLOSE = 0, /** Close file descriptor and free memory\
								allocated for structure */
	PT_FREE_FILE_DEL 		/** The same as PT_FREE_FILE_CLOSE, plus delete file\
								from file-system */
} pt_free_file_type;

typedef struct {
	uint32_t *buffer;
	uint32_t len;
} info;

typedef struct {
	uint32_t *buffer;
	uint32_t len;
} prod;

typedef struct {
	uint32_t name;
	prod product;
	info inform;
} model;

typedef struct {
	char *value;
	char *modified;
} outstr;

typedef struct {
	pt_file_type file_type;
	char *file_dir;
	char *file_name;
} pt_file_info;

typedef struct {
	FILE *ptfile;
	const char *ptpath;
} pt_file;

/* Create file */
static pt_file *pt_create_file(pt_file_info *file_info);

/* Set information of file */
static pt_file_info *pt_create_file_info(char *name, char *dir, pt_file_type type);

/* Synchronize a file's in-core state with storage device. */
static int pt_sync_file(pt_file *file);

/* Execute ppdc command with predefined set of parameters */
static char *pt_exec_ppdc(pt_file *file);

/* Read and parse drv file */
/* Return parsed structure */
static drvm *read_drv(FILE *FD);

static pt_dbconfig *pt_tune_db(FILE *file, const char *path);

/* Extract specific drv file from database by modelname */
static pt_file *extract_drv_by_model(drvm drv, const char *modelname);

/* Extract specific drv file from database by product */
static pt_file *extract_drv_by_product(drvm drv, const char *product);

/* Extract specific drv file from database by specified key */
static pt_file *extract_drv_by_key(drvm drv, const char *key, pt_search_key type);

/* Read and parse drv file */
/* Return model structure */
static model *read_model(FILE* FD);

/* Extract value of search key accordingly    */
/* specification of drv file */
static char *filter(char *string, pt_search_key key);

/* Return value of a string, deleting quotes */
static outstr *filter_string(const char *original);

/* Extraction value of Product */
/* accordingly specification of drv file */
static char *filter_product(char *original, pt_search_key key);

/* Read all data from file/pipe */
static char *reliable_read(int fd);

/* Get size of file in bytes */
static int get_file_size(const char *path);

static void pt_free_dbconfig(pt_dbconfig *dbconfig);

static void pt_free_config(pt_dbconfig *config);

/* Free memory allocated by drvm structure */
static void pt_free_drvm(drvm *drv);

/* Clean resources linked with file */
static int pt_free_file(pt_file *file, pt_free_file_type free_type);

/* Free memory allocated by pt_file_info structure */
static void pt_free_file_info(pt_file_info *file_info);

/* Free memory allocated by prod structure */
static void pt_free_prod(prod *product);

/* Free memory allocated by string */
static void pt_free_strings(gpointer data);

/* Free memory allocated by model structure */
static void pt_free_model(gpointer data);

/* Free memory allocated by info structure */
static void pt_free_info(info *inf);

/* Clean memory of allocated outstr structure */
static void clean_outstr(outstr *ostr);

int get_file_size(const char *path)
{
	struct stat statbuff;
	if(stat(path, &statbuff)) {
		return -1;
	}
	return statbuff.st_size;
}

void pt_free_file_info(pt_file_info *file_info)
{
	if(file_info) {
		if(file_info->file_dir) {
			free(file_info->file_dir);
		}

		if(file_info->file_name) {
			free(file_info->file_name);
		}

		free(file_info);
	}
}

pt_file_info *pt_create_file_info(char *name, char *dir, pt_file_type type)
{
	pt_file_info *info = (pt_file_info*)malloc(sizeof(pt_file_info));
	if(!info) {
		return NULL;
	}
	memset(info, '\0', sizeof(pt_file_info));
	char *tmp_name = NULL;
	if(name) {
		tmp_name = strdup(name);
		if(!tmp_name) {
			fprintf(stderr, "String duplicating fails\n");
			pt_free_file_info(info);
			return NULL;
		}
		info->file_name = tmp_name;
	} else {
		info->file_name = NULL;
	}

	char *tmp_dir = NULL;
	if(dir) {
		tmp_dir = strdup(dir);
		if(!tmp_dir) {
			fprintf(stderr, "String duplicating fails\n");
			pt_free_file_info(info);
			free(tmp_name);
			return NULL;
		}
		info->file_dir = tmp_dir;
	} else {
		info->file_dir = NULL;
	}

	info->file_type = type;
	return info;
}

int pt_sync_file(pt_file *file)
{
	if(!file) {
		return -1;
	}

	if(!file->ptfile) {
		return -1;
	}

	int res = fflush(file->ptfile);
	return res;
}

int pt_free_file(pt_file *file, pt_free_file_type free_type)
{
	if(!file) {
		return -1;
	}
	if(!file->ptpath) {
		return -1;
	}
	if(!file->ptfile) {
		return -1;
	}

	if(free_type == PT_FREE_FILE_CLOSE) {
		if(fclose(file->ptfile)) {
			return -1;
		}
		free((void *)file->ptpath);
		free(file);
	} else if(free_type == PT_FREE_FILE_DEL) {
		if(fclose(file->ptfile)) {
			return -1;
		}
		if(unlink(file->ptpath)) {
			return -1;
		}
		free((void *)file->ptpath);
		free(file);
	}
	return 0;
}

pt_file *pt_create_file(pt_file_info *file_info)
{
	FILE *fd = NULL;
	char *file = NULL;

	if(!file_info) {
		return NULL;
	}

	if(file_info->file_type == PT_FILE_TMP) {
		char *string = strdup(TEMPLATE);
		if(!string) {
			perror("Can't duplicate string with strdup()");
			return NULL;
		}

		char *drname = dirname(string);
		if(g_mkdir_with_parents(drname, S_IRWXU | S_IRWXG) == -1) {
			perror("Can't create nested directory path");
			PT_IF_FREE_MEM(string);
			return NULL;
		}
		PT_IF_FREE_MEM(string);

		file = strdup(TEMPLATE);
		if (!file) {
			fprintf(stderr, "Can't duplicate string with strdup()\n");
			return NULL;
		}

		umask(MASK_MODE);
		int filedesc = mkstemp(file);
		if (filedesc == -1) {
			perror("Can't create temp file");
			PT_IF_FREE_MEM(file);
			return NULL;
		}

		fd = fdopen(filedesc, "w");
		if(!fd) {
			fprintf(stderr, "Call fdopen() failed\n");
			if(close(filedesc)) {
				fprintf(stderr, "Closing file descriptor fails\n");
			}
			int rst = unlink(file);
			if(rst) {
				fprintf(stderr, "Unlink temporary file fails\n");
			}
			PT_IF_FREE_MEM(file);
			return NULL;
		}

	} else {
		fprintf(stderr, "pt_create_file: Not yet fully supported\n");
		return NULL;
	}

	pt_file *ptfile = (pt_file *)malloc(sizeof(pt_file));
	if(!ptfile) {
		fprintf(stderr, "Allocating memory fails\n");
		if(fclose(fd)) {
			PT_ERROR("File error: %s", strerror(errno));
		}
		PT_IF_FREE_MEM(file);
		return NULL;
	}
	memset(ptfile, '\0', sizeof(pt_file));

	ptfile->ptfile = fd;
	ptfile->ptpath = file;
	return ptfile;
}

char *reliable_read(int fd)
{
	char *new_buffer;
	char *buffer = malloc(BUFF_SIZE);
	if (!buffer) {
		fprintf(stderr, "call malloc() failed: %s\n", strerror(errno));
		return NULL;
	}
	memset(buffer, '\0', BUFF_SIZE);

	new_buffer = buffer;
	ssize_t rst = 0;
	ssize_t global = 0;
	size_t count = BUFF_SIZE;
	size_t new_size = count;
	while((rst = read(fd, buffer, count)) != -1) {
		if(rst > 0) {
			global += rst;
			count -= rst;
			continue;
		} else if(rst == 0) {
			if (global == new_size) {
				// we didn't read all data
				new_size = BUFF_SIZE + BUFF_SIZE;
				if (!(new_buffer = malloc(new_size))) {
					free(buffer);
					return NULL;
				}
				memset(new_buffer, '\0', new_size);
				strncpy(new_buffer, buffer, BUFF_SIZE);
				free(buffer);
				buffer = new_buffer;
			}
			break;
		}
	}
	return buffer;
}

char *pt_exec_ppdc(pt_file *file)
{
	int pipefd[2];
	char *buffer = NULL;

	if(!file) {
		return NULL;
	}

	if (pipe(pipefd))
	{
		PT_ERROR("pipe fail");
		return NULL;
	}

	pid_t pid;
	if((pid = fork()) < 0) {
		PT_ERROR("Forking process fail");
		return NULL;
	} else if (pid == 0) {

			if (dup2(pipefd[PIPE_READ], STDIN_FILENO) == -1) {
				return NULL;
			}

			if (dup2(pipefd[PIPE_WRITE], STDOUT_FILENO) == -1) {
				return NULL;
			}

			if (dup2(pipefd[PIPE_WRITE], STDERR_FILENO) == -1) {
				return NULL;
			}

			if(close(pipefd[PIPE_READ])) {
				return NULL;
			}

			// First we remove all locale variables
			if(unsetenv("LANG") || unsetenv("LANGUAGE") || unsetenv("LC_ALL")) {
				PT_ERROR("Unsetting locale variables fails");
				return NULL;
			}

			if(!setenv("LC_ALL", "POSIX", true)) {
				PT_INFO("Successfully adjust environment for ppdc");
			} else {
				PT_ERROR("Adjust environment for ppdc fails");
				return NULL;
			}

			if (execl("/usr/bin/ppdc", "ppdc", "-v",\
				"-d", ".", file->ptpath, (const char*) NULL)) {

				if (execl("/bin/ppdc", "ppdc", "-v",\
					"-d", ".", file->ptpath, (const char*) NULL)) {

					PT_ERROR("Executing process fail");
					return NULL;
				}
			}
			PT_ERROR("Executing process fail");
			return NULL;
	}

	close(pipefd[PIPE_WRITE]);

	int status;
	pid_t child = wait(&status);
	if (child != pid) {
		return NULL;
	}

	if(WIFEXITED(status)) {
		if (WEXITSTATUS(status) != 0) {
			return NULL;
		} else {
			buffer = reliable_read(pipefd[PIPE_READ]);
		}
	} else {
		return NULL;
	}

	return buffer;
}

pt_file *extract_drv_by_key(drvm drv, const char *key, pt_search_key type)
{
	pt_file *file = NULL;
	GPtrArray *strings = drv.strings;
	GPtrArray *models = drv.models;

	guint len = models->len;

	char *name;
	model *mod;
	for (int i = 0; i < len; ++i) {
		bool correct = false;
		mod = g_ptr_array_index(models, i);
		if (type == PT_MODELNAME) {
			name = g_ptr_array_index(strings, mod->name);
			name = filter(name, PT_MODELNAME);
			if(name) {
				if(!strcasecmp(key, name)) {
					free(name);
					correct = true;
				} else {
					free(name);
					continue;
				}
			} else {
				continue;
			}
		} else if (type == PT_PRODUCTNAME) {
			uint32_t value;
			uint32_t prod_len  = (mod->product).len;
			int *counter = (mod->product).buffer;
			for (int k = 0; k < prod_len; ++k) {
				value = *counter++;
				name = g_ptr_array_index(strings, value);
				name = filter_product(name, PT_PRODUCTNAME);
				if (!name) {
					continue;
				}

				if (strcasecmp(key, name)) {
					free(name);
					continue;
				}
				free(name);
				correct = true;
				break;
			}
		}

		if (!correct) {
			continue;
		}

		pt_file_info *file_info = pt_create_file_info(NULL, NULL, PT_FILE_TMP);
		if(!file_info) {
			fprintf(stderr, "Setting file mode fails\n");
			return NULL;
		}

		file = pt_create_file(file_info);
		if(!file) {
			pt_free_file_info(file_info);
			fprintf(stderr, "Creating file fails\n");
			return NULL;
		}

		char *numstr;
		int line = (mod->inform).len;
		for (int i = 0; i < line; ++i) {
			uint32_t num = (mod->inform).buffer[i];
			numstr = g_ptr_array_index(strings, num);
			if(fputs(numstr, file->ptfile) == EOF) {
				fprintf(stderr, "Writing data to temp file failed\n");
				// FIXME: Here we should free memory
				pt_free_file_info(file_info);
				if(pt_free_file(file, PT_FREE_FILE_DEL)) {
					fprintf(stderr, "Deleting temporary file fails\n");
				}
				return NULL;
			}
		}

		pt_free_file_info(file_info);
		if(pt_sync_file(file)) {
			fprintf(stderr, "Synchronize file with storage device fails\n");
			if(pt_free_file(file, PT_FREE_FILE_DEL)) {
				fprintf(stderr, "Deleting temporary file fails\n");
			}
			return NULL;
		}
		break;
	}
	return file;
}

pt_file *extract_drv_by_model(drvm drv, const char *modelname)
{
	pt_file *file;
	file = extract_drv_by_key(drv, modelname, PT_MODELNAME);
	return file;
}

pt_file *extract_drv_by_product(drvm drv, const char *product)
{
	pt_file *file;
	file = extract_drv_by_key(drv, product, PT_PRODUCTNAME);
	return file;
}

void pt_free_drvm(drvm *drv)
{
	if(drv)
	{
		if(drv->models) {
			GPtrArray *models = drv->models;

			g_ptr_array_set_free_func(models, pt_free_model);
			g_ptr_array_free(models, TRUE);
		}

		if(drv->strings) {
			GPtrArray *strings = drv->strings;

			g_ptr_array_set_free_func(strings, pt_free_strings);
			g_ptr_array_free(strings, TRUE);
		}
		free((void*)drv);
	}
}

void clean_outstr(outstr *ostr)
{
	if(ostr) {
		if(ostr->value) {
			free(ostr->value);
		}
	}
}

outstr *filter_string(const char *original)
{
	char *value = NULL;
	char *begin = NULL, *end = NULL, *word = NULL;

	if(!original) {
		PT_ERROR("Argument is NULL");
		return NULL;
	}

	outstr *string = (outstr*)malloc(sizeof(outstr));
	if(!string) {
		fprintf(stderr, "Allocating memory fails\n");
		return NULL;
	}
	memset(string, '\0', sizeof(outstr));

	ptrdiff_t strln = 0;
	begin = (char *)original;

	unsigned char count = 0;

	while(isblank(*begin)) {
		begin++;
	}

	word = begin;

	while(*begin != '\0') {
		if (*begin == '"') {
			count++;
			begin++;
			break;
		} else if ((isblank(*begin)) && (count == 0)) {
			end = begin;
			begin = word;
			strln = end - begin;
			value = malloc(strln+1);
			if (!value) {
				clean_outstr(string);
				free(string);
				fprintf(stderr, "Can't allocate memory\n");
				return NULL;
			}
			memset(value, '\0', strln+1);
			strncpy(value, begin, strln);
			string->value = value;
			string->modified = end;
			return string;
		} else {
			begin++;
		}
	}

	if (count == 0) {
		end = begin;
		begin = word;
		value = malloc(strln+1);
		if (!value) {
			clean_outstr(string);
			free(string);
			fprintf(stderr, "Can't allocate memory\n");
			return NULL;
		}
		memset(value, '\0', strln+1);
		strncpy(value, begin, strln);
		string->value = value;
		string->modified = end;
		return string;
	} else {
		end = begin;
	}

	while(*end != '\0') {
		if (*end == '"') {
			count--;
			break;
		}
		end++;
	}

	strln = end - begin;
	value = malloc(strln+1);
	if (!value) {
		clean_outstr(string);
		free(string);
		fprintf(stderr, "Can't allocate memory\n");
		return NULL;
	}
	memset(value, '\0', strln+1);
	strncpy(value, begin, strln);
	string->value = value;
	end++;
	string->modified = end;
	return string;
}

char *filter_product(char *original, pt_search_key key)
{
	const char *pname = "Product";
	const char *attr = "Attribute";
	outstr *result = NULL;

	if (!original) {
		fprintf(stderr, "original is null\n");
		return NULL;
	}
	char *copied = malloc(strlen(original)+1);
	if (!copied) {
		fprintf(stderr, "Can't allocate memory\n");
		return NULL;
	}

	char *full_string = copied;
	char *modified = NULL;
	char *value = NULL;

	strcpy(copied, original);
	result = filter_string(copied);
	if(!result) {
		PT_ERROR("Error in processing string");
		free(full_string);
		return NULL;
	}
	modified = result->modified;
	value = result->value;
	free(result);

	if (!strncmp(value, attr, strlen(attr))) {
		result = filter_string(modified);
		if(!result) {
			PT_ERROR("Error in processing string");
			free(value);
			free(full_string);
			return NULL;
		}
		free(value);
		modified = result->modified;
		value = result->value;
		free(result);

		if(!strncmp(value, pname, strlen(pname))) {
			result = filter_string(modified);
			if(!result) {
				PT_ERROR("Error in processing string");
				free(value);
				free(full_string);
				return NULL;
			}
			free(value);
			modified = result->modified;
			value = result->value;
			free(result);

			result = filter_string(modified);
			free(value);
			value = result->value;
			free(result);
		} else {
			free(value);
			free(full_string);
			return NULL;
		}
	} else {
		free(value);
		free(full_string);
		return NULL;
	}

	free(full_string);
	return value;
}

char *filter(char *original, pt_search_key key)
{
	static const char *mname = "ModelName";
	static const char *delim = " \t";
	static const char *delim_str = "\"";
	static const char *end = "\n";

	char *value = NULL;
	char *token = NULL;
	char *copied = malloc(strlen(original)+1);
	if (!copied) {
		fprintf(stderr, "Can't allocate memory\n");
		return NULL;
	}
	char *full_string = copied;

	strcpy(copied, original);
	while(isblank(*copied)) {
		copied++;
		continue;
	}

	token = strsep(&copied, delim);
	if (!strcmp(token, mname)) {
		token = strsep(&copied, delim_str);
		if(!strcmp(token, "")) {
			token = strsep(&copied, delim_str);
			value = malloc(strlen(token)+1);
			if (!value) {
				free(full_string);
				fprintf(stderr, "Can't allocate memory\n");
				return NULL;
			}
			strcpy(value, token);
			token = strsep(&copied, end);
			while(*token != '\0') {
				if(isblank(*token)) {
					token++;
					continue;
				} else {
					free(full_string);
					free(value);
					printf("String contains unexpected symbols:\n");
					printf("%s\n", original);
					return NULL;
				}
			}
		} else {
			free(full_string);
			printf("String which contains %s is not correct", original);
			return NULL;
		}
	}
	free(full_string);
	return value;
}

void pt_free_prod(prod *product)
{
	if(product) {
		if(product->buffer) {
			free(product->buffer);
		}
	}
}

void pt_free_strings(gpointer data)
{
	if(data) {
		free((char*)data);
	}
}

void pt_free_model(gpointer data)
{
	if(data)
	{
		model *mod = (model *)data;
		pt_free_prod(&(mod->product));
		pt_free_info(&(mod->inform));
		free(mod);
	}
}

drvm *read_drv(FILE *fd)
{
	size_t result;
	drvm *drv;
	GPtrArray *models;
	GPtrArray *strings;

	drv = (drvm *)calloc(1, sizeof(drvm));
	if (!drv) {
		fprintf(stderr, "Can't allocate memory\n");
		return NULL;
	}

	uint32_t len;

	result = fread(&len, sizeof(uint32_t), 1, fd);
	if (result != 1) {
		fprintf(stderr, "Can't read data from file\n");
		pt_free_drvm(drv);
		return NULL;
	}

	if(len == 0 || len > UINT32_MAX) {
		fprintf(stderr, "Invalid length(%d)\n", len);
		pt_free_drvm(drv);
		return NULL;
	}

	size_t elm_size = sizeof(model*);
	uint32_t length = len;
	size_t res_size = length * elm_size;

	models = g_ptr_array_sized_new(length);
	drv->models = models;

	model *mod;
	for (int i = 0; i < length; ++i) {
		mod = read_model(fd);
		g_ptr_array_add(models, mod);
	}

	g_ptr_array_set_free_func(models, pt_free_model);

	result = fread(&len, sizeof(uint32_t), 1, fd);
	if (result != 1) {
		fprintf(stderr, "Can't read data from file\n");
		void *ptr = g_ptr_array_free(models, TRUE);
		if(ptr) {
			perror("freeing memory fails");
		}
		free(drv);
		return NULL;
	}

	if (len == 0 || len > UINT32_MAX) {
		fprintf(stderr, "Invalid length(%d)\n", len);
		pt_free_drvm(drv);
		return NULL;
	}

	elm_size = sizeof(char *);
	length = len;
	res_size = length * elm_size;
	strings = g_ptr_array_sized_new(length);
	g_ptr_array_set_free_func(strings, pt_free_strings);
	drv->strings = strings;

	char *string;
	size_t size_buffer;
	ssize_t read;
	for (int i = 0; i < length; ++i) {
		string = NULL;
		if((read = getline(&string, &size_buffer, fd)) == -1) {
			fprintf(stderr, "Can't read line from file\n");
			void *ptr = g_ptr_array_free(strings, TRUE);
			if(ptr) {
				perror("freeing memory fails");
			}
			ptr = g_ptr_array_free(models, TRUE);
			if(ptr) {
				perror("freeing memory fails");
			}
			free(drv);
			return NULL;
		}
		g_ptr_array_add(strings, string);
	}

	return drv;
}

model *read_model(FILE *fd)
{
	model *mod;
	size_t result;
	uint32_t length;

	mod = malloc(sizeof(model));
	uint32_t len;
	if (!mod) {
		fprintf(stderr, "Can't allocate memory\n");
		return NULL;
	}

	result = fread(&len, sizeof(uint32_t), 1, fd);
	if(result != 1) {
		fprintf(stderr, "Can't read data from file\n");
		pt_free_model(mod);
		return NULL;
	}

	length = len;
	mod->name = length;

	result = fread(&len, sizeof(uint32_t), 1, fd);
	if(result != 1) {
		fprintf(stderr, "Can't read data from file\n");
		pt_free_model(mod);
		return NULL;
	}

	if(len == 0 || len > UINT32_MAX) {
		fprintf(stderr, "Invalid length(%d)\n", len);
		pt_free_model(mod);
		return NULL;
	}

	length = len;
	if ((length >> 30) > 0)
	{
		fprintf(stderr, "Integer overflow\n");
		pt_free_model(mod);
		return NULL;
	}

	uint32_t size_buffer = length * sizeof(uint32_t);

	if(!((mod->product).buffer = malloc(size_buffer))) {
		fprintf(stderr, "Can't allocate memory\n");
		pt_free_model(mod);
		return NULL;
	}
	(mod->product).len = length;

	result = fread((mod->product).buffer, size_buffer, 1, fd);
	if(result != 1) {
		fprintf(stderr, "Can't read data from file\n");
		pt_free_model(mod);
		return NULL;
	}

	// Parse part related to product

	result = fread(&len, sizeof(uint32_t), 1, fd);
	if(result != 1) {
		fprintf(stderr, "Can't read data from file\n");
		pt_free_model(mod);
		return NULL;
	}

	if(len == 0 || len > UINT32_MAX) {
		fprintf(stderr, "Invalid length(%d)\n", len);
		pt_free_model(mod);
		return NULL;
	}

	length = len;
	if ((length>>(32 - sizeof(uint32_t))) > 0)
	{
		fprintf(stderr, "Integer overflow\n");
		pt_free_model(mod);
		return NULL;
	}

	size_buffer = length * sizeof(uint32_t);
	if(!(mod->inform.buffer = malloc(size_buffer))) {
		fprintf(stderr, "Can't allocate memory\n");
		pt_free_model(mod);
		return NULL;
	}
	mod->inform.len = length;
	result = fread(mod->inform.buffer, size_buffer, 1, fd);
	if(result != 1) {
		fprintf(stderr, "Can't read data from file\n");
		pt_free_model(mod);
		return NULL;
	}

	return mod;
}

void pt_free_info(info *inf)
{
	if(inf) {
		if(inf->buffer) {
			free(inf->buffer);
		}
	}
}

pt_dbconfig *pt_tune_db(FILE *file, const char *path)
{
	if(!file) {
		PT_ERROR(STR_PAR "%p", file);
		return NULL;
	}

	if(!path) {
		PT_ERROR(STR_PAR "%p", path);
		return NULL;
	}

	char *copied_path = strdup(path);
	if(!copied_path) {
		PT_ERROR("Allocating memory fail");
		return NULL;
	}

	int size = get_file_size(copied_path);
	if(size == -1) {
		PT_ERROR("Getting info about file fail");
		free(copied_path);
		return NULL;
	}

	char *buffer = malloc(size);
	if (!buffer) {
		PT_ERROR("Allocating memory fail");
		free(copied_path);
		return NULL;
	}

	int result = setvbuf(file, buffer, _IOFBF,  size);
	if (result) {
		PT_ERROR("Tuning buffering operations fail");
		free(copied_path);
		free(buffer);
		return NULL;
	}

	pt_dbconfig *config;
	config = (pt_dbconfig*)malloc(sizeof(pt_dbconfig));
	if(!config) {
		PT_ERROR("Allocating memory fail");
		free(copied_path);
		free(buffer);
		return NULL;
	}
	memset(config, '\0', sizeof(pt_dbconfig));
	config->dbfile = file;
	config->dbpath = copied_path;
	config->dbbuffer = buffer;
	return config;
}

void pt_free_config(pt_dbconfig *config)
{
	if(!config) {
		PT_ERROR(STR_PTR "%p", config);
	} else {
		if(config->dbbuffer) {
			free((void *)config->dbbuffer);
		}
		if(config->dbpath) {
			free((void *)config->dbpath);
		}
		free(config);
	}
}

char* pt_extract_ppd(pt_db *database, const char *search, pt_search type)
{
	if(!database || !search) {
		PT_ERROR(STR_PAR "%p, %p", database, search);
		return NULL;
	}

	drvm* dbstruct = database->dbstruct;

	if(!dbstruct) {
		PT_ERROR(STR_PTR "%p", dbstruct);
		return NULL;
	}

	pt_file *file = NULL;
	if(type == PT_SEARCH_MODEL) {
		file = extract_drv_by_model(*dbstruct, search);
	} else if(type == PT_SEARCH_PROD) {
		file = extract_drv_by_product(*dbstruct, search);
	} else if(type == PT_SEARCH_ALL) {
		file = extract_drv_by_model(*dbstruct, search);
		if(!file) {
			file = extract_drv_by_product(*dbstruct, search);
		}
	} else {
		PT_ERROR(STR_PAR "%d", type);
		return NULL;
	}

	if(!file) {
		PT_ERROR("Didn't find \"%s\" in database", search);
		return NULL;
	}

	char *output = pt_exec_ppdc(file);
	if(!output) {
		// Handle error
		PT_ERROR("Executing ppdc fails\n");
		if(pt_free_file(file, PT_FREE_FILE_DEL)) {
			fprintf(stderr, "Freeing memory fails\n");
		}
		return NULL;
	}

	if(pt_free_file(file, PT_FREE_FILE_DEL)) {
		fprintf(stderr, "Freeing memory fails\n");
		free(output);
		return NULL;
	}
	return output;
}

void pt_free_dbconfig(pt_dbconfig *dbconfig)
{
	if(dbconfig) {
		if(dbconfig->dbbuffer) {
			free((void*)dbconfig->dbbuffer);
		}
		if(dbconfig->dbpath) {
			free((void*)dbconfig->dbpath);
		}
		if(dbconfig->dbfile) {
			fclose(dbconfig->dbfile);
		}
	}
}

void pt_free_db(pt_db *database)
{
	if(database) {
		if(database->dbconfig) {
			pt_free_dbconfig(database->dbconfig);
		}
		if(database->dbstruct) {
			pt_free_drvm(database->dbstruct);
		}
	}
}

pt_db *pt_create_db(const char *path)
{
	if(!path) {
		return NULL;
	}

	FILE *dbfile = fopen(path, "rb");
	if (!dbfile) {
		return NULL;
	}

	pt_dbconfig *config;
	config = pt_tune_db(dbfile, path);
	if(!config) {
		PT_ERROR("Tuning database fail");
		fclose(dbfile);
		return NULL;
	}

	drvm *dbstruct = read_drv(dbfile);
	if(!dbstruct) {
		pt_free_config(config);
		fclose(dbfile);
		return NULL;
	}

	pt_db *database = (pt_db*)malloc(sizeof(pt_db));
	if(!database) {
		pt_free_config(config);
		pt_free_drvm(dbstruct);
		fclose(dbfile);
		return NULL;
	}

	database->dbstruct = dbstruct;
	database->dbconfig = config;
	return database;
}