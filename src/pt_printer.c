/*
*	Printservice
*
* Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#define _GNU_SOURCE
#include <sys/types.h>
#include <app.h>
#include <stdbool.h>
#include <glib.h>

#include "pt_debug.h"
#include "pt_common.h"
#include "pt_ppd.h"
#include "pt_printer.h"
#include "pt_utils.h"

#define PREFERENCE_DEFAULT_PRINTER_NAME "mobileprint_default_printer_name"
#define PREFERENCE_DEFAULT_PRINTER_ADDRESS "mobileprint_default_printer_address"
#define PREFERENCE_DEFAULT_PRINTER_MFG "mobileprint_default_printer_mfg"
#define PREFERENCE_DEFAULT_PRINTER_MDL "mobileprint_default_printer_mdl"

static void g_printer_data_destroy(gconstpointer a)
{
	free((char *)a);
}

static gboolean __printer_compare(gconstpointer a, gconstpointer b)
{
	pt_printer_info_t *printer_a = (pt_printer_info_t *)a;
	pt_printer_info_t *printer_b = (pt_printer_info_t *)b;
	PT_RETV_IF(a==NULL || b==NULL, FALSE, "a or b is NULL");

	if (strcmp(printer_a->device_uri, printer_b->device_uri) == 0) {
		return TRUE;
	} else if (!strncmp(printer_a->device_uri, "dnssd:",6) && !strncmp(printer_b->device_uri, "dnssd:",6)) {
		if (strcmp(printer_a->device_info, printer_b->device_info) == 0) {
			return TRUE;
		} else {
			return FALSE;
		}
	}
	return FALSE;
}

static pt_printer_info_t *pt_util_make_printer_info(const char *device_info,
		const char *device_make_and_model,
		const char *device_uri)
{
	pt_printer_info_t *localdetail = NULL;
	localdetail = (pt_printer_info_t *)malloc(sizeof(pt_printer_info_t));
	PT_RETV_IF(localdetail == NULL, NULL, "localdetail is NULL");
	memset(localdetail, 0, sizeof(pt_printer_info_t));

	char *device_mfg = NULL;
	char *device_mdl = NULL;
	char *device_name = NULL;
	int ret = 0;
	ret = pt_utils_get_mfg_mdl(device_make_and_model, &device_mfg, &device_mdl);
	if (ret != PT_ERR_NONE) {
		PT_IF_FREE_MEM(localdetail);
		PT_IF_FREE_MEM(device_mfg);
		PT_IF_FREE_MEM(device_mdl);
		PT_DEBUG("ERROR : %d in pt_utils_get_mfg_mdl", ret);
		return NULL;
	}
	if (device_mfg == NULL) {
		PT_IF_FREE_MEM(localdetail);
		PT_IF_FREE_MEM(device_mdl);
		PT_DEBUG("device_mfg == NULL");
		return NULL;
	}
	if (device_mdl == NULL) {
		PT_IF_FREE_MEM(localdetail);
		PT_IF_FREE_MEM(device_mfg);
		PT_DEBUG("device_mdl == NULL");
		return NULL;
	}

	device_name = calloc(1, strlen(device_mfg)+strlen(device_mdl)+2);
	if (device_name == NULL) {
		PT_IF_FREE_MEM(localdetail);
		PT_IF_FREE_MEM(device_mfg);
		PT_IF_FREE_MEM(device_mdl);
		PT_DEBUG("No memory. calloc failed");
		return NULL;
	}

	strncat(device_name,device_mfg,strlen(device_mfg));
	strncat(device_name," ",1);
	strncat(device_name,device_mdl,strlen(device_mdl));

	strncpy(localdetail->model_name, device_mdl, sizeof(localdetail->model_name)-1);
	strncpy(localdetail->manufacturer, device_mfg, sizeof(localdetail->manufacturer)-1);
	strncpy(localdetail->product_ui_name, device_name, sizeof(localdetail->product_ui_name)-1);
	__standardization(localdetail->product_ui_name);
	strncpy(localdetail->device_uri, device_uri, sizeof(localdetail->device_uri)-1);
	strncpy(localdetail->device_info, device_info, sizeof(localdetail->device_info)-1);

	PT_IF_FREE_MEM(device_mfg);
	PT_IF_FREE_MEM(device_mdl);
	PT_IF_FREE_MEM(device_name);

	return localdetail;
}

static void pt_util_add_printer_to_hashlist(GHashTable *printer_hashlist, pt_printer_info_t *printer)
{
	PT_RET_IF(printer == NULL, "printer is NULL");
	PT_RET_IF(printer_hashlist == NULL, "printer_hashlist is NULL");

	pt_printer_info_t *tempdetail = NULL;
	tempdetail = (pt_printer_info_t *) g_hash_table_lookup(printer_hashlist, printer);

	if (NULL == tempdetail) {
		g_hash_table_insert(printer_hashlist, printer, printer);
		PT_DEBUG("Added printer_hashlist: %s", printer->device_uri);
	} else {
		if (strstr(tempdetail->device_uri, "_printer.") && strstr(printer->device_uri, "_ipp.")) {
			g_hash_table_replace(printer_hashlist, printer, printer);
			PT_DEBUG("Replaced printer_hashlist: %s", printer->device_uri);
		} else if (strstr(tempdetail->device_uri, "_pdl-datastream.") &&
				   (strstr(printer->device_uri, "_printer.") || strstr(printer->device_uri, "_ipp."))) {
			g_hash_table_replace(printer_hashlist, printer, printer);
			PT_DEBUG("Replaced printer_hashlist: %s", printer->device_uri);
		}
	}
}

static int compare_printer(pt_printer_mgr_t *printer_a, pt_printer_mgr_t *printer_b)
{
	if (printer_a->is_ppd_exist == TRUE && printer_b->is_ppd_exist == FALSE) {
		return -1;
	} else if (printer_a->is_ppd_exist == FALSE && printer_b->is_ppd_exist == TRUE) {
		return 1;
	} else {
		if (strcmp(printer_a->name, printer_b->name) != 0) {
			return strcmp(printer_a->name, printer_b->name);
		}
		else {
			return strcmp(printer_a->address, printer_b->address);
		}
	}
}

static void pt_util_append_printer(pt_search_data_t *search_data, GHashTable *printer_hashlist)
{
	GHashTableIter iter;
	gpointer key, value;
	pt_printer_info_t *printer_temp = NULL;
	Eina_Compare_Cb cmp_func = (Eina_Compare_Cb)compare_printer;

	g_hash_table_iter_init(&iter, printer_hashlist);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		printer_temp = (pt_printer_info_t *)malloc(sizeof(pt_printer_info_t));
		PT_RET_IF(printer_temp == NULL, "printer_temp is NULL");
		memset(printer_temp, 0, sizeof(pt_printer_info_t));
		memcpy(printer_temp, value, sizeof(pt_printer_info_t));
		search_data->pt_local_list = eina_list_append(search_data->pt_local_list, (pt_printer_info_t *)printer_temp);
		if (eina_error_get()) {
			PT_DEBUG("Failed to add eina_list for search_data->pt_local_list");
		}
	}

	/* add searched printer */
	Eina_List *cursor = NULL;
	pt_printer_info_t *it = NULL;
	pt_printer_mgr_t *detail = NULL;

	//TODO:: free ( search_data->response_data.printerlist );  -- dwmax
	search_data->response_data.printerlist = NULL;

	Eina_Bool bfirst = EINA_TRUE;
	/* add printer to response list */
	EINA_LIST_FOREACH(search_data->pt_local_list, cursor, it) {
		detail = (pt_printer_mgr_t *)malloc(sizeof(pt_printer_mgr_t));
		if (detail != NULL) {
			memset(detail, 0, sizeof(pt_printer_mgr_t));
			detail->copies = 1;
			strncpy(detail->name, it->product_ui_name, PT_MAX_LENGTH -1);
			strncpy(detail->address, it->device_uri, PT_MAX_LENGTH -1);
			strncpy(detail->mdl, it->model_name, PT_MAX_LENGTH -1);
			strncpy(detail->mfg, it->manufacturer, PT_MAX_LENGTH -1);
			PT_DEBUG("product %s",it->product_ui_name);
			PT_DEBUG("url %s",it->device_uri);
			PT_DEBUG("mdl %s",it->model_name);
			PT_DEBUG("mfg %s",it->manufacturer);

			if (EINA_TRUE == bfirst) {
				bfirst = EINA_FALSE;
			}

			if (pt_get_printer_ppd(detail) != PT_ERR_NONE) {
				//Unsupported Printer
				detail->is_ppd_exist = FALSE;
			} else {
				detail->is_ppd_exist = TRUE;
			}

//			search_data->response_data.printerlist = eina_list_append(search_data->response_data.printerlist, detail);
			search_data->response_data.printerlist = eina_list_sorted_insert(search_data->response_data.printerlist, cmp_func,detail);
			if (eina_error_get()) {
				PT_DEBUG("Failed to add eina_list for search_data->response_data.printerlist");
			}
		}
	}

}

