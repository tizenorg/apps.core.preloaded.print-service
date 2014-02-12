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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>

/* For multi-user support */
#include <tzplatform_config.h>

#include <pt_db.h>
#include "pt_debug.h"
#include "pt_common.h"
#include "pt_ppd.h"

#define GETPPD "/usr/bin/getppd"
#define CUPS_PPDDIR tzplatform_mkpath(TZ_SYS_ETC, "cups/ppd/")
#define SAMSUNG_DRV tzplatform_mkpath(TZ_SYS_ETC, "cups/ppd/samsung/samsung.drv")
#define SAMSUNG_DRV_GZ tzplatform_mkpath(TZ_SYS_ETC, "cups/ppd/samsung/samsung.drv.gz")
#define SAMSUNG_DRV_GZ_ORG "/usr/share/cups/ppd/samsung/samsung.drv.gz"
#define HP_DRV tzplatform_mkpath(TZ_SYS_ETC, "cups/ppd/hp/hp.drv")
#define HP_DRV_GZ tzplatform_mkpath(TZ_SYS_ETC, "cups/ppd/hp/hp.drv.gz")
#define HP_DRV_GZ_ORG "/usr/share/cups/ppd/hp/hp.drv.gz"
#define EPSON_DRV tzplatform_mkpath(TZ_SYS_ETC, "cups/ppd/epson/epson.drv")
#define EPSON_DRV_GZ tzplatform_mkpath(TZ_SYS_ETC, "cups/ppd/epson/epson.drv.gz")
#define EPSON_DRV_GZ_ORG "/usr/share/cups/ppd/epson/epson.drv.gz"
#define SAMSUNG_PPD_DIR tzplatform_mkpath(TZ_SYS_ETC, "cups/ppd/samsung")
#define EPSON_PPD_DIR tzplatform_mkpath(TZ_SYS_ETC, "cups/ppd/epson")
#define HP_PPD_DIR tzplatform_mkpath(TZ_SYS_ETC, "cups/ppd/hp")

#define PPDC_PREFIX "ppdc: Writing ./"

/*supported manufacturer printer*/
const char *manufacturer[MANUFACTURER_NUM] = {"Samsung", "Hp", "Epson"};

ppd_size_t *pt_utils_paper_size_pts(const char *name)
{
	return ppdPageSize(ppd, name);
}

/**
 *	This function let the app get ppd file for the specified printer
 *	@return    If success, return PT_ERR_NONE, else return PT_ERR_FAIL
 *	@param[in] ppd the pointer to the printer's ppd path
 *	@param[in] printer the printer entry
 */
