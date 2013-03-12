/*
*  Printservice
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

#define PREFERENCE_DEFAULT_PRINTER_NAME "mobileprint_default_printer_name"
#define PREFERENCE_DEFAULT_PRINTER_ADDRESS "mobileprint_default_printer_address"
#define PREFERENCE_DEFAULT_PRINTER_MFG "mobileprint_default_printer_mfg"
#define PREFERENCE_DEFAULT_PRINTER_MDL "mobileprint_default_printer_mdl"

#define AVAHI_DAEMON "/usr/sbin/avahi-daemon"
#define CUPS_DAEMON  "/usr/sbin/cupsd"

#define AVAHI_TEMP			"/opt/var/run/avahi-daemon/pid"
#define CLEAR_JOB_TEMP 		"rm -rf /opt/var/spool/cups/* /opt/var/run/cups/*;killall cupsd"

#define JOB_IN_PROGRESS "Spooling job, "
#define JOB_COMPLETE "Job completed."
#define JOB_PAGE_PRINTED "Printed "

pt_info_t *g_pt_info = NULL;

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

	FILE *result = NULL;
	if ((result = popen(CLEAR_JOB_TEMP, "r")) != NULL) {
		pclose(result);
		result = NULL;
	}

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
		}
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

void pt_debug_print_device_attr(ipp_attribute_t *attr)
{
	PT_RET_IF(attr == NULL, "Invalid argument");
/* CUPS 1.6.1 support new ippGet function
	ippGetBoolean
	ippGetCollection
	ippGetCount
	ippGetDate
	ippGetGroupTag
	ippGetInteger
	ippGetName
	ippGetOperation
	ippGetRange
	ippGetRequestId
	ippGetResolution
	ippGetState
	ippGetStatusCode
	ippGetString
	ippGetValueTag
	ippGetVersion
  */
#ifdef CUPS_161_UPGRADE
	if (ippGetValueTag(attr) == IPP_TAG_TEXT || ippGetValueTag(attr) == IPP_TAG_URI) {
		PT_DEBUG("grp=[%d] name=[%s] text=[%s]", ippGetGroupTag(attr), (ippGetName(attr) ==NULL)?"nill":ippGetName(attr), ippGetString(attr, 0, NULL));
	} else if (ippGetValueTag(attr) == IPP_TAG_INTEGER) {
		PT_DEBUG("grp=[%d] name=[%s] int=[%d]", ippGetGroupTag(attr), (ippGetName(attr)==NULL)?"nill":ippGetName(attr), ippGetInteger(attr,0));
	} else if (ippGetValueTag(attr) == IPP_TAG_BOOLEAN) {
		PT_DEBUG("grp=[%d] name=[%s] bool=[%d]", ippGetGroupTag(attr), (ippGetName(attr)==NULL)?"nill":ippGetName(attr), ippGetBoolean(attr,0));
	} else if (ippGetValueTag(attr) == IPP_TAG_RESOLUTION) {
		int xres = -1;
		int yres = -1;
		ipp_res_t units = -1;
		xres = ippGetResolution(attr, 0, &yres, &units);
		PT_DEBUG("grp=[%d] name=[%s] res=[%d,%d, %d]", ippGetGroupTag(attr), (ippGetName(attr)==NULL)?"nill":ippGetName(attr),
			   xres, yres, units);
	} else if (ippGetValueTag(attr) == IPP_TAG_RANGE) {
		int uppper = -1;
		int lower = -1;
		lower = ippGetRange(attr, 0, &uppper);
		PT_DEBUG("grp=[%d] name=[%s] range=[%d,%d]", ippGetGroupTag(attr), (ippGetName(attr)==NULL)?"nill":ippGetName(attr), uppper, lower);
	} else if (ippGetValueTag(attr) == IPP_TAG_EVENT_NOTIFICATION) {
		PT_DEBUG("IPP_TAG_EVENT_NOTIFICATION");
		PT_DEBUG("grp=[%d] name=[%s] value_tag=[%d]", ippGetGroupTag(attr), (ippGetName(attr)==NULL)?"nill":ippGetName(attr), ippGetValueTag(attr));
	} else if (ippGetValueTag(attr) == IPP_TAG_SUBSCRIPTION) {
		PT_DEBUG("grp=[%d] name=[%s] value_tag=[%d]", ippGetGroupTag(attr), (ippGetName(attr)==NULL)?"nill":ippGetName(attr), ippGetValueTag(attr));
	} else {
		PT_DEBUG("grp=[%d] name=[%s] value_tag=[%d]", ippGetGroupTag(attr), (ippGetName(attr)==NULL)?"nill":ippGetName(attr), ippGetValueTag(attr));
	}
#else
	if (attr->value_tag == IPP_TAG_TEXT ||
			attr->value_tag == IPP_TAG_URI) {
		PT_DEBUG("grp=[%d] name=[%s] text=[%s]", attr->group_tag, (attr->name==NULL)?"nill":attr->name, attr->values[0].string.text);
	} else if (attr->value_tag == IPP_TAG_INTEGER) {
		PT_DEBUG("grp=[%d] name=[%s] int=[%d]", attr->group_tag, (attr->name==NULL)?"nill":attr->name, attr->values[0].integer);
	} else if (attr->value_tag == IPP_TAG_BOOLEAN) {
		PT_DEBUG("grp=[%d] name=[%s] bool=[%d]", attr->group_tag, (attr->name==NULL)?"nill":attr->name, attr->values[0].boolean);
	} else if (attr->value_tag == IPP_TAG_RESOLUTION)
		PT_DEBUG("grp=[%d] name=[%s] res=[%d,%d, %d]", attr->group_tag, (attr->name==NULL)?"nill":attr->name,
			   attr->values[0].resolution.xres, attr->values[0].resolution.yres,attr->values[0].resolution.units);
	else if (attr->value_tag == IPP_TAG_RANGE)
		PT_DEBUG("grp=[%d] name=[%s] range=[%d,%d]", attr->group_tag, (attr->name==NULL)?"nill":attr->name,
			   attr->values[0].range.upper, attr->values[0].range.lower);
	else if (attr->value_tag == IPP_TAG_EVENT_NOTIFICATION) {
		PT_DEBUG("IPP_TAG_EVENT_NOTIFICATION");
		PT_DEBUG("grp=[%d] name=[%s] value_tag=[%d]", attr->group_tag, (attr->name==NULL)?"nill":attr->name, attr->value_tag);
	} else if (attr->value_tag == IPP_TAG_SUBSCRIPTION) {
		PT_DEBUG("grp=[%d] name=[%s] value_tag=[%d]", attr->group_tag, (attr->name==NULL)?"nill":attr->name, attr->value_tag);
	} else {
		PT_DEBUG("grp=[%d] name=[%s] value_tag=[%d]", attr->group_tag, (attr->name==NULL)?"nill":attr->name, attr->value_tag);
	}