static void searching_thread_notify_cb(void *data, Ecore_Thread *thread, void *msg_data)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RET_IF(data == NULL, "data is NULL");
	PT_RET_IF(msg_data == NULL, "msg_data is NULL");
	PT_DEBUG("Thread is sent msg successfully.");

	pt_search_data_t 	*search_data 	= NULL;
	ipp_t 			*response 		= NULL;

	search_data = (pt_search_data_t *)data;
	response = (ipp_t *)msg_data;

	PT_RET_IF(search_data->user_cb == NULL, "user_cb is NULL");

	GHashTable *printer_hashlist = g_hash_table_new_full(g_str_hash, __printer_compare, NULL, (GDestroyNotify)g_printer_data_destroy);

	ipp_attribute_t *attr 				= NULL;
	const char *device_uri 			= NULL;	/* device-uri attribute value */
	const char *device_info 			= NULL;	/* device-info value */
	const char *device_make_and_model	= NULL;	/* device-make-and-model value */

	for (attr = ippFirstAttribute(response); attr != NULL; attr = ippNextAttribute(response)) {


		if (ippGetGroupTag(attr) == IPP_TAG_ZERO) {
			PT_DEBUG("-----------------------------------------");
		} else if (ippGetGroupTag(attr) != IPP_TAG_PRINTER) {
			continue;
		} else if (!strcmp(ippGetName(attr), "device-info") && ippGetValueTag(attr) == IPP_TAG_TEXT) {
			device_info = ippGetString(attr, 0, NULL);
		} else if (!strcmp(ippGetName(attr), "device-make-and-model") && ippGetValueTag(attr) == IPP_TAG_TEXT) {
			device_make_and_model = ippGetString(attr, 0, NULL);
		} else if (!strcmp(ippGetName(attr), "device-uri") && ippGetValueTag(attr) == IPP_TAG_URI) {
			device_uri = ippGetString(attr, 0, NULL);
		} else {
			continue;
		}

		if (device_info == NULL || device_make_and_model == NULL
				|| device_uri == NULL) {
			continue;
		} else if (strcasestr(device_uri, "usb://") ||
				   strstr(device_uri,"_pdl-datastream.") ||
				   strstr(device_uri,"_ipp.") ||
				   strstr(device_uri,"_printer.")) {
			pt_printer_info_t *localdetail = NULL;
			localdetail = pt_util_make_printer_info(device_info, device_make_and_model, device_uri);
			pt_util_add_printer_to_hashlist(printer_hashlist, localdetail);
		} else {
			PT_DEBUG("the device has another scheme such as socket");
		}

		device_info 				= NULL;
		device_uri				= NULL;
		device_make_and_model	= NULL;

	}

	pt_util_append_printer(search_data, printer_hashlist);
	g_hash_table_destroy(printer_hashlist);
	ippDelete(response);

	search_data->user_cb(&search_data->response_data);

	PRINT_SERVICE_FUNC_LEAVE;
}

