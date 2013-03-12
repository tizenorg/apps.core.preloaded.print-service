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
#include <dirent.h>

#include "pt_debug.h"
#include "pt_common.h"
#include "pt_ppd.h"

#define GETPPD "/usr/bin/getdrv"
#define CUPS_PPDDIR "/opt/etc/cups/ppd/"
#define SAMSUNG_DRV " -i /opt/etc/cups/ppd/samsung/samsung.drv"
#define SAMSUNG_DRV_GZ "/opt/etc/cups/ppd/samsung/samsung.drv.gz"
#define HP_DRV " -i /opt/etc/cups/ppd/hp/hp.drv"
#define HP_DRV_GZ "/opt/etc/cups/ppd/hp/hp.drv.gz"
#define EPSON_DRV " -i /opt/etc/cups/ppd/epson/epson.drv"
#define EPSON_DRV_GZ "/opt/etc/cups/ppd/epson/epson.drv.gz"
#define SAMSUNG_PPD_DIR "/opt/etc/cups/ppd/samsung"
#define EPSON_PPD_DIR "/opt/etc/cups/ppd/epson"
#define HP_PPD_DIR "/opt/etc/cups/ppd/hp"

/*supported manufacturer printer*/
const char *manufacturer[MANUFACTURER_NUM] = {"Samsung", "Hp", "Epson"};

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

	FILE *getdrv = NULL;
	FILE *result = NULL;

	char *getppd_argv[3];
	char model_name[PT_MAX_LENGTH] = {0,};
	char gunzip_drv[PT_MAX_LENGTH] = {0,};
	char *mfg_path = NULL;

	snprintf(model_name, PT_MAX_LENGTH, "-r \"%s\"", printer->mdl);
	getppd_argv[0] = model_name;

	if (strncasecmp(printer->mfg, "Samsung", 7) == 0) {
		mfg_path = "samsung";
		getppd_argv[1] = SAMSUNG_DRV;
		if (chdir(SAMSUNG_PPD_DIR)) {
			PT_DEBUG("Failed to change directory");
			return PT_ERR_FAIL;
		}
		if (access(SAMSUNG_DRV, F_OK) != 0) {
			// Can't find drv file
			snprintf(gunzip_drv, PT_MAX_LENGTH, "gzip -d %s", SAMSUNG_DRV_GZ);
			result = popen(gunzip_drv, "r");
			if (result == NULL) {
				PT_DEBUG("%s is failed to popen error",gunzip_drv);
				return PT_ERR_FAIL;
			}
		}
	} else if (strncasecmp(printer->mfg, "Hp", 2) == 0) {
		mfg_path = "hp";
		getppd_argv[1] = HP_DRV;
		if (chdir(HP_PPD_DIR)) {
			PT_DEBUG("Failed to change directory");
			return PT_ERR_FAIL;
		}
		if (access(HP_DRV, F_OK) != 0) {
			// Can't find drv file
			snprintf(gunzip_drv, PT_MAX_LENGTH, "gzip -d %s", HP_DRV_GZ);
			result = popen(gunzip_drv, "r");
			if (result == NULL) {
				PT_DEBUG("%s is failed to popen error",gunzip_drv);
				return PT_ERR_FAIL;
			}
		}
	} else if (strncasecmp(printer->mfg, "Epson", 5) == 0) {
		mfg_path = "epson";
		getppd_argv[1] = EPSON_DRV;
		if (chdir(EPSON_PPD_DIR)) {
			PT_DEBUG("Failed to change directory");
			return PT_ERR_FAIL;
		}
		if (access(EPSON_DRV, F_OK) != 0) {
			// Can't find drv file
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

	PT_DEBUG("/usr/bin/getdrv %s%s",getppd_argv[0], getppd_argv[1]);
	getppd_argv[2] = NULL;

	char temp[PT_MAX_LENGTH] = {0,};
	snprintf(temp, PT_MAX_LENGTH, "/usr/bin/getdrv %s%s",getppd_argv[0], getppd_argv[1]);

	getdrv = popen(temp, "r");
	PT_RETV_IF(getdrv == NULL, PT_ERR_FAIL, "Can't popen() getdrv");

	char output[PT_MAX_LENGTH] = {0};
	while (fgets(temp, PT_MAX_LENGTH, getdrv) != NULL) {
		strncpy(output, temp, sizeof(output));
	}
	pclose(getdrv);

	if (strstr(output, "ppdc: Writing ./")) {
		snprintf(printer->ppd, MAX_PATH_SIZE, "%s/%s/%s", PPD_DIR, mfg_path, temp+strlen("ppdc: Writing ./"));
		*strrchr(printer->ppd,'.') = '\0'; /* remove the last '.' returned by getdrv */
	} else {
		PT_DEBUG("getdrv returned an error: %s", temp);
		return PT_ERR_FAIL;
	}

	/*check the specified ppd file whether it is exist*/
	FILE *fd = NULL;
	fd = fopen(printer->ppd, "r");
	PT_RETV_IF(fd == NULL, PT_ERR_FAIL, "Specified PPD file doesn't exist!");
	fclose(fd);

	PT_DEBUG("Succeed to get ppd[%s] by getdrv", printer->ppd);

	PRINT_SERVICE_FUNC_LEAVE;
	return PT_ERR_NONE;
}