#endif

}

// FIXME - It will be used again if job-media-progress attribute is updated properly by CUPS
#if 0
static const char *const attrs[] = {
	"job-state",
	"job-media-progress",
};
#endif

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
#ifdef CUPS_161_UPGRADE
			printing_job->job->subscription_id = ippGetInteger(attr,0);
#else
			printing_job->job->subscription_id = attr->values[0].integer;
#endif
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

static Eina_Bool __pt_parse_job_complete(char *msg)
{
	PRINT_SERVICE_FUNC_ENTER;
	gboolean ret = EINA_FALSE;

	if (strstr(msg, JOB_COMPLETE) != NULL) {
		ret = EINA_TRUE;
	}

	PRINT_SERVICE_FUNC_LEAVE;
	return ret;
}


static int __pt_parse_progress_value(char *msg)
{
	PRINT_SERVICE_FUNC_ENTER;
	char progress[4] = {0,};
	int ret = 0;

	if (strstr(msg, JOB_IN_PROGRESS) != NULL) {
		progress[0] = *(msg + strlen(JOB_IN_PROGRESS));
		if (*(msg + strlen(JOB_IN_PROGRESS) + 1) == '%') {
			progress[1] = '\0';
		} else if (*(msg + strlen(JOB_IN_PROGRESS) + 2) == '%') {
			progress[1] = *(msg + strlen(JOB_IN_PROGRESS) + 1);
			progress[2] = '\0';
		} else {
			progress[1] = *(msg + strlen(JOB_IN_PROGRESS) + 1);
			progress[2] = *(msg + strlen(JOB_IN_PROGRESS) + 2);
			progress[3] = '\0';
		}

		ret = atoi(progress);
	}// else if (strstr(msg, JOB_COMPLETE) != NULL)
	//	ret = 100;

	if (ret > 0) {
		PT_DEBUG("Parsed progress : %d", ret);
	}

	PRINT_SERVICE_FUNC_LEAVE;
	return ret;
}

static int __pt_parse_page_value(char *msg)
{
	PRINT_SERVICE_FUNC_ENTER;
	char page[4] = {0,};
	int ret = 0;

	// FIXME - supported max page 999
	if (strstr(msg, JOB_PAGE_PRINTED) != NULL) {
		page[0] = *(msg + strlen(JOB_PAGE_PRINTED));
		if (*(msg + strlen(JOB_PAGE_PRINTED) + 1) == ' ') {
			page[1] = '\0';
		} else if (*(msg + strlen(JOB_PAGE_PRINTED) + 2) == ' ') {
			page[1] = *(msg + strlen(JOB_PAGE_PRINTED) + 1);
			page[2] = '\0';
		} else {
			page[1] = *(msg + strlen(JOB_PAGE_PRINTED) + 1);
			page[2] = *(msg + strlen(JOB_PAGE_PRINTED) + 2);
			page[3] = '\0';
		}

		ret = atoi(page);
	}

	PT_DEBUG("Parsed page : %d", ret);

	PRINT_SERVICE_FUNC_LEAVE;
	return ret;
}

static Eina_Bool __pt_parse_page_progress_value(char *msg, int *page, int *progress)
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

#if 0
// FIXME - It will be used again if job-media-progress attribute is updated properly by CUPS
// Get progress value by job-media-progress attribute of IPP
static Eina_Bool __pt_get_job_progress(void *data)
{
	PRINT_SERVICE_FUNC_ENTER;

	Eina_Bool ret = ECORE_CALLBACK_RENEW;
	int jobid = (int)data;
	char uri[HTTP_MAX_URI];
	pt_progress_info_t progress_info = {0,};

	http_t *http;
	ipp_t  *request, *response;
	//ipp_attribute_t *job_id;
	ipp_attribute_t *job_state;
	ipp_attribute_t *job_progress;

	http = httpConnectEncrypt(cupsServer(), ippPort(), cupsEncryption());
	request = ippNewRequest(IPP_GET_JOB_ATTRIBUTES);

	//httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL, "localhost", 631, "/printers/%s", g_pt_info->active_printer->name);
	//PT_DEBUG("request print-uri: %s\n", uri);
	//ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);

	httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL, "localhost", 631, "/jobs/%d", jobid);
	PT_DEBUG("request job-uri: %s\n", uri);
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "job-uri", NULL, uri);

	//PT_DEBUG("request job-id: %d\n", *jobid);
	//ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER, "job-id", *jobid);

	PT_DEBUG("request requested-attributes: %s\n", attrs[0]);
	PT_DEBUG("request requested-attributes: %s\n", attrs[1]);
	ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
				  "requested-attributes", sizeof(attrs) / sizeof(attrs[0]), NULL, attrs);

	if ((response = cupsDoRequest(http, request, "/")) != NULL) {
		PT_DEBUG("request ok!");

		if ((job_state = ippFindAttribute(response, "job-state", IPP_TAG_ENUM)) != NULL) {
			/*
			 * Stop polling if the job is finished or pending-held...
			 */

			//if ((job_id = ippFindAttribute(response, "job-id", IPP_TAG_INTEGER)) != NULL) {
			//	PT_DEBUG("job id of response : %d\n", job_id->values[0].integer);
			//}

			if (job_state->values[0].integer == IPP_JOB_PROCESSING) {
				if ((job_progress = ippFindAttribute(response, "job-media-progress", IPP_TAG_INTEGER)) != NULL) {
					PT_DEBUG("Current job id[%d] progress : %d\n", jobid, job_progress->values[0].integer);
					if (job_progress->values[0].integer > 0) {
						if (g_pt_info->evt_cb != NULL) {
							progress_info.job_id = jobid;
							progress_info.progress = job_progress->values[0].integer;
							g_pt_info->evt_cb(PT_EVENT_JOB_PROGRESS, g_pt_info->user_data, &progress_info);
						} else {
							PT_DEBUG("Failed to send job progress because of info->evt_cb is NULL");
						}
					}
				}
			} else if (job_state->values[0].integer == IPP_JOB_PENDING) {
				PT_DEBUG("job[%d] is pended.\n", jobid);
				if (g_pt_info->evt_cb != NULL) {
					progress_info.job_id = jobid;
					g_pt_info->evt_cb(PT_EVENT_JOB_PENDING, g_pt_info->user_data, &progress_info);
				} else {
					PT_DEBUG("Failed to send pending event because of info->evt_cb is NULL");
				}
			} else {
				PT_DEBUG("Current job id[%d] progress is stopped by job_state[%d]\n", jobid, job_state->values[0].integer);
				// FIXME - add proper routines for other cases
				// IPP_JOB_HELD
				// IPP_JOB_STOPPED
				// IPP_JOB_CANCELED
				// IPP_JOB_ABORTED
				// IPP_JOB_COMPLETED
				if (job_state->values[0].integer == IPP_JOB_STOPPED ||
						job_state->values[0].integer == IPP_JOB_CANCELED ||
						job_state->values[0].integer == IPP_JOB_ABORTED ||
						job_state->values[0].integer == IPP_JOB_COMPLETED) {
					ret = ECORE_CALLBACK_DONE;
				}
			}
		} else {
			/*
			 * If the printer does not return a job-state attribute, it does not
			 * conform to the IPP specification - break out immediately and fail
			 * the job...
			 */

			PT_DEBUG("No job-state available from printer - stopping queue.\n");
			//ipp_status = IPP_INTERNAL_ERROR;
		}

		ippDelete(response);
	}
	httpClose(http);

	PRINT_SERVICE_FUNC_LEAVE;

	return ret;
}
#endif

