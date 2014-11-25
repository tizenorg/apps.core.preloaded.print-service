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

#ifndef __PRINT_PTDB_H__
#define __PRINT_PTDB_H__

#include <stdint.h>
#include <glib.h>

typedef enum {
	PT_SEARCH_MODEL = 0, /** search ppd file by ModelName field */
	PT_SEARCH_PROD,		 /** search ppd file by Product field */
	PT_SEARCH_ALL		 /** search ppd file by PT_SEARCH_MODEL or PT_SEARCH_PROD */
} pt_search;

typedef struct {
	GPtrArray *models;
	GPtrArray *strings;
} drvm;

typedef struct {
	FILE *dbfile;
	const char *dbpath;
	char *dbbuffer;
} pt_dbconfig;

typedef struct {
	pt_dbconfig *dbconfig;
	drvm *dbstruct;
} pt_db;

/**
 *	Create in-core representation of print-service database.
 *  This database is used for searching, extracting ppd files.
 *	@return   If success, returns pointer to in-core database representation ,
 *  else returns NULL
 *	@param[in] absolute or relative path to the database file
 */
pt_db *pt_create_db(const char *path);

/**
 *	Extract ppd file
 *	@return   If success, returns string with ppdc output,
 *  else returns NULL
 *	@param[in] database pointer to the database
 *	@param[in] search key used for searching ppd file
 *	@param[in] type which type of searching will be performed
 */
char* pt_extract_ppd(pt_db *database, const char *search, pt_search type);

/**
 *	Free memory and other resources used by in-core database representation.
 *	@return   If success, returns pointer to in-core database representation ,
 *  else returns NULL
 *	@param[in] database pointer to the database
 */
void pt_free_db(pt_db *database);

#endif /* __PRINT_PTDB_H__ */