static void searching_thread_end_cb(void *data, Ecore_Thread *thread)
{
	PRINT_SERVICE_FUNC_ENTER;
	g_pt_info->searching_state = PT_SEARCH_END;
	PT_DEBUG("Thread is completed successfully.");
	PRINT_SERVICE_FUNC_LEAVE;
}

static void searching_thread_cancel_cb(void *data, Ecore_Thread *thread)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_DEBUG("Thread is canceled successfully.");
	PRINT_SERVICE_FUNC_LEAVE;
}

static void searching_thread(void *data, Ecore_Thread *thread)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RET_IF(data == NULL, "data is NULL");
	PT_RET_IF(thread == NULL, "thread is NULL");

	http_t *http 				= NULL;
	ipp_t *request 			= NULL;
	ipp_t *response 			= NULL;
	int connection_status		= 0;
	int ret 					= -1;
	char *exclude_schemes[] = {
		"http",
		"https",
		"ipp",
		"ipp15",
		"ipps",
		"lpd",
		"smb",
		"snmp",
		"socket",
		NULL,
		NULL
	};
	int num_schemes = 9;

	http = httpConnectEncrypt(cupsServer(), ippPort(), cupsEncryption());
	PT_RET_IF(http == NULL, "unable to connect server");

	request = ippNewRequest(CUPS_GET_DEVICES);
	PT_RET_IF(request == NULL, "unable to create request");

	ret = pt_get_connection_status(&connection_status);

	if (ret != PT_ERR_NONE) {
		ippDelete(request);
		PT_DEBUG("unable to get connection status");
	}

	if ((connection_status & PT_CONNECTION_USB) == 0) {
		exclude_schemes[num_schemes++] = "usb";
	}
	if ((connection_status & PT_CONNECTION_WIFI) == 0 &&
			(connection_status & PT_CONNECTION_WIFI_DIRECT) == 0) {
		exclude_schemes[num_schemes++] = "dnssd";
	}

	ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "exclude-schemes",
				  num_schemes ,NULL, exclude_schemes);
	ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER, "timeout",5);

	if ((response = cupsDoRequest(http, request, "/")) != NULL) {
		PT_DEBUG("CUPS_GET_DEVICES (%s)",cupsLastErrorString());
		if (g_pt_info->searching_state == PT_SEARCH_CANCEL) {
			ippDelete(response);
			goto canceled;
		}

		if (ecore_thread_feedback(thread, (const void *)response) == EINA_FALSE) {
			PT_DEBUG("Failed to send data to main loop");
			ippDelete(response);
		}
	} else {
		PT_DEBUG("CUPS_GET_DEVICES (%s)", cupsLastErrorString());
	}