static void g_printer_data_destroy(gconstpointer a)
{
	free((char *)a);
}

#ifdef CUPS_161_UPGRADE

static void searching_thread_notify_cb(void *data, Ecore_Thread *thread, void *msg_data)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RET_IF(data == NULL, "data is NULL");
	PT_RET_IF(msg_data == NULL, "msg_data is NULL");
	PT_DEBUG("Thread is sent msg successfully.");

	pt_search_data_t 	*search_data;
	ipp_t 			*response;

	search_data = (pt_search_data_t *)data;
	response = (ipp_t *)msg_data;

	GHashTable *printer_hashlist = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify)g_printer_data_destroy);

	ipp_attribute_t *attr;
	const char *device_uri;				/* device-uri attribute value */
	const char *device_info;			/* device-info value */
	char *device_make_and_model;	/* device-make-and-model value */

	for (attr = ippFirstAttribute(response); attr != NULL; attr = ippNextAttribute(response)) {
		PT_DEBUG("response attributes!");

		while (attr && (ippGetGroupTag(attr) != IPP_TAG_PRINTER)) {
			pt_debug_print_device_attr(attr);
			attr = ippNextAttribute(response);
		}

		if (!attr) {
			break;
		}

		PT_DEBUG("attributes is not NULL!");

		device_info = NULL;
		device_uri = NULL;
		device_make_and_model = NULL;

		while (attr && ippGetGroupTag(attr) == IPP_TAG_PRINTER) {
			pt_debug_print_device_attr(attr);

			if (!strcmp(ippGetName(attr), "device-info") && ippGetValueTag(attr) == IPP_TAG_TEXT) {
				device_info = ippGetString(attr, 0, NULL);
			}

			if (!strcmp(ippGetName(attr), "device-make-and-model") && ippGetValueTag(attr) == IPP_TAG_TEXT) {
				device_make_and_model = ippGetString(attr, 0, NULL);
			}

			if (!strcmp(ippGetName(attr), "device-uri") && ippGetValueTag(attr) == IPP_TAG_URI) {
				device_uri = ippGetString(attr, 0, NULL);
			}

			attr = ippNextAttribute(response);
		}

		pt_debug_print_device_attr(attr);

		if (device_info != NULL && device_make_and_model !=NULL
				&& device_uri != NULL && strcasecmp(device_make_and_model, "unknown") != 0) {
			// Found some printers
			// there are "MFG MDL" || "MFG MFG MDL"
			// int pt_utils_get_mfg_mdl(const char* printer, char **mfg, char **mdl)
			if (strcasestr(device_uri, "usb://")) {
				pt_printer_info_t *localdetail = NULL;
				localdetail = (pt_printer_info_t *)malloc(sizeof(pt_printer_info_t));

				if (localdetail) {
					memset(localdetail, 0, sizeof(pt_printer_info_t));

					char *device_mfg = NULL;
					char *device_mdl = NULL;
					char *device_name = NULL;
					int ret = 0;
					ret = pt_utils_get_mfg_mdl(device_make_and_model, &device_mfg, &device_mdl);
					PT_RET_IF(ret != PT_ERR_NONE, "ERROR : %d in pt_utils_get_mfg_mdl", ret);

					device_name = calloc(1, strlen(device_mfg)+strlen(device_mdl)+2);
					PT_RET_IF(device_name == NULL, "No memory. calloc failed");

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

					search_data->pt_local_list = eina_list_append(search_data->pt_local_list, localdetail);
					if (eina_error_get()) {
						PT_DEBUG("Failed to add eina_list for search_data->pt_local_list");
					}
				}
			} else if (strstr(device_uri,"_pdl-datastream.") ||
					   strstr(device_uri,"_ipp.") ||
					   strstr(device_uri,"_printer.")) {
				pt_printer_info_t *localdetail = NULL;
				localdetail = (pt_printer_info_t *)malloc(sizeof(pt_printer_info_t));

				if (localdetail) {
					memset(localdetail, 0, sizeof(pt_printer_info_t));

					char *device_mfg = NULL;
					char *device_mdl = NULL;
					char *device_name = NULL;
					int ret = 0;
					ret = pt_utils_get_mfg_mdl(device_make_and_model, &device_mfg, &device_mdl);
					PT_RET_IF(ret != PT_ERR_NONE, "ERROR : %d in pt_utils_get_mfg_mdl", ret);
					PT_RET_IF(device_mfg == NULL, "device_mfg == NULL");
					PT_RET_IF(device_mdl == NULL, "device_mdl == NULL");
					device_name = calloc(1, strlen(device_mfg)+strlen(device_mdl)+2);
					PT_RET_IF(device_name == NULL, "No memory. calloc failed");

					strncat(device_name,device_mfg,strlen(device_mfg));
					strncat(device_name," ",1);
					strncat(device_name,device_mdl,strlen(device_mdl));

					strncpy(localdetail->model_name, device_mdl, sizeof(localdetail->model_name));
					strncpy(localdetail->manufacturer, device_mfg, sizeof(localdetail->manufacturer));
					strncpy(localdetail->product_ui_name, device_name, sizeof(localdetail->product_ui_name));
					__standardization(localdetail->product_ui_name);
					strncpy(localdetail->device_uri, device_uri, sizeof(localdetail->device_uri));
					strncpy(localdetail->device_info, device_info, sizeof(localdetail->device_info));

					PT_IF_FREE_MEM(device_mfg);
					PT_IF_FREE_MEM(device_mdl);
					PT_IF_FREE_MEM(device_name);

					pt_printer_info_t *tempdetail = NULL;
					tempdetail = (pt_printer_info_t *) g_hash_table_lookup(printer_hashlist, localdetail->device_info);

					if (NULL == tempdetail) {
						g_hash_table_insert(printer_hashlist, localdetail->device_info, localdetail);
						PT_DEBUG("Added printer_hashlist: %s", localdetail->device_uri);
					} else {
						if (strstr(tempdetail->device_uri, "_printer.") && strstr(localdetail->device_uri, "_ipp.")) {
							g_hash_table_replace(printer_hashlist, localdetail->device_info, localdetail);
							PT_DEBUG("Replaced printer_hashlist: %s", localdetail->device_uri);
						} else if (strstr(tempdetail->device_uri, "_pdl-datastream.") &&
								   (strstr(localdetail->device_uri, "_printer.") || strstr(localdetail->device_uri, "_ipp."))) {
							g_hash_table_replace(printer_hashlist, localdetail->device_info, localdetail);
							PT_DEBUG("Replaced printer_hashlist: %s", localdetail->device_uri);
						} else {
							PT_DEBUG("No change printer_hashlist: %s", tempdetail->device_uri);
						}
					}

					PT_DEBUG("device_info: %s", device_info);
					PT_DEBUG("device_make_and_model: %s", device_make_and_model);
					PT_DEBUG("device_uri: %s", device_uri);
					PT_DEBUG("localdetail mdl(%s) ", localdetail->model_name);
					PT_DEBUG("localdetail mfg(%s) ", localdetail->manufacturer);
					PT_DEBUG("localdetail product: %s", localdetail->product_ui_name);
					PT_DEBUG("localdetail url: %s", localdetail->device_uri);
				}
			} else {
				PT_DEBUG("Ignoring print device_uri=[%s]", device_uri);
				//ignore other protocols
			}
		}

		if (!attr) {
			break;
		}
	}

	GHashTableIter iter;
	gpointer key, value;
	pt_printer_info_t *printer_temp = NULL;
	g_hash_table_iter_init(&iter, printer_hashlist);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		printer_temp = (pt_printer_info_t *)malloc(sizeof(pt_printer_info_t));
		if (NULL == printer_temp) {
			return;
		}

		memset(printer_temp, 0, sizeof(pt_printer_info_t));
		memcpy(printer_temp, value, sizeof(pt_printer_info_t));
		search_data->pt_local_list = eina_list_append(search_data->pt_local_list, (pt_printer_info_t *)printer_temp);
		if (eina_error_get()) {
			PT_DEBUG("Failed to add eina_list for search_data->pt_local_list");
		}
	}
	g_hash_table_destroy(printer_hashlist);

	/* add searched printer */
	Eina_List *cursor = NULL;
	pt_printer_info_t *it = NULL;

	//TODO:: free ( search_data->response_data.printerlist );  -- dwmax
	search_data->response_data.printerlist = NULL;

	Eina_Bool bfirst = EINA_TRUE;
	/* add printer to response list */
	EINA_LIST_FOREACH(search_data->pt_local_list, cursor, it) {
		pt_printer_mgr_t *detail = NULL;
		detail = (pt_printer_mgr_t *)malloc(sizeof(pt_printer_mgr_t));
		if (detail != NULL)	{
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
			search_data->response_data.printerlist = eina_list_append(search_data->response_data.printerlist, detail);
			if (eina_error_get()) {
				PT_DEBUG("Failed to add eina_list for search_data->response_data.printerlist");
			}
		}
	}

	if (search_data->user_cb != NULL) {
		search_data->user_cb(&search_data->response_data);
	} else {
		PT_DEBUG("Failed to send searching result to application because of user_cb is NULL.");
	}

	ippDelete(response);
	PRINT_SERVICE_FUNC_LEAVE;
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
		int page_printed;
		Eina_Bool job_completed;
		Eina_Bool ret;

		PT_DEBUG("notify-text : %s", ippGetString(attr, 0, NULL));

		ret = __pt_parse_page_progress_value(ippGetString(attr, 0, NULL),
											 &printing_job->page_printed,
											 &progress);
		if (ret == EINA_FALSE) {
			page_printed = __pt_parse_page_value(ippGetString(attr, 0, NULL));
			if (page_printed > 0) {
				printing_job->page_printed = page_printed;
			}
			progress = __pt_parse_progress_value(ippGetString(attr, 0, NULL));
		}

		if (progress > 0) {
			if (NULL != g_pt_info->evt_cb) {
				progress_info.progress = progress;
				progress_info.page_printed = printing_job->page_printed;
				g_pt_info->evt_cb(PT_EVENT_JOB_PROGRESS, g_pt_info->user_data, &progress_info);
			}
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
				if (ret == EINA_FALSE) {
					page_printed = __pt_parse_page_value(ippGetString(attr, 0, NULL));
					if (page_printed > 0) {
						printing_job->page_printed = page_printed;
					}
					progress = __pt_parse_progress_value(ippGetString(attr, 0, NULL));
				}

				if (progress > 0) {
					if (NULL != g_pt_info->evt_cb) {
						progress_info.progress = progress;
						progress_info.page_printed = printing_job->page_printed;
						g_pt_info->evt_cb(PT_EVENT_JOB_PROGRESS, g_pt_info->user_data, &progress_info);
					}
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

#if 0 // No need to check job-media-attribute directly? scheduler log send "Printing page X X"
	//FIXME
	if (strcasestr(g_pt_info->active_printer->name, "samsung") == NULL) {
		__pt_get_job_progress((void *) printing_job->job->id);
	}
#endif

	PRINT_SERVICE_FUNC_LEAVE;
}


#else

static void searching_thread_notify_cb(void *data, Ecore_Thread *thread, void *msg_data)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RET_IF(data == NULL, "data is NULL");
	PT_RET_IF(msg_data == NULL, "msg_data is NULL");
	PT_DEBUG("Thread is sent msg successfully.");

	pt_search_data_t 	*search_data;
	ipp_t 			*response;

	search_data = (pt_search_data_t *)data;
	response = (ipp_t *)msg_data;

	GHashTable *printer_hashlist = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify)g_printer_data_destroy);

	ipp_attribute_t *attr;
	const char *device_uri;				/* device-uri attribute value */
	const char *device_info;			/* device-info value */
	char *device_make_and_model;	/* device-make-and-model value */

	for (attr = response->attrs; attr; attr = attr->next) {
		PT_DEBUG("response attributes!");

		while (attr && attr->group_tag != IPP_TAG_PRINTER) {
			pt_debug_print_device_attr(attr);
			attr = attr->next;
		}

		if (!attr) {
			break;
		}

		PT_DEBUG("attributes is not NULL!");

		device_info = NULL;
		device_uri = NULL;
		device_make_and_model = NULL;

		while (attr && attr->group_tag == IPP_TAG_PRINTER) {
			pt_debug_print_device_attr(attr);

			if (!strcmp(attr->name, "device-info") && attr->value_tag == IPP_TAG_TEXT) {
				device_info = attr->values[0].string.text;
			}

			if (!strcmp(attr->name, "device-make-and-model") && attr->value_tag == IPP_TAG_TEXT) {
				device_make_and_model = attr->values[0].string.text;
			}

			if (!strcmp(attr->name, "device-uri") && attr->value_tag == IPP_TAG_URI) {
				device_uri = attr->values[0].string.text;
			}

			attr = attr->next;
		}

		pt_debug_print_device_attr(attr);

		if (device_info != NULL && device_make_and_model !=NULL
				&& device_uri != NULL && strcasecmp(device_make_and_model, "unknown") != 0) {
			// Found some printers
			// there are "MFG MDL" || "MFG MFG MDL"
			// int pt_utils_get_mfg_mdl(const char* printer, char **mfg, char **mdl)
			if (strcasestr(device_uri, "usb://")) {
				pt_printer_info_t *localdetail = NULL;
				localdetail = (pt_printer_info_t *)malloc(sizeof(pt_printer_info_t));

				if (localdetail) {
					memset(localdetail, 0, sizeof(pt_printer_info_t));

					char *device_mfg = NULL;
					char *device_mdl = NULL;
					char *device_name = NULL;
					int ret = 0;
					ret = pt_utils_get_mfg_mdl(device_make_and_model, &device_mfg, &device_mdl);
					PT_RET_IF(ret != PT_ERR_NONE, "ERROR : %d in pt_utils_get_mfg_mdl", ret);

					device_name = calloc(1, strlen(device_mfg)+strlen(device_mdl)+2);
					PT_RET_IF(device_name == NULL, "No memory. calloc failed");

					strncat(device_name,device_mfg,strlen(device_mfg));
					strncat(device_name," ",1);
					strncat(device_name,device_mdl,strlen(device_mdl));

					strncpy(localdetail->model_name, device_mdl, sizeof(localdetail->model_name));
					strncpy(localdetail->manufacturer, device_mfg, sizeof(localdetail->manufacturer));
					strncpy(localdetail->product_ui_name, device_name, sizeof(localdetail->product_ui_name));
					__standardization(localdetail->product_ui_name);
					strncpy(localdetail->device_uri, device_uri, sizeof(localdetail->device_uri));
					strncpy(localdetail->device_info, device_info, sizeof(localdetail->device_info));

					PT_IF_FREE_MEM(device_mfg);
					PT_IF_FREE_MEM(device_mdl);
					PT_IF_FREE_MEM(device_name);

					search_data->pt_local_list = eina_list_append(search_data->pt_local_list, localdetail);
					if (eina_error_get()) {
						PT_DEBUG("Failed to add eina_list for search_data->pt_local_list");
					}
				}
			} else if (strstr(device_uri,"_pdl-datastream.") ||
					   strstr(device_uri,"_ipp.") ||
					   strstr(device_uri,"_printer.")) {
				pt_printer_info_t *localdetail = NULL;
				localdetail = (pt_printer_info_t *)malloc(sizeof(pt_printer_info_t));

				if (localdetail) {
					memset(localdetail, 0, sizeof(pt_printer_info_t));

					char *device_mfg = NULL;
					char *device_mdl = NULL;
					char *device_name = NULL;
					int ret = 0;
					ret = pt_utils_get_mfg_mdl(device_make_and_model, &device_mfg, &device_mdl);
					PT_RET_IF(ret != PT_ERR_NONE, "ERROR : %d in pt_utils_get_mfg_mdl", ret);
					PT_RET_IF(device_mfg == NULL, "device_mfg == NULL");
					PT_RET_IF(device_mdl == NULL, "device_mdl == NULL");
					device_name = calloc(1, strlen(device_mfg)+strlen(device_mdl)+2);
					PT_RET_IF(device_name == NULL, "No memory. calloc failed");

					strncat(device_name,device_mfg,strlen(device_mfg));
					strncat(device_name," ",1);
					strncat(device_name,device_mdl,strlen(device_mdl));

					strncpy(localdetail->model_name, device_mdl, sizeof(localdetail->model_name));
					strncpy(localdetail->manufacturer, device_mfg, sizeof(localdetail->manufacturer));
					strncpy(localdetail->product_ui_name, device_name, sizeof(localdetail->product_ui_name));
					__standardization(localdetail->product_ui_name);
					strncpy(localdetail->device_uri, device_uri, sizeof(localdetail->device_uri));
					strncpy(localdetail->device_info, device_info, sizeof(localdetail->device_info));

					PT_IF_FREE_MEM(device_mfg);
					PT_IF_FREE_MEM(device_mdl);
					PT_IF_FREE_MEM(device_name);

					pt_printer_info_t *tempdetail = NULL;
					tempdetail = (pt_printer_info_t *) g_hash_table_lookup(printer_hashlist, localdetail->device_info);

					if (NULL == tempdetail) {
						g_hash_table_insert(printer_hashlist, localdetail->device_info, localdetail);
						PT_DEBUG("Added printer_hashlist: %s", localdetail->device_uri);
					} else {
						if (strstr(tempdetail->device_uri, "_printer.") && strstr(localdetail->device_uri, "_ipp.")) {
							g_hash_table_replace(printer_hashlist, localdetail->device_info, localdetail);
							PT_DEBUG("Replaced printer_hashlist: %s", localdetail->device_uri);
						} else if (strstr(tempdetail->device_uri, "_pdl-datastream.") &&
								   (strstr(localdetail->device_uri, "_printer.") || strstr(localdetail->device_uri, "_ipp."))) {
							g_hash_table_replace(printer_hashlist, localdetail->device_info, localdetail);
							PT_DEBUG("Replaced printer_hashlist: %s", localdetail->device_uri);
						} else {
							PT_DEBUG("No change printer_hashlist: %s", tempdetail->device_uri);
						}
					}

					PT_DEBUG("device_info: %s", device_info);
					PT_DEBUG("device_make_and_model: %s", device_make_and_model);
					PT_DEBUG("device_uri: %s", device_uri);
					PT_DEBUG("localdetail mdl(%s) ", localdetail->model_name);
					PT_DEBUG("localdetail mfg(%s) ", localdetail->manufacturer);
					PT_DEBUG("localdetail product: %s", localdetail->product_ui_name);
					PT_DEBUG("localdetail url: %s", localdetail->device_uri);
				}
			} else {
				PT_DEBUG("Ignoring print device_uri=[%s]", device_uri);
				//ignore other protocols
			}
		}

		if (!attr) {
			break;
		}
	}

	GHashTableIter iter;
	gpointer key, value;
	pt_printer_info_t *printer_temp = NULL;
	g_hash_table_iter_init(&iter, printer_hashlist);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		printer_temp = (pt_printer_info_t *)malloc(sizeof(pt_printer_info_t));
		if (NULL == printer_temp) {
			return;
		}

		memset(printer_temp, 0, sizeof(pt_printer_info_t));
		memcpy(printer_temp, value, sizeof(pt_printer_info_t));
		search_data->pt_local_list = eina_list_append(search_data->pt_local_list, (pt_printer_info_t *)printer_temp);
		if (eina_error_get()) {
			PT_DEBUG("Failed to add eina_list for search_data->pt_local_list");
		}
	}
	g_hash_table_destroy(printer_hashlist);

	/* add searched printer */
	Eina_List *cursor = NULL;
	pt_printer_info_t *it = NULL;

	//TODO:: free ( search_data->response_data.printerlist );  -- dwmax
	search_data->response_data.printerlist = NULL;

	Eina_Bool bfirst = EINA_TRUE;
	/* add printer to response list */
	EINA_LIST_FOREACH(search_data->pt_local_list, cursor, it) {
		pt_printer_mgr_t *detail = NULL;
		detail = (pt_printer_mgr_t *)malloc(sizeof(pt_printer_mgr_t));
		if (detail != NULL)	{
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
			search_data->response_data.printerlist = eina_list_append(search_data->response_data.printerlist, detail);
			if (eina_error_get()) {
				PT_DEBUG("Failed to add eina_list for search_data->response_data.printerlist");
			}
		}
	}

	if (search_data->user_cb != NULL) {
		search_data->user_cb(&search_data->response_data);
	} else {
		PT_DEBUG("Failed to send searching result to application because of user_cb is NULL.");
	}

	ippDelete(response);
	PRINT_SERVICE_FUNC_LEAVE;
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
		int page_printed;
		Eina_Bool job_completed;
		Eina_Bool ret;

		PT_DEBUG("notify-text : %s", attr->values[0].string.text);

		ret = __pt_parse_page_progress_value(attr->values[0].string.text,
											 &printing_job->page_printed,
											 &progress);
		if (ret == EINA_FALSE) {
			page_printed = __pt_parse_page_value(attr->values[0].string.text);
			if (page_printed > 0) {
				printing_job->page_printed = page_printed;
			}
			progress = __pt_parse_progress_value(attr->values[0].string.text);
		}

		if (progress > 0) {
			if (NULL != g_pt_info->evt_cb) {
				progress_info.progress = progress;
				progress_info.page_printed = printing_job->page_printed;
				g_pt_info->evt_cb(PT_EVENT_JOB_PROGRESS, g_pt_info->user_data, &progress_info);
			}
		}

		job_completed = __pt_parse_job_complete(attr->values[0].string.text);
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
				PT_DEBUG("notify-text : %s", attr->values[0].string.text);
				ret = __pt_parse_page_progress_value(attr->values[0].string.text,
													 &printing_job->page_printed,
													 &progress);
				if (ret == EINA_FALSE) {
					page_printed = __pt_parse_page_value(attr->values[0].string.text);
					if (page_printed > 0) {
						printing_job->page_printed = page_printed;
					}
					progress = __pt_parse_progress_value(attr->values[0].string.text);
				}

				if (progress > 0) {
					if (NULL != g_pt_info->evt_cb) {
						progress_info.progress = progress;
						progress_info.page_printed = printing_job->page_printed;
						g_pt_info->evt_cb(PT_EVENT_JOB_PROGRESS, g_pt_info->user_data, &progress_info);
					}
				}

				job_completed = __pt_parse_job_complete(attr->values[0].string.text);
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
			if (attr->values[0].integer > printing_job->job->sequence_num) {
				printing_job->job->sequence_num = attr->values[0].integer;
			}

		ippDelete(response);
	}

