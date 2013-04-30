/*
*  Printservice
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

#define _GNU_SOURCE
#include <string.h>
#include <math.h>
#include <cups/ipp.h>
#include <cups/ppd.h>
#include <glib/gprintf.h>
#include <vconf.h>
#include <vconf-keys.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <app.h>

#include "pt_debug.h"
#include "pt_common.h"
#include "pt_ppd.h"
#include "pt_utils.h"


#define AVAHI_DAEMON "/usr/sbin/avahi-daemon"
#define CUPS_DAEMON  "/usr/sbin/cupsd"

#define AVAHI_TEMP			"/opt/var/run/avahi-daemon/pid"
#define CLEAR_JOB_TEMP 		"rm -rf /opt/var/spool/cups/* /opt/var/run/cups/*;killall cupsd"

//#define JOB_IN_PROGRESS "Spooling job, "
#define JOB_COMPLETE "Job completed."
//#define JOB_PAGE_PRINTED "Printed "

pt_info_t *g_pt_info = NULL;

static Eina_Bool __pt_parse_job_complete(const char *msg)
{
	PRINT_SERVICE_FUNC_ENTER;
	gboolean ret = EINA_FALSE;

	if (strstr(msg, JOB_COMPLETE) != NULL) {
		ret = EINA_TRUE;
	}

	PRINT_SERVICE_FUNC_LEAVE;
	return ret;
}

static Eina_Bool __pt_create_job_subscription(int jobid)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(jobid <= 0 , EINA_FALSE, "Invalid jobid[%d]", jobid);

	char uri[HTTP_MAX_URI];
	const char *events[100];
	int num_events;
	Eina_List *cursor = NULL;
	pt_printing_job_t *printing_job = NULL;

	EINA_LIST_FOREACH(g_pt_info->printing_thd_list, cursor, printing_job) {
		if (printing_job->job->id == jobid) {
			PT_DEBUG("Found printing_job for job[%d]", jobid);
			break;
		}
	}
	PT_RETV_IF(printing_job == NULL || printing_job->job->id <= 0, EINA_FALSE
			   , "No found printing_job for job[%d]", jobid);

	http_t *http;
	ipp_t  *request, *response;
	ipp_attribute_t *attr;

	http = httpConnectEncrypt(cupsServer(), ippPort(), cupsEncryption());
	request = ippNewRequest(IPP_CREATE_JOB_SUBSCRIPTION);

	httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL, "localhost", 631, "/printers/%s", g_pt_info->active_printer->name);
	PT_DEBUG("request print-uri: %s\n", uri);
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);

	PT_DEBUG("request requesting-user-name: %s\n", cupsUser());
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsUser());

	events[0] = "all";
	num_events = 1;
	ippAddStrings(request, IPP_TAG_SUBSCRIPTION, IPP_TAG_KEYWORD, "notify-events",
				  num_events, NULL, events);
	ippAddString(request, IPP_TAG_SUBSCRIPTION, IPP_TAG_KEYWORD,
				 "notify-pull-method", NULL, "ippget");

	if ((response = cupsDoRequest(http, request, "/")) != NULL) {
		PT_DEBUG("request ok!");

		if ((attr = ippFindAttribute(response, "notify-subscription-id", IPP_TAG_INTEGER)) != NULL) {
			printing_job->job->subscription_id = ippGetInteger(attr,0);
			printing_job->job->sequence_num = 0;
			PT_DEBUG("Subscription id[%d]\n", printing_job->job->subscription_id);
		} else {
			/*
			 * If the printer does not return a job-state attribute, it does not
			 * conform to the IPP specification - break out immediately and fail
			 * the job...
			 */

			PT_DEBUG("No notify-subscription-id available from job-uri.\n");
		}

		ippDelete(response);
	}
	httpClose(http);

	PRINT_SERVICE_FUNC_LEAVE;
	return EINA_TRUE;
}

static Eina_Bool __pt_parse_page_progress_value(const char *msg, int *page, int *progress)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(msg == NULL || page == NULL || progress == NULL, EINA_FALSE, "Invalid argument");

	int ret = 0;
	ret = sscanf(msg, "Printing page %d, %d%%", page, progress);
	PT_RETV_IF(ret != 2, EINA_FALSE, "Failed to sscanf page, progress");
	PT_DEBUG("Parsed page: %d, progress:  %d", *page, *progress);

	PRINT_SERVICE_FUNC_LEAVE;
	return EINA_TRUE;
}

