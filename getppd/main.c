/*
*	Printservice
*
* Copyright 2012  Samsung Electronics Co., Ltd

* Licensed under the Flora License, Version 1.0 (the "License");
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
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include "main.h"

#define TEMPLATE "mobileprint/printXXXXXX"
#define MASK_MODE 022
#define BUFF_SIZE 512

enum {
	PIPE_READ = 0,
	PIPE_WRITE = 1
};

static char **gargv;
static resource globalres;

__attribute__((weak)) void init_signals()
{
	return;
}

void clean_all_resources(void) {
	clean_drv(globalres.drv);
	clean_buffer(globalres.buffer);
	clean_tmp_file(globalres.tempfile, PT_SOFT_CLEAN);
}

int get_file_size(const char *path) {
	struct stat statbuff;
	if(stat(path, &statbuff)) {
		printf("[Error] Can't call stat() function\n");
		exit(EXIT_FAILURE);
	}
	return statbuff.st_size;
}

FILE *create_tmp_file(char **out_fname) {
	// Create directory anyway
	if(!out_fname) {
		return NULL;
	}

	char *string = strdup(TEMPLATE);
	if(!string) {
		perror("Can't duplicate string with strdup()");
	}

	char *drname = dirname(string);
	if(g_mkdir_with_parents(drname, S_IRWXU | S_IRWXG)) {
		perror("Can't create nested directory path");
		exit(EXIT_FAILURE);
	}
	free(string);

	char *file = strdup(TEMPLATE);
	if (!file) {
		fprintf(stderr, "Can't duplicate string with strdup()\n");
		exit(EXIT_FAILURE);
	}

	umask(MASK_MODE);
	int filedesc = mkstemp(file);
	if (filedesc == -1) {
		perror("Can't create temp file");
		exit(EXIT_FAILURE);
	}
	globalres.tempfile = file;

	FILE *fd = fdopen(filedesc, "w");
	if(!fd) {
		fprintf(stderr, "Call fdopen() failed\n");
		exit(EXIT_FAILURE);
	}

	*out_fname = file;
	return fd;
}

char *reliable_read(int fd) {
	char *new_buffer;
	char *buffer = malloc(BUFF_SIZE);
	if (!buffer) {
		fprintf(stderr, "call malloc() failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	bzero(buffer, BUFF_SIZE);

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
				bzero(new_buffer, new_size);
				strncpy(new_buffer, buffer, BUFF_SIZE);
				free(buffer);
				buffer = new_buffer;
			}
			break;
		}
	}
	return buffer;
}

char *exec_ppdc(const char *file) {
	int pipefd[2];
	char *buffer = NULL;

	if(!file) {
		return NULL;
	}

	if (pipe(pipefd))
	{
		perror("pipe failed");
		exit(EXIT_FAILURE);
	}

	pid_t pid;
	if((pid = fork()) < 0) {
		fprintf(stderr, "Call fork() failed\n");
		exit(EXIT_FAILURE);
	} else if (pid == 0) {

			if (dup2(pipefd[PIPE_READ], STDIN_FILENO) == -1) {
				exit(EXIT_FAILURE);
			}

			if (dup2(pipefd[PIPE_WRITE], STDOUT_FILENO) == -1) {
				exit(EXIT_FAILURE);
			}

			if (dup2(pipefd[PIPE_WRITE], STDERR_FILENO) == -1) {
				exit(EXIT_FAILURE);
			}

			if(close(pipefd[PIPE_READ])) {
				exit(EXIT_FAILURE);
			}

			if (execl("/usr/bin/ppdc", "ppdc", "-v",\
				"-d", ".", file, (const char*) NULL)) {

				if (execl("/bin/ppdc", "ppdc", "-v",\
					"-d", ".", file, (const char*) NULL)) {

					perror("exec failed");
					exit(EXIT_FAILURE);
				}
			}
			fprintf(stderr, "[2]Call exec failed\n");
			exit(EXIT_FAILURE);
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

	if(!clean_tmp_file(file, PT_FORCE_CLEAN)) {
		perror("Deleting temporary file error");
		exit(EXIT_FAILURE);
	}

	return buffer;
}

char *extract_drv_by_key(drvm drv, const char *key, pt_search_key type) {
	char *fname = NULL;
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

		FILE *fd = create_tmp_file(&fname);
		if(!fd) {
			fprintf(stderr, "Creating temporary file failed\n");
			exit(EXIT_FAILURE);
		}

		char *numstr;
		int line = (mod->inform).len;
		for (int i = 0; i < line; ++i) {
			uint32_t num = (mod->inform).buffer[i];
			numstr = g_ptr_array_index(strings, num);
			if(fputs(numstr, fd) == EOF) {
				fprintf(stderr, "Writing data to temp file failed\n");
				exit(EXIT_FAILURE);
			}
		}
		fclose(fd);
		break;
	}
	return fname;
}

char *extract_drv_by_model(drvm drv, const char *modelname) {
	char *fname = extract_drv_by_key(drv, modelname, PT_MODELNAME);
	return fname;
}

char *extract_drv_by_product(drvm drv, const char *product) {
	char *fname = extract_drv_by_key(drv, product, PT_PRODUCTNAME);
	return fname;
}

bool clean_tmp_file(const char *file, pt_clean_file type) {
	bool deleted = false;

	if(file) {
		int rst;
		if(!(rst = unlink(file))) {
			deleted = true;
		} else {
			int error = errno;
			if((error == ENOENT) &&
				(type == PT_FORCE_CLEAN)) {
				deleted = false;
			} else if ((error == ENOENT) &&
						(type == PT_SOFT_CLEAN)) {
				deleted = true;
			} else {
				perror("clean_tmp_file failed()");
				deleted = false;
			}
		}
	}
	return deleted;
}

void clean_buffer(char *buffer) {
	if(buffer) {
		free(buffer);
	}
}

bool clean_drv(drvm *drv) {
	bool free_all = false;
	bool free_model = false;
	bool free_strings = false;

	if(drv)
	{
		if(drv->models) {
			GPtrArray *models = drv->models;

			g_ptr_array_set_free_func(models, clean_model);
			void *ptr = g_ptr_array_free(models, TRUE);
			if(ptr) {
				perror("freeing memory");
				exit(EXIT_FAILURE);
			}
		}

		if(drv->strings) {
			GPtrArray *strings = drv->strings;

			g_ptr_array_set_free_func(strings, clean_strings);
			void *ptr = g_ptr_array_free(strings, TRUE);
			if(ptr) {
				perror("freeing memory");
				exit(EXIT_FAILURE);
			}
		}
	}

//	if(free_model && free_strings) {
//		free_all = true;
//	}
	return free_all;
}

void clean_outstr(outstr *ostr) {
	if(ostr) {
		if(ostr->value) {
			free(ostr->value);
		}
	}
}

outstr filter_string(const char *original) {
	static const char *delim_str = "\"";
	char *value = NULL;
	char *token = NULL, *product = NULL;
	char *begin = NULL, *end = NULL, *word = NULL;

	outstr string;
	string.value = NULL;
	string.modified = NULL;

	char *copied = malloc(strlen(original)+1);
	char *full_string = copied;
	if (!copied) {
		fprintf(stderr, "Can't allocate memory\n");
		exit(EXIT_FAILURE);
	}
	ptrdiff_t strln = 0;
	strcpy(copied, original);
	begin = copied;

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
				free(full_string);
				fprintf(stderr, "Can't allocate memory\n");
				exit(EXIT_FAILURE);
			}
			bzero(value, strln+1);
			strncpy(value, begin, strln);
			string.value = value;
			string.modified = end;
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
			free(full_string);
			fprintf(stderr, "Can't allocate memory\n");
			exit(EXIT_FAILURE);
		}
		bzero(value, strln+1);
		strncpy(value, begin, strln);
		string.value = value;
		string.modified = end;
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
		free(full_string);
		fprintf(stderr, "Can't allocate memory\n");
		exit(EXIT_FAILURE);
	}
	bzero(value, strln+1);
	strncpy(value, begin, strln);
	string.value = value;
	end++;
	string.modified = end;
	return string;
}

char *filter_product(char *original, pt_search_key key) {
	static const char *pname = "Product";
	static const char *attr = "Attribute";
	outstr parse_str;
	char *value = NULL;

	if (!original) {
		fprintf(stderr, "original is null\n");
		exit(EXIT_FAILURE);
	}
	char *copied = malloc(strlen(original)+1);
	if (!copied) {
		fprintf(stderr, "Can't allocate memory\n");
		exit(EXIT_FAILURE);
	}
	char *full_string = copied;

	strcpy(copied, original);
	parse_str = filter_string(copied);
	if (!strcmp(parse_str.value, attr)) {
		clean_outstr(&parse_str);
		parse_str = filter_string(parse_str.modified);
		if(!strcmp(parse_str.value, pname)) {
			clean_outstr(&parse_str);
			parse_str = filter_string(parse_str.modified);
			clean_outstr(&parse_str);
			parse_str = filter_string(parse_str.modified);
			value = parse_str.value;
		}
	}

	free(full_string);
	return value;
}

char *filter(char *original, pt_search_key key) {
	static const char *mname = "ModelName";
	static const char *pname = "Product";
	static const char *delim = " \t";
	static const char *delim_str = "\"";
	static const char *end = "\n";

	char *value = NULL;
	char *token = NULL;
	char *copied = malloc(strlen(original)+1);
	if (!copied) {
		fprintf(stderr, "Can't allocate memory\n");
		exit(EXIT_FAILURE);
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
				exit(EXIT_FAILURE);
			}
			strcpy(value, token);
			token = strsep(&copied, end);
			while(*token != '\0') {
				if(isblank(*token)) {
					token++;
					continue;
				} else {
					free(full_string);
					printf("String contains unexpected symbols:\n");
					printf("%s\n", original);
					exit(EXIT_FAILURE);
				}
			}
		} else {
			free(full_string);
			printf("String which contains %s is not correct", original);
			exit(EXIT_FAILURE);
		}
	}
	free(full_string);
	return value;
}

bool clean_product(prod *product) {
	bool free_buffer = false;
	if(product) {
		if(product->buffer) {
			free(product->buffer);
			free_buffer = true;
		}
	}
	return free_buffer;
}

void clean_strings(gpointer data) {
	if(data) {
		free((char*)data);
	}
}

void clean_model(gpointer data) {
	if(data)
	{
		model *mod = (model *)data;
		clean_product(&(mod->product));
		clean_info(&(mod->inform));
		free(mod);
	}
}

drvm *read_drv(FILE *fd) {
	size_t result;
	drvm *drv;
	GPtrArray *models;
	GPtrArray *strings;

	drv = malloc(sizeof(drvm));
	if (!drv) {
		fprintf(stderr, "Can't allocate memory\n");
		exit(EXIT_FAILURE);
	}
	globalres.drv = drv;

	uint32_t len;

	result = fread(&len, sizeof(uint32_t), 1, fd);
	if (result != 1) {
		fprintf(stderr, "Can't read data from file\n");
		exit(EXIT_FAILURE);
	}

	if(len <= 0) {
		fprintf(stderr, "Invalid length(%d)\n", len);
		exit(EXIT_FAILURE);
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

	result = fread(&len, sizeof(uint32_t), 1, fd);
	if (result != 1) {
		fprintf(stderr, "Can't read data from file\n");
		exit(EXIT_FAILURE);
	}

	if(len <= 0) {
		fprintf(stderr, "Invalid length(%d)\n", len);
		exit(EXIT_FAILURE);
	}

	elm_size = sizeof(char *);
	length = len;
	res_size = length * elm_size;
	strings = g_ptr_array_sized_new(length);
	drv->strings = strings;

	char *string;
	size_t size_buffer;
	ssize_t read;
	for (int i = 0; i < length; ++i) {
		string = NULL;
		if((read = getline(&string, &size_buffer, fd)) == -1) {
			fprintf(stderr, "Can't read line from file\n");
			exit(EXIT_FAILURE);
		}
		g_ptr_array_add(strings, string);
	}

	return drv;
}

model *read_model(FILE *fd) {
	model *mod;
	size_t result;
	uint32_t length;

	mod = malloc(sizeof(model));
	uint32_t len;
	if (!mod) {
		fprintf(stderr, "Can't allocate memory\n");
		exit(EXIT_FAILURE);
	}

	result = fread(&len, sizeof(uint32_t), 1, fd);
	if(result != 1) {
		fprintf(stderr, "Can't read data from file\n");
		exit(EXIT_FAILURE);
	}

	length = len;
	mod->name = length;

	result = fread(&len, sizeof(uint32_t), 1, fd);
	if(result != 1) {
		fprintf(stderr, "Can't read data from file\n");
		exit(EXIT_FAILURE);
	}

	if(len <= 0) {
		fprintf(stderr, "Invalid length(%d)\n", len);
		exit(EXIT_FAILURE);
	}

	length = len;
	if ((length >> 30) > 0)
	{
		fprintf(stderr, "Integer overflow\n");
		exit(EXIT_FAILURE);
	}

	uint32_t size_buffer = length * sizeof(uint32_t);

	if(!((mod->product).buffer = malloc(size_buffer))) {
		fprintf(stderr, "Can't allocate memory\n");
		exit(EXIT_FAILURE);
	}
	(mod->product).len = length;

	result = fread((mod->product).buffer, size_buffer, 1, fd);
	if(result != 1) {
		fprintf(stderr, "Can't read data from file\n");
		exit(EXIT_FAILURE);
	}

	// Parse part related to product

	result = fread(&len, sizeof(uint32_t), 1, fd);
	if(result != 1) {
		fprintf(stderr, "Can't read data from file\n");
		exit(EXIT_FAILURE);
	}

	if(len <= 0) {
		fprintf(stderr, "Invalid length(%d)\n", len);
		exit(EXIT_FAILURE);
	}

	length = len;
	if ((length>>(32 - sizeof(uint32_t))) > 0)
	{
		fprintf(stderr, "Integer overflow\n");
		exit(EXIT_FAILURE);
	}

	size_buffer = length * sizeof(uint32_t);
	if(!(mod->inform.buffer = malloc(size_buffer))) {
		fprintf(stderr, "Can't allocate memory\n");
		exit(EXIT_FAILURE);
	}
	mod->inform.len = length;
	result = fread(mod->inform.buffer, size_buffer, 1, fd);
	if(result != 1) {
		fprintf(stderr, "Can't read data from file\n");
		exit(EXIT_FAILURE);
	}

	return mod;
}

bool clean_info(info *inf) {
	bool free_buffer = false;
	if(inf) {
		if(inf->buffer) {
			free(inf->buffer);
			free_buffer = true;
		}
	}
	return free_buffer;
}

void help(void) {
	fprintf(stderr, "Extract ppd file by model name:\n");
	fprintf(stderr, "Usage: %s -m <model> -i <database>\n\n", gargv[0]);
	fprintf(stderr, "Extract ppd file by product name:\n");
	fprintf(stderr, "Usage: %s -p <product> -i <database>\n\n", gargv[0]);
	fprintf(stderr, "Extract ppd file either by model name or product name:\n");
	fprintf(stderr, "Usage: %s -a <key> -i <database>\n", gargv[0]);
}

config *initialization(int argc, char **argv) {
	int opt;
	bool product = false;
	bool model = false;
	bool drvfile = false;

	int res = atexit(clean_all_resources);
	if(res) {
		perror("atexit() failed");
	}

	config *cfg = malloc(sizeof(config));
	if (!cfg) {
		fprintf(stderr, "Can't allocate memory\n");
		exit(EXIT_FAILURE);
	}

	while ((opt = getopt(argc, argv, "i:m:p:a:r:")) != -1) {
		switch (opt) {
		case 'i':
			if (!drvfile) {
				cfg->drvstr = strdup(optarg);
				if (!cfg->drvstr) {
					fprintf(stderr, "Can't duplicate string with strdup()\n");
					exit(EXIT_FAILURE);
				}
				drvfile = true;
			} else {
				fprintf(stderr, "Don't support extraction from multiple databases\n");
				fprintf(stderr, "Do not provide some \'-i\'' options\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'p':
			if (!product) {
				cfg->product = strdup(optarg);
				if (!cfg->product)
				{
					fprintf(stderr, "Can't duplicate string with strdup()\n");
					exit(EXIT_FAILURE);
				}
				product = true;
			} else {
				fprintf(stderr, "Supported extraction only of one ppd file by product\n");
				fprintf(stderr, "Do not provide some \'-p\'' options\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'm':
			if (!model) {
				cfg->model = strdup(optarg);
				if (!cfg->model)
				{
					fprintf(stderr, "Can't duplicate string with strdup()\n");
					exit(EXIT_FAILURE);
				}
				model = true;
			} else {
				fprintf(stderr, "Supported extraction only of one ppd file by modelname\n");
				fprintf(stderr, "Do not provide some \'-m\'' options\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'r':
		case 'a':
			if (!model) {
				cfg->model = strdup(optarg);
				if (!cfg->model) {
					fprintf(stderr, "Can't allocate memory\n");
					exit(EXIT_FAILURE);
				}
				model = true;
			} else {
				fprintf(stderr, "Supported extraction only of one ppd file by modelname\n");
				fprintf(stderr, "Do not provide some \'-m\'' options\n");
				exit(EXIT_FAILURE);
			}

			if (!product) {
				cfg->product = strdup(optarg);
				if (!cfg->product) {
					fprintf(stderr, "Can't duplicate string with strdup()\n");
					exit(EXIT_FAILURE);
				}
				product = true;
			} else {
				fprintf(stderr, "Supported extraction only of one ppd file by product\n");
				fprintf(stderr, "Do not provide some \'-p\'' options\n");
				exit(EXIT_FAILURE);
			}
			break;
		default:				/* '?' */
			help();
			exit(EXIT_FAILURE);
		}
	}

	cfg->find_product = product;
	cfg->find_model = model;

	if (!cfg->drvstr) {
		fprintf(stderr, "drv file is not provided\n");
		exit(EXIT_FAILURE);
	}

	cfg->drvfile = fopen(cfg->drvstr, "rb");
	if (cfg->drvfile == NULL) {
		fprintf(stderr, "cannot open %s: %s\n", cfg->drvstr, strerror(errno));
		exit(EXIT_FAILURE);
	}

	int size = get_file_size(cfg->drvstr);
	char *buffer = malloc(size);
	if (!buffer) {
		fprintf(stderr, "cannot allocate memory: %s\n", strerror(errno));
		exit (EXIT_FAILURE);
	}
	globalres.buffer = buffer;

	int result = setvbuf(cfg->drvfile, buffer, _IOFBF,  size);
	if (result) {
		fprintf(stderr, "Can't call setvbuf: %s\n", strerror(errno));
		exit (EXIT_FAILURE);
	}
	return cfg;
}

