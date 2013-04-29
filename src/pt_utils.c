/*
*	Printservice
*
* Copyright 2012  Samsung Electronics Co., Ltd

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
#include <ctype.h>
#include <cups/ppd.h>
#include <cups/http.h>
#include <linux/unistd.h>
#include <sys/utsname.h>
#include <dirent.h>

#include "pt_debug.h"
#include "pt_common.h"

#define MAX_PPDGZ_SIZE (1048576)

ppd_file_t *ppd;

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

char *pt_utils_filename_from_URI(const char *uri)
{
	char			scheme[1024];	/* URI for printer/class */
	char			username[1024];		/* Line from PPD file */
	char			hostname[1024];		/* Keyword from Default line */
	int             port;		/* Pointer into keyword... */
	char			resource[1024];	/* Temporary filename */
	char           *filename = NULL;

	PT_RETV_IF(uri == NULL, NULL, "uri is NULL");

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

static char *pt_utils_trim_prefix(const char *string, const char *prefix)
{
	PT_RETV_IF(string == NULL, NULL, "string is NULL");
	PT_RETV_IF(prefix == NULL, NULL, "prefix is NULL");

	char *buffer = NULL;
	char *ret = NULL;

	if (strlen(string) <= strlen(prefix)) {
		PT_DEBUG("(%s) is greater than (%s)",string, prefix);
		return NULL;
	}
	if (strncasecmp(string, prefix, strlen(prefix)) == 0) {
		buffer = strdup(string + strlen(prefix));
		PT_RETV_IF(buffer == NULL, NULL, "Not enough memory");
		ret = pt_utils_trim_space(buffer);
		PT_IF_FREE_MEM(buffer);
		PT_RETV_IF(ret == NULL, NULL, "Not enough memory");
		buffer = strdup(ret);
		PT_RETV_IF(buffer == NULL, NULL, "Not enough memory");
	}

	return buffer;
}

int pt_utils_get_mfg_mdl(const char *printer, char **mfg, char **mdl)
{
	PT_RETV_IF(printer == NULL || mfg == NULL || mdl == NULL , PT_ERR_FAIL, "Invalid argument");

	const char *vendor[3] = {"Samsung", "HP", "Epson"};
	int 	index 	= 0;

	if (strcasestr(printer, vendor[0]) != NULL) {
		index = 0;
	} else if (strcasestr(printer, vendor[1]) != NULL) {
		index = 1;
	} else if (strcasestr(printer, vendor[2]) != NULL) {
		index = 2;
	} else {
		PT_DEBUG("Unsupported printer %s", printer);
		return PT_ERR_UNSUPPORTED;
	}

	*mfg = strdup(vendor[index]);
	PT_RETV_IF(*mfg == NULL, PT_ERR_NO_MEMORY, "Not enough memory");

	char *temp1 = NULL;
	char *buffer = strdup(printer);
	PT_RETV_IF(buffer == NULL, PT_ERR_NO_MEMORY, "Not enough memory");

	//Patch unusual HP modelname case
	if (strcasestr(*mfg, "HP") != NULL) {
		while(strncasecmp(buffer, "Hewlett-Packard", strlen("Hewlett-Packard")) == 0) {
			temp1 = pt_utils_trim_prefix(buffer, "Hewlett-Packard");
			PT_IF_FREE_MEM(buffer);
			PT_RETV_IF(temp1 == NULL, PT_ERR_INVALID_PARAM, "Invalid value");
			buffer = temp1;
		}
	}

	// It can include Manufacturer twice in printer name.
	while(strncasecmp(buffer, vendor[index], strlen(vendor[index])) == 0) {
		temp1 = pt_utils_trim_prefix(buffer, vendor[index]);
		PT_IF_FREE_MEM(buffer);
		PT_RETV_IF(temp1 == NULL, PT_ERR_INVALID_PARAM, "Invalid value");
		buffer = temp1;
	}
	*mdl = buffer;
	return PT_ERR_NONE;
}

static long _pt_get_file_length(const char *filename)
{

	struct stat file_info;
	long len;

	if (stat(filename, &file_info)) {
		PT_DEBUG("File %s stat error", filename);
		len = 0;
	} else {
		PT_DEBUG("File size = %d", file_info.st_size);
		len = file_info.st_size;
	}

	return len;
}

int _pt_filecopy(const char *org_file_path, const char *dest_file_path)
{
	FILE *src = NULL;
	FILE *dest = NULL;
	char *buffer = NULL;
	size_t len = 0;

	PT_RETV_IF(org_file_path==NULL, -1, "org_file_path is NULL");
	PT_RETV_IF(dest_file_path==NULL, -1, "dest_file_path is NULL");

	if (!strcmp(org_file_path, dest_file_path)) {
		PT_DEBUG("org_file_path is same with dest_file_path");
		return -1;
	}

	if ((src  = fopen(org_file_path, "rb")) == NULL) {
		PT_DEBUG("Failed to open %s",org_file_path);
		return -1;
	}

	if (access(dest_file_path, F_OK) == 0) {
		PT_DEBUG("It exists already");
		fclose(src);
		return 0;
	}

	long src_length = _pt_get_file_length(org_file_path);
	if (src_length <= 0) {
		PT_DEBUG("failed to get file length(%ld)",src_length);
		fclose(src);
		return -1;
	}

	if ((dest = fopen(dest_file_path, "wb")) == NULL) {
		fclose(src);
		PT_DEBUG("Failed to open %s",dest_file_path);
		return -1;
	}

	buffer =	(char *) malloc(MAX_PPDGZ_SIZE);
	if (buffer == NULL) {
		fclose(src);
		fclose(dest);
		PT_DEBUG("Not enough memory");
		return -1;
	}

	while (1) {
		len = fread(buffer, sizeof(char), MAX_PPDGZ_SIZE, src);
		if (len < MAX_PPDGZ_SIZE) {
			if (feof(src) != 0) {
				fwrite(buffer, sizeof(char), len, dest);
				break;
			} else {
				PT_DEBUG("Failed to write output file");
				fclose(src);
				fclose(dest);
				free(buffer);
				unlink(dest_file_path);
				return -1;
			}
		}
		fwrite(buffer, sizeof(char), MAX_PPDGZ_SIZE, dest);
	}
	fclose(src);
	fclose(dest);
	free(buffer);

	long dest_length = _pt_get_file_length(dest_file_path);
	if (src_length != dest_length) {
		PT_DEBUG("file length is diffrent(%ld, %ld)",src_length, dest_length);
		return -1;
	}

	return 0;
}

void pt_utils_remove_files_in(const char *path)
{
	PT_RET_IF(path == NULL, "path is NULL");
	char *cwd;
	struct dirent *entry;
	int ret = 1;
	int iret = -1;
	DIR *dir;

	cwd = get_current_dir_name();

	errno = 0;
	ret = chdir(path);
	if (ret == 0) {
		dir = opendir(path);
		if (dir == NULL) {
			PT_IF_FREE_MEM(cwd);
			return;
		}
		while ((entry = readdir(dir)) != NULL) {
			PT_DEBUG("Remove %s", entry->d_name);
			iret = remove(entry->d_name);
			if (iret == -1) {
				PT_DEBUG("unable to remove %s",entry->d_name);
			}
		}
		closedir(dir);

		iret = chdir(cwd);
		PT_IF_FREE_MEM(cwd);
		PT_RET_IF(iret == -1, "unable to chdir");
	} else {
		if (errno == ENOENT) {
			PT_DEBUG("Not existed %s, just skip", path);
		}
	}
	PT_IF_FREE_MEM(cwd);
	return;
}