/*
* launch daemon, such as cups and avahi
*/
static Eina_Bool __pt_get_cupsd_pid(void *data)
{
	PRINT_SERVICE_FUNC_ENTER;
	FILE *fd = NULL;
	char *ret = NULL;
	char pid_str[9] = {0,};
	int pid = 0;

	fd = fopen("/opt/var/run/cups/cupsd.pid", "r");
	if (fd != NULL) {
		ret = fgets(pid_str, 8, fd);
		if (ret != NULL) {
			PT_DEBUG("ret : %s", ret);
			pid = atoi(pid_str);
			PT_DEBUG("CUPS process is %d", pid);
		}
		fclose(fd);
	}
	g_pt_info->cups_pid = pid;
	PRINT_SERVICE_FUNC_LEAVE;
	return ECORE_CALLBACK_CANCEL;
}

static void __pt_launch_daemon(void)
{
	PRINT_SERVICE_FUNC_ENTER;

	int pid = 0;
	int pid2 = 0;

	char *cups_argv[2];
	char *avahi_argv[6];

	cups_argv[0] = "cupsd";
	cups_argv[1] = NULL;

	avahi_argv[0] = "avahi-daemon";
	avahi_argv[1] = "-s";
	avahi_argv[2] = "--no-drop-root";
	avahi_argv[3] = "--no-chroot";
	avahi_argv[4] = "--debug";
	avahi_argv[5] = NULL;

	pt_utils_remove_files_in("/opt/var/spool/cups");
	pt_utils_remove_files_in("/opt/var/run/cups");

	/*
	when temp file content is NULL
	avahi daemon will not start
	delete this file can avoid this issue
	*/
	if (access(AVAHI_TEMP, F_OK) ==0) {
		if (unlink(AVAHI_TEMP) != 0) {
			PT_DEBUG("DEL_AVAHI_TEMP is failed(%d)", errno);
		}
	}

	pid = vfork();
	if (0 == pid) {
		PT_DEBUG("I'm avahi process %d", getpid());
		if (-1 == execv(AVAHI_DAEMON, avahi_argv)) {
			PT_DEBUG("AVAHI failed");
		}
	} else {
		PT_DEBUG("avahi process is %d", pid);
		g_pt_info->avahi_pid = pid;

		pid2 = vfork();
		if (0 == pid2) {
			PT_DEBUG("I'm CUPS process %d", getpid());
			if (-1 == execv(CUPS_DAEMON, cups_argv)) {
				PT_DEBUG("exec failed");
			}
		} else {
			ecore_timer_add(2.0, (Ecore_Task_Cb)__pt_get_cupsd_pid, NULL);
			PT_DEBUG("CUPS parent process is %d", pid2);
		}
	}
	PRINT_SERVICE_FUNC_LEAVE;
}

static gboolean __pt_process_hotplug_event(GIOChannel *source, GIOCondition condition, gpointer data)
{
	pt_info_t *info = (pt_info_t *)data;
	int read_len = -1;
	char buf[MAX_MSG_LEN] = {0,};

	memset(buf, '\0', MAX_MSG_LEN);
	read_len = recv(info->ueventsocket, buf, sizeof(buf)-1, 0);

	if (read_len == -1) {
		return FALSE;
	}

	if (strstr(buf, "/usb/lp0")) {
		pt_event_e event;
		if (strstr(buf, "add@")) {
			event = PT_EVENT_USB_PRINTER_ONLINE;
		} else if (strstr(buf, "remove@")) {
			event = PT_EVENT_USB_PRINTER_OFFLINE;
		} else {
			PT_DEBUG("Unknown usb event!!! So just think offline");
			event = PT_EVENT_USB_PRINTER_OFFLINE;
		}

		if (info->evt_cb != NULL) {
			info->evt_cb(event, info->user_data, NULL);
		} else {
			PT_DEBUG("Failed to send hotplug event because of info->evt_cb is NULL");
			return FALSE;
		}
	} else {
		return FALSE;
	}

	return TRUE;
}