#if 0 // No need to check job-media-attribute directly? scheduler log send "Printing page X X"
	//FIXME
	if (strcasestr(g_pt_info->active_printer->name, "samsung") == NULL) {
		__pt_get_job_progress((void *) printing_job->job->id);
	}
#endif

	PRINT_SERVICE_FUNC_LEAVE;
}
#endif

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
	g_pt_info->searching_state = PT_SEARCH_CANCEL;
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

	if ((connection_status & PT_CONNECTION_USB) == 0 ) {
		exclude_schemes[num_schemes++] = "usb";
	}
	if ((connection_status & PT_CONNECTION_WIFI) == 0 &&
		(connection_status & PT_CONNECTION_WIFI_DIRECT) == 0) {
		exclude_schemes[num_schemes++] = "dnssd";
	}
	
	ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "exclude-schemes",
					num_schemes ,NULL , exclude_schemes);
	ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER, "timeout",5);

	if ((response = cupsDoRequest(http, request, "/")) != NULL) {
		PT_DEBUG("request ok!");
		if (g_pt_info->searching_state == PT_SEARCH_CANCEL) {
			ippDelete(response);
			goto canceled;
		}

		if (ecore_thread_feedback(thread, (const void *)response) == EINA_FALSE) {
			PT_DEBUG("Failed to send data to main loop");
			ippDelete(response);
		}
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
	//memset(g_pt_info, 0, sizeof(pt_info_t));

	g_pt_info->evt_cb = evt_cb;
	g_pt_info->user_data = user_data;

	__pt_launch_daemon();

	g_pt_info->active_printer = (pt_printer_mgr_t *)calloc(1, sizeof(pt_printer_mgr_t));
	PT_RETV_IF(g_pt_info->active_printer == NULL, PT_ERR_NO_MEMORY,
			   "Failed to malloc");
	//memset(g_pt_info->active_printer, 0, sizeof(pt_printer_mgr_t));

	g_pt_info->option = (pt_print_option_t *)calloc(1, sizeof(pt_print_option_t));
	PT_RETV_IF(g_pt_info->option == NULL, PT_ERR_NO_MEMORY,
			   "Failed to malloc");
	//memset(g_pt_info->option, 0, sizeof(pt_print_option_t));

	g_pt_info->search = (pt_search_data_t *)calloc(1, sizeof(pt_search_data_t));
	PT_RETV_IF(g_pt_info->search == NULL, PT_ERR_NO_MEMORY,
			   "Failed to malloc");
	//memset(g_pt_info->search, 0, sizeof(pt_search_data));

	g_pt_info->search->response_data.printerlist = NULL ;
	g_pt_info->search->is_searching = 0 ;
	g_pt_info->ueventsocket = -1;

	//memset(g_pt_info->active_printer, 0, sizeof(g_pt_info->active_printer));
	//memset(g_pt_info->option, 0, sizeof(g_pt_info->option));

	__pt_register_hotplug_event(g_pt_info);

	//_pt_vendor_option_init();

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
/**
 *	This API let the app set copies option for active printer, if not find active printer, will use default printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] copies the copy number
 */
int pt_set_print_option_copies(int copies)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info == NULL || g_pt_info->option == NULL, PT_ERR_INVALID_PARAM, "Invalid argument");
	PT_RETV_IF(copies <= 0, PT_ERR_INVALID_PARAM, "Invalid argument");

	char buf[MAX_SIZE] = {0};
	snprintf(buf, MAX_SIZE, "%d", copies);
	g_pt_info->num_options = cupsAddOption("copies", buf, g_pt_info->num_options, &g_pt_info->job_options);
	PT_DEBUG("num is %d", g_pt_info->num_options);
	PT_DEBUG("options is %p", g_pt_info->job_options);

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}
/**
 *	This API let the app set copies option for active printer, if not find active printer, will use default printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] copies the copy number
 */