canceled:
	httpClose(http);
	PRINT_SERVICE_FUNC_LEAVE;
}

static void __pt_get_printers(pt_search_data_t *data)
{
	PRINT_SERVICE_FUNC_ENTER;

	g_pt_info->searching_thd_hdl = ecore_thread_feedback_run(
									   searching_thread,
									   searching_thread_notify_cb,
									   searching_thread_end_cb,
									   searching_thread_cancel_cb,
									   data,
									   EINA_FALSE);

	PRINT_SERVICE_FUNC_LEAVE;
}

static Eina_Bool __pt_check_cups_before_searching(void *data)
{
	PRINT_SERVICE_FUNC_ENTER;
	Eina_Bool ret = ECORE_CALLBACK_RENEW;

	if (g_pt_info->cups_pid > 0) {
		pt_search_data_t *ad = (pt_search_data_t *)data;
		__pt_get_printers(ad);
		ret = ECORE_CALLBACK_CANCEL;
	}

	PRINT_SERVICE_FUNC_LEAVE;
	return ret;
}

/***
* '__enable_printer()' - Enable a printer...
* I - Server connection
* I - Printer to enable
* O - 0 on success, 1 on fail
***/

static int	__enable_printer(http_t *http, char *printer, char *device_uri)
{
	/* IPP Request */
	ipp_t *request;
	ipp_t *response;	  /* IPP Response */
	char uri[HTTP_MAX_URI];  /* URI for printer/class */

	/*
	* Build a CUPS_ADD_PRINTER request, which requires the following
	* attributes:
	*
	*	 attributes-charset
	*	 attributes-natural-language
	*	 printer-uri
	*	 printer-state
	*	 printer-is-accepting-jobs
	*/

	//request = ippNewRequest(CUPS_ADD_PRINTER);
	request = ippNewRequest(CUPS_ADD_MODIFY_PRINTER);

	//httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL, "localhost", 0, "/printers/%s", printer);
	httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL, "localhost", 631, "/printers/%s", printer);
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);

	if (device_uri[0] == '/') {
		snprintf(uri, sizeof(uri), "file://%s", device_uri);
		ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_URI, "device-uri", NULL, uri);
	} else {
		ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_URI, "device-uri", NULL, device_uri);
	}

	ippAddInteger(request, IPP_TAG_PRINTER, IPP_TAG_ENUM, "printer-state", IPP_PRINTER_IDLE);

	ippAddBoolean(request, IPP_TAG_PRINTER, "printer-is-accepting-jobs", 1);

	/*
	* Do the request and get back a response...
	*/
	if ((response = cupsDoRequest(http, request, "/admin/")) == NULL) {
		PT_DEBUG("cupsDoRequest Failed");
		return (1);
	} else if (ippGetStatusCode(response) > IPP_OK_EVENTS_COMPLETE) {
		PT_DEBUG("%x greater than IPP_OK_EVENTS_COMPLETE -- Failed", ippGetStatusCode(response));
		ippDelete(response);
		return (1);
	} else {
		ippDelete(response);
		return (0);
	}

}