static gboolean __pt_register_hotplug_event(void *data)
{
	pt_info_t *info = (pt_info_t *)data;
	PT_RETV_IF(info == NULL, FALSE, "Invalid argument");

	int ret = -1;
	int ueventsocket = -1;
	GIOChannel *gio = NULL;
	int g_source_id = -1;

	ueventsocket = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	PT_RETV_IF(ueventsocket == -1 , FALSE, "create uevent socket failed. errno=%d", errno);

	struct sockaddr_nl addr;
	bzero(&addr, sizeof(struct sockaddr_nl));
	addr.nl_family = AF_NETLINK;
	addr.nl_groups = 1;
	addr.nl_pid = getpid();

	ret = bind(ueventsocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_nl));
	if (-1 == ret) {
		PT_DEBUG("bind uevent socket failed. errno=%d", errno);
		close(ueventsocket);
		return FALSE;
	}

	const int buffersize = 1024;
	ret = setsockopt(ueventsocket, SOL_SOCKET, SO_RCVBUF, &buffersize, sizeof(buffersize));
	if (-1 == ret) {
		PT_DEBUG("set uevent socket option failed. errno=%d", errno);
		close(ueventsocket);
		return FALSE;
	}

	info->ueventsocket = ueventsocket;
	gio = g_io_channel_unix_new(ueventsocket);
	g_source_id = g_io_add_watch(gio, G_IO_IN | G_IO_ERR | G_IO_HUP, (GIOFunc)__pt_process_hotplug_event, info);
	g_io_channel_unref(gio);
	PT_DEBUG("Socket is successfully registered to g_main_loop.\n");

	return TRUE;
}

static void printing_thread_notify_cb(void *data, Ecore_Thread *thread, void *msg_data)
{
	PRINT_SERVICE_FUNC_ENTER;

	PT_DEBUG("Thread is sent msg successfully.");

	ipp_t *response;
	ipp_attribute_t *attr;
	pt_progress_info_t progress_info = {0,};
	pt_printing_job_t *printing_job = NULL;

	if (data != NULL) {
		printing_job = (pt_printing_job_t *)data;
	} else {
		PT_DEBUG("data is NULL.");
		// FIXME - no need to call evt_cb?
		return;
	}

	if (msg_data != NULL) {
		response = (ipp_t *)msg_data;
	} else {
		PT_DEBUG("msg_data is NULL.");
		// FIXME - no need to call evt_cb?
		return;
	}

	PT_DEBUG("Received notification of JOB ID %d", printing_job->job->id);
	progress_info.job_id = printing_job->job->id;
	printing_job->page_printed = 0;

	// Get progress value by parsing notify-text
	if ((attr = ippFindAttribute(response, "notify-text", IPP_TAG_TEXT)) != NULL) {
		int progress;
		Eina_Bool job_completed;
		Eina_Bool ret;

		PT_DEBUG("notify-text : %s", ippGetString(attr, 0, NULL));

		ret = __pt_parse_page_progress_value(ippGetString(attr, 0, NULL),
											 &printing_job->page_printed,
											 &progress);

		if ((progress > 0) && (NULL != g_pt_info->evt_cb)) {
			progress_info.progress = progress;
			progress_info.page_printed = printing_job->page_printed;
			g_pt_info->evt_cb(PT_EVENT_JOB_PROGRESS, g_pt_info->user_data, &progress_info);
		}

		job_completed = __pt_parse_job_complete(ippGetString(attr, 0, NULL));
		if (job_completed == EINA_TRUE) {
			PT_DEBUG("Job completed. Stop timer callback to get notification");
			printing_job->printing_state = PT_PRINT_END;
			if (NULL != g_pt_info->evt_cb) {
				g_pt_info->evt_cb(PT_EVENT_JOB_COMPLETED, g_pt_info->user_data, &progress_info);
			}
		}

		while (attr) {
			attr = ippFindNextAttribute(response, "notify-text", IPP_TAG_TEXT);
			if (attr) {
				PT_DEBUG("notify-text : %s", ippGetString(attr, 0, NULL));
				ret = __pt_parse_page_progress_value(ippGetString(attr, 0, NULL),
													 &printing_job->page_printed,
													 &progress);

				if ((progress > 0) && (NULL != g_pt_info->evt_cb)) {
					progress_info.progress = progress;
					progress_info.page_printed = printing_job->page_printed;
					g_pt_info->evt_cb(PT_EVENT_JOB_PROGRESS, g_pt_info->user_data, &progress_info);
				}

				job_completed = __pt_parse_job_complete(ippGetString(attr, 0, NULL));
				if (job_completed == EINA_TRUE) {
					PT_DEBUG("Job completed. Stop timer callback to get notification");
					printing_job->printing_state = PT_PRINT_END;
					if (NULL != g_pt_info->evt_cb) {
						g_pt_info->evt_cb(PT_EVENT_JOB_COMPLETED, g_pt_info->user_data, &progress_info);
					}
				}
			}
		}

		for (attr = ippFindAttribute(response, "notify-sequence-number", IPP_TAG_INTEGER);
				attr;
				attr = ippFindNextAttribute(response, "notify-sequence-number", IPP_TAG_INTEGER))
			if (ippGetInteger(attr, 0) > printing_job->job->sequence_num) {
				printing_job->job->sequence_num = ippGetInteger(attr, 0);
			}

		ippDelete(response);
	}

	PRINT_SERVICE_FUNC_LEAVE;
}

