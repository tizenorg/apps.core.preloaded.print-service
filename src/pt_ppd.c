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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "pt_debug.h"
#include "pt_common.h"
#include "pt_ppd.h"

#define GETPPD "/usr/bin/getppd"
#define CUPS_PPDDIR "/opt/etc/cups/ppd/"
#define SAMSUNG_DRV "/opt/etc/cups/ppd/samsung/samsung.drv"
#define SAMSUNG_DRV_GZ "/opt/etc/cups/ppd/samsung/samsung.drv.gz"
#define SAMSUNG_DRV_GZ_ORG "/usr/share/cups/ppd/samsung/samsung.drv.gz"
#define HP_DRV "/opt/etc/cups/ppd/hp/hp.drv"
#define HP_DRV_GZ "/opt/etc/cups/ppd/hp/hp.drv.gz"
#define HP_DRV_GZ_ORG "/usr/share/cups/ppd/hp/hp.drv.gz"
#define EPSON_DRV "/opt/etc/cups/ppd/epson/epson.drv"
#define EPSON_DRV_GZ "/opt/etc/cups/ppd/epson/epson.drv.gz"
#define EPSON_DRV_GZ_ORG "/usr/share/cups/ppd/epson/epson.drv.gz"
#define SAMSUNG_PPD_DIR "/opt/etc/cups/ppd/samsung"
#define EPSON_PPD_DIR "/opt/etc/cups/ppd/epson"
#define HP_PPD_DIR "/opt/etc/cups/ppd/hp"

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

	FILE *getppd = NULL;
	FILE *result = NULL;

	char *getppd_argv[3];
	char model_name[PT_MAX_LENGTH] = {0,};
	char gunzip_drv[PT_MAX_LENGTH] = {0,};
	char *mfg_path = NULL;

	snprintf(model_name, PT_MAX_LENGTH, "-m \"%s\"", printer->mdl);
	getppd_argv[0] = model_name;

	if (strncasecmp(printer->mfg, "Samsung", 7) == 0) {
		mfg_path = "samsung";
		getppd_argv[1] = " -i "SAMSUNG_DRV;
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
		}
	} else if (strncasecmp(printer->mfg, "Hp", 2) == 0) {
		snprintf(model_name, PT_MAX_LENGTH, "-a \"%s\"", printer->mdl);
		getppd_argv[0] = model_name;
		mfg_path = "hp";
		getppd_argv[1] = " -i "HP_DRV;
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
		}
	} else if (strncasecmp(printer->mfg, "Epson", 5) == 0) {
		mfg_path = "epson";
		getppd_argv[1] = " -i "EPSON_DRV;
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
		}
	} else {
		PT_DEBUG("Can't find PPD file");
		return PT_ERR_FAIL;
	}

	if (result != NULL) {
		pclose(result);
	}

	PT_DEBUG("/usr/bin/getppd %s%s",getppd_argv[0], getppd_argv[1]);
	getppd_argv[2] = NULL;

	char temp[PT_MAX_LENGTH] = {0,};
	snprintf(temp, PT_MAX_LENGTH, "/usr/bin/getppd %s%s",getppd_argv[0], getppd_argv[1]);

	getppd = popen(temp, "r");
	PT_RETV_IF(getppd == NULL, PT_ERR_FAIL, "Can't popen() getppd");

	char output[PT_MAX_LENGTH] = {0};
	while (fgets(temp, PT_MAX_LENGTH, getppd) != NULL) {
		strncpy(output, temp, sizeof(output));
	}
	pclose(getppd);

	if (strstr(output, "ppdc: Writing ./")) {
		snprintf(printer->ppd, MAX_PATH_SIZE, "%s/%s/%s", PPD_DIR, mfg_path, temp+strlen("ppdc: Writing ./"));
		*strrchr(printer->ppd,'.') = '\0'; /* remove the last '.' returned by getppd */
	} else {
		PT_DEBUG("getppd returned an error: %s", temp);
		PT_DEBUG("output: %s", output);
		return PT_ERR_FAIL;
	}

	/*check the specified ppd file whether it is exist*/
	FILE *fd = NULL;
	fd = fopen(printer->ppd, "r");
	PT_RETV_IF(fd == NULL, PT_ERR_FAIL, "Specified PPD file doesn't exist!");
	fclose(fd);

	PT_DEBUG("Succeed to get ppd[%s] by getppd", printer->ppd);

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}