static int	__set_printer_ppd(http_t *http, char *printer, char *ppd_file)
{
	ipp_t *request = NULL;
	ipp_t *response = NULL;
	char uri[HTTP_MAX_URI] = {0,};

	httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL, "localhost", 0, "/printers/%s", printer);

	request = ippNewRequest(CUPS_ADD_MODIFY_PRINTER);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsUser());

	if (ppd) {
		ppdClose(ppd);
		ppd = NULL;
	}

	if (ppd_file != NULL) {
		PT_DEBUG("ppd file is not NULL. %s", ppd_file);

		response = cupsDoFileRequest(http, request, "/admin/", ppd_file);

		if (response != NULL) {
			ippDelete(response);
			ppd = ppdOpenFile(ppd_file);
			if (ppd != NULL) {
				pt_parse_options(ppd);
			}
		} else {
			PT_DEBUG("Failed to call cupsDoFileRequest");
		}
	} else {
		PT_DEBUG("ppd file is NULL");
		ippDelete(request);
		return 1;
	}

	if (cupsLastError() > IPP_OK_CONFLICT) {
		PT_DEBUG("The request is failed, detail description: %s\n", cupsLastErrorString());
		return 1;
	} else {
		return 0;
	}
}

/**
 *	This function let the app register the printer to the server
 *	@return    If success, return PT_ERR_NONE, else return PT_ERR_FAIL
 *	@param[in] printer_name the pointer to the printer's name
 *	@param[in] scheme the pointer to the register's scheme
 *	@param[in] ip_address the pointer to the printer's address
 *	@param[in] ppd_file the pointer to the printer's ppd file
 */
static int pt_utils_regist_printer(char *printer_name, char *scheme, char *ip_address, char *ppd_file)
{
	PT_RETV_IF(printer_name == NULL || ip_address == NULL || ppd_file == NULL , 1 , "Invalid argument");

	/* Connection to server */
	http_t *http = NULL;
	char device_url[PT_MAX_LENGTH] = {0,};

	snprintf(device_url, MAX_URI_SIZE, "%s://%s", scheme, ip_address);

	http = httpConnectEncrypt(cupsServer(), ippPort(), cupsEncryption());
	PT_RETV_IF(http == NULL, 1, "Unable to connect to server");

	if (__enable_printer(http, printer_name, device_url)) {
		PT_DEBUG("enable return 1");
		return (1);
	}

	if (__set_printer_ppd(http, printer_name, ppd_file)) {
		return (1);
	}

	if (http) {
		httpClose(http);
	}
	PT_DEBUG("add printer success");
	return (0);
}