static void printing_thread_end_cb(void *data, Ecore_Thread *thread)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RET_IF(data == NULL || thread == NULL, "Invalid argument");
	PT_DEBUG("Thread is completed successfully.");

	pt_printing_job_t *printing_job = (pt_printing_job_t *)data;

	Eina_List *printing_thd_list = NULL;
	pt_printing_job_t *printing_job_item = NULL;
	EINA_LIST_FOREACH(g_pt_info->printing_thd_list, printing_thd_list, printing_job_item) {
		PT_RET_IF(printing_job_item->job == NULL, "printing_job_item->job is NULL");
		if (printing_job_item->job->id == printing_job->job->id) {
			PT_DEBUG("Found printing_thd_list for job[%d]", printing_job_item->job->id);
			break;
		}
	}

	PT_RET_IF(printing_job == NULL || printing_job->job->id <= 0
			  , "No found printing_job for job[%d]", printing_job->job->id);

	g_pt_info->printing_thd_list = eina_list_remove_list(g_pt_info->printing_thd_list, printing_thd_list);

	PT_IF_FREE_MEM(printing_job->job);
	PT_IF_FREE_MEM(printing_job);

	if (eina_list_count(g_pt_info->printing_thd_list) == 0) {
		if (g_pt_info->evt_cb != NULL) {
			g_pt_info->evt_cb(PT_EVENT_ALL_THREAD_COMPLETED, g_pt_info->user_data, NULL);
		}
	}

	PRINT_SERVICE_FUNC_LEAVE;
}

static void printing_thread_cancel_cb(void *data, Ecore_Thread *thread)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RET_IF(data == NULL || thread == NULL, "Invalid argument");
	PT_DEBUG("Thread is canceled successfully.");

	pt_printing_job_t *printing_job = (pt_printing_job_t *)data;

	Eina_List *printing_thd_list = NULL;
	pt_printing_job_t *printing_job_item = NULL;
	EINA_LIST_FOREACH(g_pt_info->printing_thd_list, printing_thd_list, printing_job_item) {
		PT_RET_IF(printing_job_item->job == NULL, "printing_job_item->job is NULL");
		if (printing_job_item->job->id == printing_job->job->id) {
			PT_DEBUG("Found printing_thd_list for job[%d]", printing_job_item->job->id);
			break;
		}
	}
	PT_RET_IF(printing_job == NULL || printing_job->job->id <= 0
			  , "No found printing_job for job[%d]", printing_job->job->id);

	g_pt_info->printing_thd_list = eina_list_remove_list(g_pt_info->printing_thd_list, printing_thd_list);

	PT_IF_FREE_MEM(printing_job->job);
	PT_IF_FREE_MEM(printing_job);

	if (eina_list_count(g_pt_info->printing_thd_list) == 0) {
		if (g_pt_info->evt_cb != NULL) {
			g_pt_info->evt_cb(PT_EVENT_ALL_THREAD_COMPLETED, g_pt_info->user_data, NULL);
		}
	}

	PRINT_SERVICE_FUNC_LEAVE;
}