int pt_set_print_option_color(void)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info == NULL || g_pt_info->option == NULL, PT_ERR_INVALID_PARAM, "Invalid argument");

	ppd_choice_t *choi = pt_selected_choice(PT_OPTION_ID_GRAYSCALE);
	if (choi) {
		g_pt_info->num_options = cupsAddOption(choi->option->keyword, choi->choice, g_pt_info->num_options, &g_pt_info->job_options);
	}

	PT_DEBUG("num is %d", g_pt_info->num_options);
	PT_DEBUG("options is %p", g_pt_info->job_options);
	if (choi) {
		PT_DEBUG("%s=%s", choi->option->keyword, choi->choice);
	}

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}

int pt_set_print_option_paper_type(void)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info == NULL || g_pt_info->option == NULL, PT_ERR_INVALID_PARAM, "Invalid argument");

	ppd_choice_t *choi = pt_selected_choice(PT_OPTION_ID_PAPER);
	if (choi) {
		g_pt_info->num_options = cupsAddOption(choi->option->keyword, choi->choice, g_pt_info->num_options, &g_pt_info->job_options);
	}

	PT_DEBUG("num is %d", g_pt_info->num_options);
	PT_DEBUG("options is %p", g_pt_info->job_options);
	if (choi) {
		PT_DEBUG("%s=%s", choi->option->keyword, choi->choice);
	}

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}