/**
 *	This API let the app get the printer list
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] callback the pointer to the function which will be excuted after search
 *	@param[in] userdata the pointer to the user data
 */
int pt_get_printers(get_printers_cb callback, void *userdata)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info == NULL || g_pt_info->search == NULL, PT_ERR_INVALID_PARAM, "Invalid argument");
	PT_RETV_IF(callback == NULL || userdata == NULL, PT_ERR_INVALID_PARAM, "Invalid argument");

	/*check the current connection*/
	int status = -1;
	int connect_type = 0;
	status = pt_get_connection_status(&connect_type);
	PT_RETV_IF(status != PT_ERR_NONE, PT_ERR_FAIL, "Vconf access error");
	PT_RETV_IF(connect_type == 0, PT_ERR_FAIL, "No available connection");

	pt_search_data_t *ad = g_pt_info->search;
	ad->response_data.userdata = userdata;
	ad->user_cb = callback;

	PT_RETV_IF(ad->is_searching == 1 , PT_ERR_UNKNOWN, "in searching");

	/*clear previous search result*/
	if (ad->pt_local_list != NULL) {
		pt_utils_free_local_printer_list(ad->pt_local_list);
		ad->pt_local_list = NULL;
	}

	/*clear previous search result*/
	if (ad->response_data.printerlist != NULL) {
		pt_utils_free_search_list(ad->response_data.printerlist);
		ad->response_data.printerlist = NULL;
	}

	g_pt_info->searching_state = PT_SEARCH_IN_PROGRESS;
	if (g_pt_info->cups_pid > 0) {
		__pt_get_printers(ad);
	} else {
		g_pt_info->cups_checking_timer = ecore_timer_add(1.0, (Ecore_Task_Cb)__pt_check_cups_before_searching, ad);
	}

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}

/**
 *	This API let the app cancel getting the printer list
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 */
int pt_cancel_get_printers()
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info == NULL, PT_ERR_INVALID_PARAM, "g_pt_info is NULL");
	PT_RETV_IF(g_pt_info->search == NULL, PT_ERR_INVALID_PARAM, "g_pt_info->search is NULL");
	PT_RETV_IF(g_pt_info->searching_thd_hdl == NULL, PT_ERR_INVALID_PARAM, "g_pt_info->searching_thd_hdl is NULL");

	pt_search_data_t *ad = g_pt_info->search;

	if (g_pt_info->cups_checking_timer) {
		ecore_timer_del(g_pt_info->cups_checking_timer);
		g_pt_info->cups_checking_timer = NULL;
	}

	if (ecore_thread_cancel(g_pt_info->searching_thd_hdl) == EINA_FALSE) {
		PT_DEBUG("Canceling of searching_thd_hdl[%p] is pended", g_pt_info->searching_thd_hdl);
	}

	g_pt_info->searching_state = PT_SEARCH_CANCEL;

	/*clear the search result*/
	if (ad->pt_local_list != NULL) {
		pt_utils_free_local_printer_list(ad->pt_local_list);
		ad->pt_local_list = NULL;
	}

	/*clear the search result*/
	if (ad->response_data.printerlist != NULL) {
		pt_utils_free_search_list(ad->response_data.printerlist);
		ad->response_data.printerlist = NULL;
	}

	ad->response_data.userdata = NULL;
	ad->user_cb = NULL;

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}

/**
 *	This API let the app get the current default printer
 *	allocates memory for printer info structure! Please free after use!
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[out] printer the pointer to the printer object
 */