static void printing_thread(void *data, Ecore_Thread *thread)
{
	PRINT_SERVICE_FUNC_ENTER;
	pt_printing_job_t *printing_job = (pt_printing_job_t *)data;
	//int *jobid = (int *)data;
	char uri[HTTP_MAX_URI] = {0,};
	http_t *http;
	ipp_t  *request, *response;
	pt_progress_info_t progress_info = {0,};

	PT_DEBUG("print_file %s",printing_job->files[0]);

	/*start printing file*/
	printing_job->job->id = cupsPrintFiles(g_pt_info->active_printer->name, printing_job->num_files, printing_job->files, NULL,
										   g_pt_info->num_options, g_pt_info->job_options);

	cups_option_t *option = g_pt_info->job_options;
	int i;
	PT_DEBUG("++++++++++++ PRINTING ++++++++++++\n");
	for (i=0; i < g_pt_info->num_options; i++) {
		PT_DEBUG("%d. %s=%s", i, option[i].name, option[i].value);
	}

	PT_RET_IF(printing_job->job->id <= 0, "print file failed, description:%s", cupsLastErrorString());

	if (NULL != g_pt_info->evt_cb) {
		progress_info.job_id = printing_job->job->id;
		g_pt_info->evt_cb(PT_EVENT_JOB_STARTED, g_pt_info->user_data, &progress_info);
	}

	__pt_create_job_subscription(printing_job->job->id);
	//__pt_register_job_progress(printing_job);

	while (printing_job->printing_state == PT_PRINT_IN_PROGRESS || printing_job->printing_state == PT_PRINT_PENDING) {
		if (printing_job->printing_state == PT_PRINT_IN_PROGRESS) {
			http = httpConnectEncrypt(cupsServer(), ippPort(), cupsEncryption());
			request = ippNewRequest(IPP_GET_NOTIFICATIONS);

			httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL, "localhost", 631, "/jobs/%d", printing_job->job->id);
			PT_DEBUG("request job-uri: %s\n", uri);
			ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "job-uri", NULL, uri);

			ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
						 "requesting-user-name", NULL, cupsUser());

			ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER,
						  "notify-subscription-ids", printing_job->job->subscription_id);
			if (printing_job->job->sequence_num)
				ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER,
							  "notify-sequence-numbers", printing_job->job->sequence_num + 1);

			if ((response = cupsDoRequest(http, request, "/")) != NULL) {
				PT_DEBUG("request ok!");

				if (ecore_thread_feedback(thread, (const void *)response) == EINA_FALSE) {
					PT_DEBUG("Failed to send data to main loop");
					ippDelete(response);
				}
			}
			httpClose(http);
		}

		sleep(3);
	}

	PRINT_SERVICE_FUNC_LEAVE;
}

/**
 *	This API let the app start the print job on the active printer, if not find active printer, will use default printer
 *	now only support image and txt file,
 *	will support pdf,office,eml,html in the future
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] files is the print files path address
 *	@param[in] num is the print files number
 */