int main(int argc, char **argv)
{
	gargv = argv;
	config *cfg = initialization(argc, argv);
	if (!cfg) {
		fprintf(stderr, "Unexpected error\n");
		exit(EXIT_FAILURE);
	}

	drvm *drv = read_drv(cfg->drvfile);
	if (!drv) {
		fprintf(stderr, "Can't read drv file\n");
		exit(EXIT_FAILURE);
	}
	char *filename;

	if (cfg->find_model && !cfg->find_product) {
		filename = extract_drv_by_model(*drv, cfg->model);
	} else if (cfg->find_product && !cfg->find_model) {
		filename = extract_drv_by_product(*drv, cfg->product);
	} else if (cfg->find_model && cfg->find_product) {
		if (strcmp(cfg->product, cfg->model)) {
			fprintf(stderr, "Supported extraction only of one ppd file\n");
			fprintf(stderr, "Do not provide mixed \'-m\',\'-p\',\'-a\' with different names\n");
			exit(EXIT_FAILURE);
		}

		filename = extract_drv_by_model(*drv, cfg->model);
		if (!filename) {
			filename = extract_drv_by_product(*drv, cfg->product);
		}
	} else {
		help();
		exit(EXIT_FAILURE);
	}

	if (!filename) {
		fprintf(stderr, "Couldn't find suitable ppd file\n");
		exit(EXIT_FAILURE);
	}

	char *output = exec_ppdc(filename);
	if(!output) {
		printf("Execution of ppdc failed\nCheck LANG environment\n");
		exit(EXIT_FAILURE);
	}
	printf("%s", output);
	exit(EXIT_SUCCESS);
}