int pt_get_default_printer(pt_printer_mgr_t **printer)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(printer == NULL || g_pt_info == NULL, PT_ERR_INVALID_PARAM, "Invalid argument");
	pt_printer_mgr_t *pt = NULL;

	/* check whether if the default printer exists in the preference */
	bool isexist = 0;
	int ret = -1;
	ret = preference_is_existing(PREFERENCE_DEFAULT_PRINTER_NAME, &isexist);
	PT_RETV_IF(!isexist, PT_ERR_FAIL, "the default printer name isn't exist in preference!");

	ret = preference_is_existing(PREFERENCE_DEFAULT_PRINTER_ADDRESS, &isexist);
	PT_RETV_IF(!isexist, PT_ERR_FAIL, "the default printer name isn't exist in preference!");

	/* get the printer in the preference */
	char *name = NULL;
	ret = preference_get_string(PREFERENCE_DEFAULT_PRINTER_NAME, &name);
	PT_RETV_IF(ret, PT_ERR_FAIL, "get the default printer name failed, errno: %d!", ret);

	char *address = NULL;
	ret = preference_get_string(PREFERENCE_DEFAULT_PRINTER_ADDRESS, &address);
	PT_RETV_IF(ret, PT_ERR_FAIL, "get the default printer name failed, errno: %d!", ret);

	pt = (pt_printer_mgr_t *)calloc(1, sizeof(pt_printer_mgr_t));
	PT_RETV_IF(pt == NULL, PT_ERR_FAIL, "Failed to calloc pt");
	memcpy(pt->name, name, PT_MAX_LENGTH);
	memcpy(pt->address, address, PT_MAX_LENGTH);

	memcpy(g_pt_info->active_printer, pt, sizeof(pt_printer_mgr_t));

	PT_DEBUG("get printer info from preference, name: %s, address: %s!",
			 pt->name, pt->address);

	PT_IF_FREE_MEM(name);
	PT_IF_FREE_MEM(address);

	*printer = pt;

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}

/**
 *	This API let the app set a printer as the default printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] printer the pointer to the printer object
 */
int pt_set_default_printer(pt_printer_mgr_t *printer)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(printer == NULL || g_pt_info == NULL, PT_ERR_INVALID_PARAM, "Invalid argument");

	/*check the current connection*/
	int status = -1;
	int connect_type = 0;
	status = pt_get_connection_status(&connect_type);

	PT_RETV_IF(status != PT_ERR_NONE, PT_ERR_FAIL, "Vconf access error");
	PT_RETV_IF(connect_type == 0, PT_ERR_FAIL, "No available connection");

	/* set the printer name in the preference */
	int ret = -1;
	ret = preference_set_string(PREFERENCE_DEFAULT_PRINTER_NAME, (const char *)printer->name);
	PT_RETV_IF(ret, PT_ERR_FAIL, "set the default printer name(%s) failed!", printer->name);

	/* set the printer address in the preference */
	ret = preference_set_string(PREFERENCE_DEFAULT_PRINTER_ADDRESS, (const char *)printer->address);
	PT_RETV_IF(ret, PT_ERR_FAIL, "set the default printer name(%s) failed!", printer->address);

	/* set the printer address in the preference */
	ret = preference_set_string(PREFERENCE_DEFAULT_PRINTER_MFG, (const char *)printer->mfg);
	PT_RETV_IF(ret, PT_ERR_FAIL, "set the default printer name(%s) failed!", printer->mfg);

	/* set the printer address in the preference */
	ret = preference_set_string(PREFERENCE_DEFAULT_PRINTER_MDL, (const char *)printer->mdl);
	PT_RETV_IF(ret, PT_ERR_FAIL, "set the default printer name(%s) failed!", printer->mdl);

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}

/**
 *	This API let the app get the current active printer
 *	allocates memory for printer info structure! Please free after use!
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[out] printer the pointer to the printer object
 */
