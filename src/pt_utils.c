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

#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>
#include <cups/ppd.h>
#include <cups/http.h>

#include "pt_debug.h"
#include "pt_common.h"

static ppd_file_t *ppd;

int __standardization(char *name)
{
	char *ptr = NULL;/* Pointer into name */
	PT_RETV_IF(name == NULL, -1, "Invalid argument");

	for (ptr = name; *ptr; ptr++) {
		if (*ptr == '@') {
			return 0;
		} else if ((*ptr >= 0 && *ptr < ' ') ||
				   *ptr == 127 || *ptr == '/' ||*ptr == '#') {
			return -1;
		} else if (*ptr == 0x20) {
			*ptr = 0x5F; /*convert space to _*/
		} else if (*ptr == '[' || *ptr == '(') {
			*ptr='\0';
			while (name <= ptr) {
				ptr--;
				if (*ptr==0x5F) {
					*ptr='\0';
				} else {
					return 0;
				}
			}
			return 0;
		}
	}
	return 0;
}

/**
 *	This function let the app clear the list of previous searching
 *	@return    void
 *	@param[in] list the pointer to the printers list
 */
void pt_utils_free_local_printer_list(Eina_List *list)
{
	PT_RET_IF(list == NULL, "Invalid argument");
	pt_printer_info_t *detail;

	EINA_LIST_FREE(list, detail) {
		PT_IF_FREE_MEM(detail);
	}

	eina_list_free(list);
}

/**
 *	This function let the app clear the list of previous searching
 *	@return    void
 *	@param[in] list the pointer to the printers list
 */
void pt_utils_free_search_list(Eina_List *list)
{
	PT_RET_IF(list == NULL, "Invalid argument");
	pt_printer_mgr_t *detail;

	EINA_LIST_FREE(list, detail) {
		PT_IF_FREE_MEM(detail);
	}
	eina_list_free(list);
}

void pt_utils_free_printing_thd_list(Eina_List *list)
{
	PT_RET_IF(list == NULL, "Invalid argument");
	pt_printing_job_t *detail;

	EINA_LIST_FREE(list, detail) {
		if (detail) {

			if (detail->job_noti_timer) {
				ecore_timer_del(detail->job_noti_timer);
				detail->job_noti_timer = NULL;
			}
		}
	}
}

/******************************print***********************************/

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
#ifdef CUPS_161_UPGRADE
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
#else
	if ((response = cupsDoRequest(http, request, "/admin/")) == NULL) {
		PT_DEBUG("cupsDoRequest Failed");
		return (1);
	} else if (response->request.status.status_code > IPP_OK_EVENTS_COMPLETE) {
		PT_DEBUG("%x greater than IPP_OK_EVENTS_COMPLETE -- Failed", response->request.status.status_code);
		ippDelete(response);
		return (1);
	} else {
		ippDelete(response);
		return (0);
	}
#endif
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
int pt_utils_regist_printer(char *printer_name, char *scheme, char *ip_address, char *ppd_file)
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

char *pt_utils_filename_from_URI(const char *uri)
{
	char			scheme[1024];	/* URI for printer/class */
	char			username[1024];		/* Line from PPD file */
	char			hostname[1024];		/* Keyword from Default line */
	int             port;		/* Pointer into keyword... */
	char			resource[1024];	/* Temporary filename */
	char           *filename = NULL;

	http_uri_status_t status = httpSeparateURI(HTTP_URI_CODING_ALL, uri, scheme, sizeof(scheme),
							   username, sizeof(username), hostname, sizeof(hostname), &port,
							   resource, sizeof(resource));
	PT_DEBUG("uri:[%s]", uri);
	PT_DEBUG("scheme: [%s]", scheme);
	PT_DEBUG("resource: [%s]", resource);
	PT_DEBUG("status: [%d], HTTP_URI_OK = %d", status, HTTP_URI_OK);
	if ((status == HTTP_URI_OK || status == HTTP_URI_MISSING_SCHEME)
			&& (strlen(resource) != 0) && (strcmp(scheme, "file") == 0)) {
		filename = strdup(resource);
	}
	return filename;
}

static char *pt_utils_rtrim(char *s)
{
	static char t[1024];
	char *end;

	memset(t,0x00,sizeof(t));
	strncpy(t, s, sizeof(t) - 1);
	end = t + strlen(t) - 1;
	while (end != t && isspace(*end)) {
		end--;
	}
	*(end + 1) = '\0';
	s = t;

	return s;
}

static char *pt_utils_ltrim(char *s)
{
	char *begin;
	begin = s;

	while (*begin != '\0') {
		if (isspace(*begin)) {
			begin++;
		} else {
			s = begin;
			break;
		}
	}

	return s;
}

static char *pt_utils_trim_space(char *string)
{
	return pt_utils_rtrim(pt_utils_ltrim(string));
}

int pt_utils_get_mfg_mdl(const char *printer, char **mfg, char **mdl)
{
	PT_RETV_IF(printer == NULL || mfg == NULL || mdl == NULL , PT_ERR_FAIL, "Invalid argument");

	const char *vendor[3] = {"Samsung", "HP", "Epson"};
	int 	index 	= 0;

	if (strcasestr(printer, vendor[0]) != 0) {
		index = 0;
	} else if (strcasestr(printer, vendor[1]) != 0) {
		index = 1;
	} else if (strcasestr(printer, vendor[2]) != 0) {
		index = 2;
	} else {
		PT_DEBUG("Unsupported printer %s", printer);
		return PT_ERR_UNSUPPORTED;
	}

	*mfg = strdup(vendor[index]);
	PT_RETV_IF(*mfg == NULL, PT_ERR_NO_MEMORY, "Not enough memory");

	// FIXME
	char *temp1 = NULL;
	if (strcasestr(printer, "Hewlett-Packard") != NULL) {
		temp1 = strdup(printer + strlen("Hewlett-Packard ") + strlen(vendor[index]));
	} else {
		temp1 = strdup(printer+strlen(vendor[index]));
	}
	char *temp2 = pt_utils_trim_space(temp1);
	char *temp3 = strdup(temp2);
	PT_IF_FREE_MEM(temp1);

	// It can include Manufacturer twice in printer name.
	if (strncasecmp(temp3, vendor[index], strlen(vendor[index])) == 0) {
		temp1 = strdup(temp3 + strlen(vendor[index]));
		temp2 = pt_utils_trim_space(temp1);
		temp3 = strdup(temp2);
		PT_IF_FREE_MEM(temp1);
	}

	*mdl = temp3;
	return PT_ERR_NONE;
}

ppd_size_t *pt_utils_paper_size_pts(const char *name)
{
	return ppdPageSize(ppd, name);
}
