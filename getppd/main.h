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

#include <bfd.h>
#include <errno.h>
#include <execinfo.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdint.h>

#define PT_RET_IF(expr, fmt, args...) \
	do { \
		if(expr) { \
			fprintf(stderr,"[%s] Return, message "fmt, #expr, ##args );\
			return; \
		} \
	} while (0)

#define PT_RETV_IF(expr, val, fmt, args...) \
	do { \
		if(expr) { \
			fprintf(stderr,"[%s] Return value, message "fmt, #expr, ##args );\
			return (val); \
		} \
	} while (0)

#define PT_IF_FREE_MEM(mem) \
	do { \
		if(mem) { \
			free(mem);\
			mem = NULL; \
		} \
	} while (0)

typedef enum {
	PT_MODELNAME = 0,
	PT_PRODUCTNAME,
} pt_search_key;

typedef enum {
	PT_FORCE_CLEAN = 0,
	PT_SOFT_CLEAN       // Don't fail if file doesn't exist
} pt_clean_file;

struct _info {
	uint32_t *buffer;
	uint32_t len;
};

typedef struct _info info;

struct _prod {
	uint32_t *buffer;
	uint32_t len;
};

typedef struct _prod prod;

struct _model {
	uint32_t name;
	prod product;
	info inform;
};

typedef struct _model model;

struct _drvm {
	GPtrArray *models;
	GPtrArray *strings;
};

typedef struct _drvm drvm;

struct _config {
	FILE *drvfile;
	const char *drvstr;
	const char *product;
	const char *model;
	const char *all;
	bool find_product;
	bool find_model;
};

typedef struct _config config;

struct _outstr {
	char *value;
	char *modified;
};

struct _resource {
	config *cfg;
	drvm *drv;
	char *buffer;
	char *tempfile;
};

typedef struct _resource resource;

typedef struct _outstr outstr;

/* Get size of file in bytes */
int get_file_size(const char *path);

/* Create temporary file */
/* Return file description and */
/* path to the created file  */
FILE *create_tmp_file(char **out_fname);

/* Delete temporary files */
bool clean_tmp_file(const char *file, pt_clean_file type);

/* Execute ppdc command with specific parameters */
char *exec_ppdc(const char *file);

/* Extract specific drv file from database by modelname */
char *extract_drv_by_model(drvm drv, const char *modelname);

/* Extract specific drv file from database by product */
char *extract_drv_by_product(drvm drv, const char *product);

/* Extract specific drv file from database by specified key */
char *extract_drv_by_key(drvm drv, const char *key, pt_search_key type);

/* Read and parse drv file */
/* Return parsed structure */
drvm *read_drv(FILE *FD);

/* Read and parse drv file */
/* Return model structure */
model *read_model(FILE* FD);

/* Extract value of search key accordingly    */
/* specification of drv file */
char *filter(char *string, pt_search_key key);

/* Extraction value of Product */
/* accordingly specification of drv file */
char *filter_product(char *original, pt_search_key key);

/* Return value of a string, deleting quotes */
outstr filter_string(const char *original);

/* Clean memory of allocated outstr structure */
void clean_outstr(outstr *ostr);

/* Base initialization function */
config *initialization(int argc, char **argv);

/* Read all data from file/pipe */
char *reliable_read(int fd);

/* Output help strings */
void help(void);

/* Free memory allocated by drvm structure */
bool clean_drv(drvm *drv);

/* Free memory allocated by prod structure */
bool clean_product(prod *product);

/* Free memory allocated by string */
void clean_strings(gpointer data);

/* Free memory allocated by model structure */
void clean_model(gpointer data);

/* Free memory allocated by info structure */
bool clean_info(info *inf);

/* Free all resources which was allocated/opened*/
void clean_all_resources(void);

/* Clean buffer which is allocated for */
/* file operations bufferization (I/O) */
void clean_buffer(char *buffer);