int pt_set_print_option_quality(void)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info == NULL || g_pt_info->option == NULL, PT_ERR_INVALID_PARAM, "Invalid argument");

	ppd_choice_t *choi = pt_selected_choice(PT_OPTION_ID_QUALITY);
	if (choi) {
		g_pt_info->num_options = cupsAddOption(choi->option->keyword, choi->choice, g_pt_info->num_options, &g_pt_info->job_options);
	}

	PT_DEBUG("num is %d", g_pt_info->num_options);
	PT_DEBUG("options is %p", g_pt_info->job_options);
	if (choi) {
		PT_DEBUG("%s=%s", choi->option->keyword, choi->choice);
	}

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}

/**
 *	This API let the app set paper option for active printer, if not find active printer, will use default printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] papersize the paper size
 */
int pt_set_print_option_papersize(void)
{
	PRINT_SERVICE_FUNC_ENTER;

	ppd_choice_t *choi = pt_selected_choice(PT_OPTION_ID_PAPERSIZE);
	if (choi) {
		g_pt_info->num_options = cupsAddOption(choi->option->keyword, choi->choice, g_pt_info->num_options, &g_pt_info->job_options);
	}

	PT_DEBUG("num is %d", g_pt_info->num_options);
	PT_DEBUG("options is %p", g_pt_info->job_options);
	if (choi) {
		PT_DEBUG("%s=%s", choi->option->keyword, choi->choice);
	}

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}