int pt_get_active_printer(pt_printer_mgr_t **printer)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info == NULL || g_pt_info->active_printer == NULL
			   , PT_ERR_INVALID_PARAM, "global printer information is NULL or no active printer");
	PT_RETV_IF(printer == NULL, PT_ERR_INVALID_PARAM, "printer is NULL");

	pt_printer_mgr_t *pt = NULL;
	pt = (pt_printer_mgr_t *)malloc(sizeof(pt_printer_mgr_t));
	PT_RETV_IF(pt == NULL, PT_ERR_NO_MEMORY, "Not enough memory");

	memset(pt, 0, sizeof(pt_printer_mgr_t));
	memcpy(pt, g_pt_info->active_printer, sizeof(pt_printer_mgr_t));

	PT_DEBUG("g_pt_info->active_printer->name %s" , g_pt_info->active_printer->name);
	PT_DEBUG("g_pt_info->active_printer->actived = %d", g_pt_info->active_printer->actived);
	PT_DEBUG("g_pt_info->active_printer->ppd%s" , g_pt_info->active_printer->ppd);
	PT_DEBUG("g_pt_info->active_printer->address %s" , g_pt_info->active_printer->address);
	PT_DEBUG("g_pt_info->active_printer->mfg %s" , g_pt_info->active_printer->mfg);
	PT_DEBUG("g_pt_info->active_printer->mdl %s" , g_pt_info->active_printer->mdl);

	*printer = pt;

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}


/**
 *	This API let the app select a specify printer as active printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] printer the pointer to the printer object
 */
int pt_set_active_printer(pt_printer_mgr_t *printer)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info == NULL || g_pt_info->active_printer == NULL
			   , PT_ERR_INVALID_PARAM, "global printer information is NULL or no active printer");
	PT_RETV_IF(printer == NULL, PT_ERR_INVALID_PARAM, "printer is NULL");

	/*check the current connection*/
	int status = -1;
	int connect_type = 0;
	status = pt_get_connection_status(&connect_type);

	PT_RETV_IF(status != PT_ERR_NONE, PT_ERR_FAIL, "Vconf access error");
	PT_RETV_IF(connect_type == 0, PT_ERR_FAIL, "No available connection");

	/*register the printer*/
	int ret = -1;

	if (pt_get_printer_ppd(printer) != PT_ERR_NONE) {
		/* Error can not get ppd info*/
		PT_DEBUG("Get %s ppd failed", printer->name);
		return PT_ERR_INVALID_PARAM;
	}

	PT_DEBUG("Get ppd info: %s", printer->ppd);
	PT_DEBUG("address is %s", printer->address);

	if ((strncasecmp(printer->address, "usb://", 6) == 0)) {
		/* usb mode */
		char *address = printer->address + sizeof("usb://") - 1;
		PT_DEBUG("usb address: %s", address);

		if (address) {
			ret = pt_utils_regist_printer(printer->name, "usb", address, printer->ppd);
		}
	} else if ((strncasecmp(printer->address, "dnssd://", 8) == 0)) {
		/* dnssd mode */
		char *address = printer->address + sizeof("dnssd://") - 1;
		PT_DEBUG("dnssd address: %s", address);

		if (address) {
			PT_DEBUG("1. printer structure address: %x", printer);
			ret = pt_utils_regist_printer(printer->name, "dnssd", address, printer->ppd);
			PT_DEBUG("2. printer structure address: %x", printer);
		}
	} else {
		ret = pt_utils_regist_printer(printer->name, "socket", printer->address, printer->ppd);
	}

	PT_RETV_IF(ret != PT_ERR_NONE, PT_ERR_UNKNOWN, "add printer failed, description:%s", cupsLastErrorString());

	/* only actived printer can print file */
	printer->actived = 1;
	memcpy(g_pt_info->active_printer, printer, sizeof(pt_printer_mgr_t));
	PT_DEBUG("g_pt_info->active_printer->name %s" , g_pt_info->active_printer->name);
	PT_DEBUG("g_pt_info->active_printer->address %s" , g_pt_info->active_printer->address);
	PT_DEBUG("g_pt_info->active_printer->mfg %s" , g_pt_info->active_printer->mfg);
	PT_DEBUG("g_pt_info->active_printer->mdl %s" , g_pt_info->active_printer->mdl);

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}