int pt_start_print(const char **files, int num)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info == NULL || g_pt_info->active_printer == NULL
			   , PT_ERR_INVALID_PARAM, "Invalid argument");

	PT_RETV_IF(files == NULL || num <=0, PT_ERR_INVALID_PARAM
			   , "Params are not valid (num=%d)", num);

	/* Check that connection is available */
	int conn_type = 0;
	int ret = 0;

	pt_save_user_choice();

	ret = pt_get_connection_status(&conn_type);
	if (strcasestr(g_pt_info->active_printer->address,"usb://") != NULL) {
		if ((conn_type&PT_CONNECTION_USB) == 0) {
			PT_ERROR("USB printer is selected. But, USB is not available.");
			PRINT_SERVICE_FUNC_LEAVE;
			return PT_ERR_NOT_USB_ACCESS;
		}
	} else {
		if ((conn_type&PT_CONNECTION_WIFI) == 0 &&
				(conn_type&PT_CONNECTION_WIFI_DIRECT) == 0) {
			PT_ERROR("Network printer is selected. But, Network is not available.");
			PRINT_SERVICE_FUNC_LEAVE;
			return PT_ERR_NOT_NETWORK_ACCESS;
		}
	}

	/*get print options*/
	char *instance = NULL;		 /* Instance name */
	cups_dest_t *dest = NULL;	 /* Current destination */

	if ((instance = strrchr(g_pt_info->active_printer->name, '/')) != NULL) {
		*instance++ = '\0';
	}

	dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT,
							g_pt_info->active_printer->name,
							instance);
	PT_RETV_IF(dest == NULL, PT_ERR_UNKNOWN, "get options failed");

	pt_printing_job_t *printing_job = malloc(sizeof(pt_printing_job_t));
	PT_RETV_IF(printing_job == NULL, PT_ERR_NO_MEMORY, "Failed to allocate memory for printing_job");

	memset(printing_job, 0, sizeof(pt_printing_job_t));

	printing_job->job = (pt_job_info_t *)calloc(1, sizeof(pt_job_info_t));
	if (NULL == printing_job->job) {
		PT_IF_FREE_MEM(printing_job);
		PRINT_SERVICE_FUNC_LEAVE;
		return PT_ERR_NO_MEMORY;
	}
	memset(printing_job->job, 0, sizeof(pt_job_info_t));

	printing_job->printing_state = PT_PRINT_IN_PROGRESS;
	printing_job->files = files;
	printing_job->num_files = num;
	printing_job->printing_thd_hdl = ecore_thread_feedback_run(
										 printing_thread,
										 printing_thread_notify_cb,
										 printing_thread_end_cb,
										 printing_thread_cancel_cb,
										 printing_job,
										 EINA_FALSE);

	g_pt_info->printing_thd_list = eina_list_append(g_pt_info->printing_thd_list, printing_job);
	if (eina_error_get()) {
		PT_DEBUG("Failed to add eina_list for printing_thd_list");

		if (ecore_thread_cancel(printing_job->printing_thd_hdl) == EINA_FALSE) {
			PT_DEBUG("Canceling of printing_thd_hdl[%p] is pended", printing_job->printing_thd_hdl);
		}

		PT_IF_FREE_MEM(printing_job->job);
		PT_IF_FREE_MEM(printing_job);
		PRINT_SERVICE_FUNC_LEAVE;
		return PT_ERR_NO_MEMORY;
	}

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}

/**
 *	This API let the app cancel the print job by jobid
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] jobid the job id
 */
int pt_cancel_print(int jobid)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info == NULL || g_pt_info->active_printer == NULL
			   , PT_ERR_INVALID_PARAM, "Invalid argument");
	PT_RETV_IF(jobid <= 0, PT_ERR_INVALID_PARAM, "Invalid argument");

	Eina_List *cursor = NULL;
	pt_printing_job_t *printing_job = NULL;
	EINA_LIST_FOREACH(g_pt_info->printing_thd_list, cursor, printing_job) {
		if (printing_job->job->id == jobid) {
			PT_DEBUG("Found printing_thd_list for job[%d]", jobid);
			printing_job->printing_state = PT_PRINT_CANCEL;
			break;
		}
	}
	PT_RETV_IF(printing_job == NULL || printing_job->job->id <= 0
			   , PT_ERR_INVALID_PARAM, "No found printing_job for job[%d]", jobid);

	if (printing_job->job_noti_timer != NULL) {
		ecore_timer_del(printing_job->job_noti_timer);
		printing_job->job_noti_timer = NULL;
	}

	int ret = -1;
	/*cancel printing file*/
	ret = cupsCancelJob(g_pt_info->active_printer->name, jobid);
	PT_RETV_IF(ret == 0, PT_ERR_UNKNOWN, "cancel job failed, description:%s", cupsLastErrorString());

	if (ecore_thread_cancel(printing_job->printing_thd_hdl) == EINA_FALSE) {
		PT_DEBUG("Canceling of printing_thd_hdl[%p] is pended", printing_job->printing_thd_hdl);
	}

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}

/**
 *	This API let the app initialize the environment(eg.ip address) by type
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] type connection type enumerated in pt_connection_type
 */
