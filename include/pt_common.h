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

#ifndef __PRINT_SERVICE_H__
#define __PRINT_SERVICE_H__

#include <cups/cups.h>
#include <cups/ppd.h>
#include "pt_api.h"
#include "pt_utils.h"

#define MAX_URI_SIZE (512)

typedef struct {
	char product_ui_name[PT_MAX_LENGTH];		/* product display name in UI */
	char device_info[PT_MAX_LENGTH];	/* printer info used as key of hashtable*/
	char device_uri[PT_MAX_LENGTH];			/* url */
	char model_name[PT_MAX_LENGTH];			/* mdl name */
	char manufacturer[PT_MAX_LENGTH];			/* mfg name	*/
	char ppd_file_path[PT_MAX_LENGTH];                    /** ppd file name (with full path) */
//	int	color;							/* 0-F, 1-T, 2-U */
//	int	duplex;							/* 0-F, 1-T, 2-U */
} pt_printer_info_t;

/*
typedef struct {
	pt_printer_info_t *printer_info;
	pt_printer_status_t *printer_status;
} pt_printer_t;

typedef struct {
} pt_printer_status_t;

typedef struct {
	pts_printing_data_t *data;
	pt_job_info_t	*info;
	pt_notification_t *notification_plugin;
	pt_print_option_t *option;
} pt_job_t;

typedef struct {
} pt_print_option_t;

typedef struct {
} pt_notification_t;
*/


typedef struct {
	int  id;									/** job id */
	int  priority;								/** priority (1-100) */
	int  subscription_id;
	int  sequence_num;
	int  progress;
	pt_job_state_e status;						/** job status */
	char title[PT_MAX_LENGTH];					/** title/job name */
	char user[PT_MAX_LENGTH];					/** reserved, username who submitted the job */
	pt_printer_mgr_t *printer;					/** the pointer to the associated printer object */
} pt_job_info_t;

typedef struct {
	int is_searching;
	Eina_List *pt_local_list;
	pt_response_data_t response_data;
	get_printers_cb user_cb;
} pt_search_data_t;

typedef enum {
	PT_SEARCH_IDLE,
	PT_SEARCH_IN_PROGRESS,
	PT_SEARCH_CANCEL,
	PT_SEARCH_END,
} pt_searching_state_e;

typedef enum {
	PT_PRINT_IDLE,
	PT_PRINT_HELD,
	PT_PRINT_PENDING,
	PT_PRINT_IN_PROGRESS,
	PT_PRINT_ABORT,
	PT_PRINT_CANCEL,
	PT_PRINT_STOP,
	PT_PRINT_END,
} pt_printing_state_e;

typedef struct {
	const char			**files;
	int					num_files;
	Ecore_Thread		*printing_thd_hdl;
	pt_job_info_t		*job;					/** job info */
	pt_printing_state_e	printing_state;
	Ecore_Timer			*job_noti_timer;
	int					page_printed;
} pt_printing_job_t;

typedef struct {
	pt_connection_type_e connect_type;			/** assigned connection type */
	pt_printer_mgr_t	*active_printer;		/** active printer info */
	pt_search_data_t		*search;

	cups_option_t		*job_options;
	int num_options;							/** print job option */

	int					ueventsocket;			/* hotplug event socket */
	int					avahi_pid;				/* avahi pid */
	int					cups_pid;				/* cups pid */

	Ecore_Timer			*cups_checking_timer;
	Ecore_Thread		*searching_thd_hdl;
	pt_searching_state_e	searching_state;
	Eina_List			*printing_thd_list;

	pt_event_cb			evt_cb;
	void				*user_data;
} pt_info_t;

extern pt_info_t *g_pt_info;					/** print info maintained globally */
extern ppd_file_t *ppd;

#endif /* __PRINT_SERVICE_H__ */