/**
 *	This API let the app set orientation option for active printer, if not find active printer, will use default printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] orientation the orientation value(0:portrait, 1:landscape)
 */
int pt_set_print_option_orientation(pt_orientation_e orientation)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info == NULL || g_pt_info->option == NULL, PT_ERR_INVALID_PARAM, "Invalid argument");
	PT_RETV_IF((orientation < PT_ORIENTATION_PORTRAIT) || (orientation > PT_ORIENTATION_LANDSCAPE)
			   , PT_ERR_INVALID_PARAM, "Invalid argument");

	if (orientation == PT_ORIENTATION_PORTRAIT) {
		g_pt_info->num_options = cupsAddOption("landscape", "off", g_pt_info->num_options, &g_pt_info->job_options);
	} else if (orientation == PT_ORIENTATION_LANDSCAPE) {
		g_pt_info->num_options = cupsAddOption("landscape", "on", g_pt_info->num_options, &g_pt_info->job_options);
	} else {
		PT_DEBUG("CUPS will do auto-rotate when landscape option is not specified");
	}

	PT_DEBUG("num is %d", g_pt_info->num_options);
	PT_DEBUG("options is %p", g_pt_info->job_options);

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}

/**
 *	This API let the app set image fit for the page size, if not find active printer, will use default printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 */