int pt_init(pt_event_cb evt_cb, void *user_data)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info != NULL, PT_ERR_NONE, "Initialized before");
	PT_RETV_IF(evt_cb == NULL, PT_ERR_INVALID_USER_CB, "evt_cb is NULL");

	g_pt_info = (pt_info_t *)calloc(1, sizeof(pt_info_t));
	PT_RETV_IF(g_pt_info == NULL, PT_ERR_NO_MEMORY, "Failed to malloc");

	g_pt_info->evt_cb = evt_cb;
	g_pt_info->user_data = user_data;

	__pt_launch_daemon();

	g_pt_info->active_printer = (pt_printer_mgr_t *)calloc(1, sizeof(pt_printer_mgr_t));
	PT_RETV_IF(g_pt_info->active_printer == NULL, PT_ERR_NO_MEMORY, "Failed to malloc");

	g_pt_info->search = (pt_search_data_t *)calloc(1, sizeof(pt_search_data_t));
	PT_RETV_IF(g_pt_info->search == NULL, PT_ERR_NO_MEMORY, "Failed to malloc");

	g_pt_info->search->response_data.printerlist = NULL ;
	g_pt_info->search->is_searching = 0 ;
	g_pt_info->ueventsocket = -1;

	__pt_register_hotplug_event(g_pt_info);

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}

/**
 *	This API let the app finalize the environment
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 */
int pt_deinit(void)
{
	PRINT_SERVICE_FUNC_ENTER;

	if (g_pt_info) {
		if (g_pt_info->printing_thd_list) {
			pt_utils_free_printing_thd_list(g_pt_info->printing_thd_list);
			g_pt_info->printing_thd_list = NULL;
		}

		g_pt_info->ueventsocket = -1;

		g_pt_info->evt_cb = NULL;
		g_pt_info->user_data = NULL;

		kill(g_pt_info->cups_pid, SIGTERM);
		g_pt_info->cups_pid = -1;
		kill(g_pt_info->avahi_pid, SIGTERM);
		g_pt_info->avahi_pid = -1;

	}

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}

/**
 *	This API let the app get the connect status
 *	@return   If success, return PT_ERR_CONNECTION_USB_ACCESS or PT_ERR_CONNECTION_WIFI_ACCESS,
 *	else return the other error code as defined in pt_err_t
 */
int pt_get_connection_status(int *net_type)
{
	PRINT_SERVICE_FUNC_ENTER;
	int status = 0;
	int connection_type = 0;
	int ret = 0;

	ret = vconf_get_int(VCONFKEY_SYSMAN_USB_HOST_STATUS, &status);
	PT_RETV_IF(ret != 0, PT_ERR_FAIL, "Critical: Get VCONF usb status failed: %d", status);

	PT_DEBUG("usb status: %d", status);

	if (status == VCONFKEY_SYSMAN_USB_HOST_CONNECTED) {
		connection_type |= PT_CONNECTION_USB;
	}

	ret = vconf_get_int(VCONFKEY_WIFI_STATE, &status);
	PT_RETV_IF(ret != 0, 	PT_ERR_FAIL, "Critical: Get VCONF wifi status failed: %d", status);

	PT_DEBUG("wifi status: %d", status);

	if (status >= VCONFKEY_WIFI_CONNECTED) {
		connection_type |= PT_CONNECTION_WIFI;
	}

	ret = vconf_get_int(VCONFKEY_WIFI_DIRECT_STATE, &status);
	PT_RETV_IF(ret != 0, PT_ERR_FAIL, "Critical: Get VCONF wifi-direct status failed: %d", status);

	PT_DEBUG("wifi-direct status: %d", status);

	if (status >= VCONFKEY_WIFI_DIRECT_CONNECTED) {
		connection_type |= PT_CONNECTION_WIFI_DIRECT;
	}

	*net_type = connection_type;

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}

/**
 *	This API let the app get the job status
 *	@return   return job status
 *	@param[in] job id
 */
