/*
*	Printservice
*
* Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*					http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#ifndef __PRINT_GETPPD_H__
#define __PRINT_GETPPD_H__

#include <stdio.h>
#include <stdbool.h>
#include <pt_db.h>

typedef enum {
	PT_FORCE_CLEAN = 0,
	PT_SOFT_CLEAN       // Don't fail if file doesn't exist
} pt_clean_file;

struct _config {
	const char *drvstr;
	const char *product;
	const char *model;
	bool find_product;
	bool find_model;
};

typedef struct _config config;

struct _resource {
	config *cfg;
	drvm *drv;
	char *buffer;
	char *tempfile;
};

typedef struct _resource resource;

/* Base initialization function */
config *initialization(int argc, char **argv);

/* Free all resources which was allocated/opened*/
void clean_all_resources(void);

/* Free memory allocated by drvm structure */
bool clean_drvm(drvm *drv);

/* Clean buffer which is allocated for */
/* file operations bufferization (I/O) */
void clean_buffer(char *buffer);

/* Delete temporary files */
bool clean_tmp_file(const char *file, pt_clean_file type);

/* Output help strings */
void help(void);

#endif /* __PRINT_GETPPD_H__ */
