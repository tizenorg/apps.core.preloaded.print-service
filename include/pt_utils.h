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

#ifndef __PT_UTILS_H__
#define __PT_UTILS_H__
#include <Ecore.h>
#include <net/if.h>

#define MAX_SIZE	128
#define MAX_MSG_LEN 512

typedef enum {
	COMMAND_HELP,
	COMMAND_VERSION,
	COMMAND_BROWSE_SERVICES,
	COMMAND_BROWSE_ALL_SERVICES,
	COMMAND_BROWSE_DOMAINS
#if defined(HAVE_GDBM) || defined(HAVE_DBM)
	, COMMAND_DUMP_STDB
#endif
} Command;
typedef struct Config {
	int verbose;
	int terminate_on_all_for_now;
	int terminate_on_cache_exhausted;
	char *domain;
	char *stype;
	int ignore_local;
	Command command;
	int resolve;
	int no_fail;
	int parsable;
#if defined(HAVE_GDBM) || defined(HAVE_DBM)
	int no_db_lookup;
#endif
} Config;

/*********** search *****************/

/**
 *	This function let the app clear the list of previous searching
 *	@return    void
 *	@param[in] list the pointer to the printers list
 */
void pt_utils_free_search_list(Eina_List *list);

/**
 *	This function let the app clear the list of previous searching
 *	@return    void
 *	@param[in] list the pointer to the printers list
 */
void pt_utils_free_local_printer_list(Eina_List *list);

void pt_utils_free_printing_thd_list(Eina_List *list);

/*********** print *****************/

/**
 *	This function let the app register the printer to the server
 *	@return    If success, return PT_ERR_NONE, else return PT_ERR_FAIL
 *	@param[in] printer_name the pointer to the printer's name
 *	@param[in] scheme the pointer to the register's scheme
 *	@param[in] ip_address the pointer to the printer's address
 *	@param[in] ppd_file the pointer to the printer's ppd file
 */
int  pt_utils_regist_printer(char *printer_name, char *scheme, char *ip_address, char *ppd_file);

char *pt_utils_filename_from_URI(const char *uri);

int __standardization(char *name);

int pt_utils_get_mfg_mdl(const char *printer, char **mfg, char **mdl);


#endif /* __PT_UTILS_H__ */