pt_job_state_e pt_get_current_job_status(int jobid)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info == NULL || g_pt_info->active_printer == NULL, PT_JOB_ERROR, "Invalid g_pt_info");
	PT_RETV_IF(jobid <= 0, PT_JOB_ERROR, "Invalid argument");

	pt_event_e event;
	pt_job_state_e ret 			= PT_JOB_ERROR;
	pt_progress_info_t progress_info = {0,};
	Eina_List *cursor 				= NULL;
	pt_printing_job_t *printing_job 	= NULL;

	EINA_LIST_FOREACH(g_pt_info->printing_thd_list, cursor, printing_job) {
		if (printing_job->job->id == jobid) {
			PT_DEBUG("Found printing_thd_list for job[%d]", jobid);
			break;
		}
	}

	PT_RETV_IF(printing_job == NULL || printing_job->job->id <= 0, PT_JOB_ERROR
			   , "No found printing_job for job[%d]", jobid);

	int num_job 		= 0;
	int i 				= 0;
	cups_job_t *jb 	= NULL;
	num_job = cupsGetJobs(&jb, g_pt_info->active_printer->name, 0, CUPS_WHICHJOBS_ALL);

	PT_DEBUG("print job id is %d, job num is %d", jobid, num_job);
	for (i = 0; i < num_job; i++) {
		PT_DEBUG("job id is %d, job state is %d", jb[i].id, jb[i].state);
		if (jb[i].id == jobid) {
			ret = jb[i].state;
			break;
		}
	}

	if (ret == PT_JOB_COMPLETED) {
		event = PT_EVENT_JOB_COMPLETED;
		printing_job->printing_state = PT_PRINT_END;
	} else if (ret == PT_JOB_ABORTED) {
		event = PT_EVENT_JOB_ABORTED;
		printing_job->printing_state = PT_PRINT_ABORT;
	} else if (ret == PT_JOB_CANCELED) {
		event = PT_EVENT_JOB_CANCELED;
		printing_job->printing_state = PT_PRINT_CANCEL;
	} else if (ret == PT_JOB_STOPPED) {
		event = PT_EVENT_JOB_STOPPED;
		printing_job->printing_state = PT_PRINT_STOP;
	} else if (ret == PT_JOB_PROCESSING) {
		event = PT_EVENT_JOB_PROCESSING;
		printing_job->printing_state = PT_PRINT_IN_PROGRESS;
	} else if (ret == PT_JOB_HELD) {
		event = PT_EVENT_JOB_HELD;
		printing_job->printing_state = PT_PRINT_HELD;
	} else if (ret == PT_JOB_PENDING) {
		event = PT_EVENT_JOB_PENDING;
		printing_job->printing_state = PT_PRINT_PENDING;
	} else {
		event = PT_EVENT_JOB_ERROR;
	}

	if (g_pt_info->evt_cb != NULL) {
		progress_info.job_id = jobid;
		g_pt_info->evt_cb(event, g_pt_info->user_data, &progress_info);
	} else {
		PT_DEBUG("Failed to send job status to application because of g_pt_info->evt_cb is NULL");
	}

	PRINT_SERVICE_FUNC_LEAVE;
	return ret;
}

/**
 *	This API check the printer status
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 */
pt_printer_state_e pt_check_printer_status(pt_printer_mgr_t *printer)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info->search == NULL || printer == NULL, PT_PRINTER_OFFLINE, "Printer is offline");

	// TODO: Need to replace new function without using avahi
	// XXX FIXME XXX
	if (g_pt_info->evt_cb != NULL) {
		g_pt_info->evt_cb(PT_EVENT_PRINTER_ONLINE, g_pt_info->user_data, NULL);
	} else {
		PT_DEBUG("Failed to send printer status to application because of g_pt_info->evt_cb is NULL");
	}

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_PRINTER_IDLE;
}

//NOTICE : Use this function instead of system() which has security hole.
//if envp is NULL, execute execv. otherwise, execute execvp.
//Argument array should include NULL as last element
/*
int execute_filter(const char *filter, const char *argv[], char *const envp[]){

	pid_t pid = 0;
	int status = 0;

	if ( filter == NULL || argv == NULL ){
		PT_DEBUG("argument is invalid");
		return -1;
	}

	pid = fork();

	if (pid == -1){
		PT_DEBUG("Failed to fork");
		return -1;
	}

	if (pid == 0) {
		if(envp != NULL){
			if ( execve(filter, argv, envp) < 0 ){
				PT_DEBUG("Can't execute(%d)",errno);
			}
		}
		else{
			if ( execv(filter, argv) < 0 ){
				PT_DEBUG("Can't execute(%d)",errno);
			}
		}
		exit(127);
	}

	do {
		if (waitpid(pid, &status, 0) == -1) {
			if (errno != EINTR){
				PT_DEBUG("Errno is not EINTR");
				return -1;
			}
		}
		else {
			PT_DEBUG("Parent process is normally terminated");
			return status;
		}
	} while (1);
}
*/