int pt_set_print_option_scaleimage(pt_scaling_e scaling)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info == NULL || g_pt_info->option == NULL, PT_ERR_INVALID_PARAM, "Invalid argument");

	char scaling_option[MAX_SIZE] = {0};
	char percent_size[MAX_SIZE] = {0};
	switch (scaling) {
	case PT_SCALING_2_PAGES:
		strcpy(scaling_option, "number-up");   // 2 pages in 1 sheet
		strcpy(percent_size, "2");
		break;
	case PT_SCALING_4_PAGES:
		strcpy(scaling_option, "number-up");   // 4 pages in 1 sheet
		strcpy(percent_size, "4");
		break;
	case PT_SCALING_FIT_TO_PAGE:
	default:
		strcpy(scaling_option, "scaling");      // fit-to-paper
		strcpy(percent_size, "100");
		break;
	}

// do not send number-up option to CUPS daemon.
// imagetoraster do not support number-up
// To print image to resolve OOM, apply number-up option to pdf in advance.
	g_pt_info->num_options = cupsAddOption(scaling_option, percent_size, g_pt_info->num_options, &g_pt_info->job_options);
	PT_DEBUG("num is %d", g_pt_info->num_options);
	PT_DEBUG("options is %p", g_pt_info->job_options);
	PT_DEBUG("scaling option:[%s %s]", scaling_option, percent_size);

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}

/**
 *	This API let the app set image fit for the page size, if not find active printer, will use default printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 */
int pt_set_print_option_imagesize(pt_imagesize_t *crop_image_info, const char *filepath, int res_x, int res_y)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info == NULL || g_pt_info->option == NULL, PT_ERR_INVALID_PARAM, "Invalid argument");
	PT_RETV_IF(filepath == NULL, PT_ERR_INVALID_PARAM, "Invalid argument");

	char imagesize_option[MAX_SIZE] = {0};
	char ppi_value[MAX_SIZE] = {0};

	int ppi = 128;
	double divisor = 1;

	switch (crop_image_info->imagesize) {
	case PT_SIZE_5X7:					/** 13 cm X 18 cm */
		PT_DEBUG("selected PT_SIZE_5X7");
		divisor = 7.000;
		break;
	case PT_SIZE_4X6:					/** 10 cm X 15 cm */
		PT_DEBUG("selected PT_SIZE_4X6");
		divisor = 6.000;
		break;
	case PT_SIZE_3_5X5:				/** 9 cm X 13 cm */
		PT_DEBUG("selected PT_SIZE_3_5X5");
		divisor = 5.000;
		break;
	case PT_SIZE_WALLET:				/** 6.4cm X 8.4 cm */
		PT_DEBUG("selected PT_SIZE_WALLET");
		divisor = 3.307;
		break;
	case PT_SIZE_CUSTOM:
		PT_DEBUG("selected PT_SIZE_CUSTOM");
		if (crop_image_info->unit == 1) { // cm to inch
			crop_image_info->resolution_width = crop_image_info->resolution_width / 2.54;
			crop_image_info->resolution_height = crop_image_info->resolution_height / 2.54;
			crop_image_info->unit = 2;
			crop_image_info->ratio = crop_image_info->resolution_width / crop_image_info->resolution_height;
		}

		if (crop_image_info->resolution_width < crop_image_info->resolution_height) {
			divisor = crop_image_info->resolution_height;
		} else {
			divisor = crop_image_info->resolution_width;
		}
		break;
	default:
		divisor = 1;
		PT_DEBUG("[ERROR] Undefined imagesize(%d)",crop_image_info->imagesize);
		return PT_ERR_INVALID_PARAM;
	}

	if (res_x < res_y) {
		ppi = (int) ceil((double)(res_y) / (double)divisor);
	} else {
		ppi = (int) ceil((double)(res_x) / (double)divisor);
	}
	strcpy(imagesize_option, "ppi");
	snprintf(ppi_value, MAX_SIZE, "%d", ppi);

	g_pt_info->num_options = cupsAddOption(imagesize_option, ppi_value, g_pt_info->num_options, &g_pt_info->job_options);
	PT_DEBUG("num is %d", g_pt_info->num_options);
	PT_DEBUG("options is %p", g_pt_info->job_options);
	PT_DEBUG("imagesize_option :[%s %s]", imagesize_option, ppi_value);

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}



/**
 *	This API let the app set image fit for the page size, if not find active printer, will use default printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 */
int pt_set_print_option_print_range(int from, int to)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(g_pt_info == NULL || g_pt_info->option == NULL, PT_ERR_INVALID_PARAM, "Invalid argument");

	char range_option[MAX_SIZE] = {0};
	if (from < 1) {
		from = 1;
	}
	sprintf(range_option, "%d-%d", from, to);
	g_pt_info->num_options = cupsAddOption("page-ranges", range_option, g_pt_info->num_options, &g_pt_info->job_options);

	PT_DEBUG("num is %d", g_pt_info->num_options);
	PT_DEBUG("options is %p", g_pt_info->job_options);
	PT_DEBUG("print range option:[%s]", range_option);

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
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