int pt_get_printer_ppd(pt_printer_mgr_t *printer)
{
	PRINT_SERVICE_FUNC_ENTER;
	PT_RETV_IF(printer == NULL, PT_ERR_INVALID_PARAM, "printer is NULL");
	PT_RETV_IF(printer->mfg == NULL, PT_ERR_INVALID_PARAM, "printer mfg is NULL");
	PT_RETV_IF(printer->mdl == NULL, PT_ERR_INVALID_PARAM, "printer mdl is NULL");

	PT_DEBUG("product: %s", printer->name);
	PT_DEBUG("printer->mdl %s", printer->mdl);
	PT_DEBUG("printer->mfg %s", printer->mfg);

	FILE *result = NULL;

	char gunzip_drv[PT_MAX_LENGTH] = {0,};

	static pt_db *db_samsung = NULL;
	static pt_db *db_epson = NULL;
	static pt_db *db_hp = NULL;
	char *output = NULL;

	if (strncasecmp(printer->mfg, "Samsung", 7) == 0) {
		if (chdir(SAMSUNG_PPD_DIR)) {
			PT_DEBUG("Failed to change directory");
			return PT_ERR_FAIL;
		}
		if (access(SAMSUNG_DRV, F_OK) != 0) {
			int ret = _pt_filecopy(SAMSUNG_DRV_GZ_ORG, SAMSUNG_DRV_GZ);
			if (ret != 0) {
				PT_DEBUG("Failed to copy file");
				return PT_ERR_FAIL;
			}
			snprintf(gunzip_drv, PT_MAX_LENGTH, "gzip -d %s", SAMSUNG_DRV_GZ);
			result = popen(gunzip_drv, "r");
			if (result == NULL) {
				PT_DEBUG("%s is failed to popen error",gunzip_drv);
				return PT_ERR_FAIL;
			}
			pclose(result);
		}
		if(!db_samsung) {
			db_samsung = pt_create_db(SAMSUNG_DRV);
			PT_DEBUG("db_samsung: %p", db_samsung);
			if(!db_samsung) {
				PT_DEBUG("Failed to create samsung database");
				return PT_ERR_FAIL;
			}
		}
		output = pt_extract_ppd(db_samsung, printer->mdl, PT_SEARCH_ALL);

	} else if (strncasecmp(printer->mfg, "Hp", 2) == 0) {
		if (chdir(HP_PPD_DIR)) {
			PT_DEBUG("Failed to change directory");
			return PT_ERR_FAIL;
		}
		if (access(HP_DRV, F_OK) != 0) {
			int ret = _pt_filecopy(HP_DRV_GZ_ORG, HP_DRV_GZ);
			if (ret != 0) {
				PT_DEBUG("Failed to copy file");
				return PT_ERR_FAIL;
			}
			snprintf(gunzip_drv, PT_MAX_LENGTH, "gzip -d %s", HP_DRV_GZ);
			result = popen(gunzip_drv, "r");
			if (result == NULL) {
				PT_DEBUG("%s is failed to popen error",gunzip_drv);
				return PT_ERR_FAIL;
			}
			pclose(result);
		}
		if(!db_hp) {
			db_hp = pt_create_db(HP_DRV);
			if(!db_hp) {
				PT_DEBUG("Failed to create hp database");
				return PT_ERR_FAIL;
			}
		}
		output = pt_extract_ppd(db_hp, printer->mdl, PT_SEARCH_ALL);
	} else if (strncasecmp(printer->mfg, "Epson", 5) == 0) {
		if (chdir(EPSON_PPD_DIR)) {
			PT_DEBUG("Failed to change directory");
			return PT_ERR_FAIL;
		}
		if (access(EPSON_DRV, F_OK) != 0) {
			int ret = _pt_filecopy(EPSON_DRV_GZ_ORG, EPSON_DRV_GZ);
			if (ret != 0) {
				PT_DEBUG("Failed to copy file");
				return PT_ERR_FAIL;
			}
			snprintf(gunzip_drv, PT_MAX_LENGTH, "gzip -d %s", EPSON_DRV_GZ);
			result = popen(gunzip_drv, "r");
			if (result == NULL) {
				PT_DEBUG("%s is failed to popen error",gunzip_drv);
				return PT_ERR_FAIL;
			}
			pclose(result);
		}
		if(!db_epson) {
			db_epson = pt_create_db(EPSON_DRV);
			if(!db_epson) {
				PT_DEBUG("Failed to create epson database");
				return PT_ERR_FAIL;
			}
		}
		output = pt_extract_ppd(db_epson, printer->mdl, PT_SEARCH_ALL);
	} else {
		PT_DEBUG("Can't find PPD file");
		return PT_ERR_FAIL;
	}

	if(!output) {
		PT_DEBUG("Can't find PPD file");
		return PT_ERR_FAIL;
	}

	char *filename = NULL;
	gboolean res = g_str_has_prefix(output, PPDC_PREFIX);
	if(res == TRUE) {
		size_t len = strlen(PPDC_PREFIX);
		filename = output+len;
		size_t sublen = strlen(filename);
		gboolean expected_suffix = g_str_has_suffix(filename, ".\n");
		if(expected_suffix == TRUE) {
			filename[sublen-2] = '\0';
		} else {
			PT_DEBUG("ppdc returned unexpected output:\n%s\n", output);
			free(output);
			return PT_ERR_FAIL;
		}
	} else {
		PT_DEBUG("ppdc returned unexpected output:\n%s\n", output);
		free(output);
		return PT_ERR_FAIL;
	}

	char *abspath = realpath(filename, NULL);
	if(!abspath) {
		PT_DEBUG("pathname canonicalization fails\n");
		free(output);
		return PT_ERR_FAIL;
	}

	if(strlen(abspath) >= PT_MAX_LENGTH) {
		PT_DEBUG("ppd filename too long\n");
		free(output);
		return PT_ERR_FAIL;
	}

	memset(printer->ppd, '\0', PT_MAX_LENGTH);

	strncpy(printer->ppd, abspath, PT_MAX_LENGTH-1);
	free(abspath);
	free(output);
	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}
