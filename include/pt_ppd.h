/*
*	Printservice
*
* Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*					http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

/* For multi-user support */
#include <tzplatform_config.h>

#ifndef __PT_PPD_H__
#define __PT_PPD_H__

#define MAX_PATH_SIZE		 512
#define MAX_COMMAND_SIZE	 128*128
#define MANUFACTURER_NUM	 3
#define PPD_DIR				 tzplatform_mkpath3(TZ_SYS_ETC, "cups", "ppd")

/**
 *  This function let the app get ppd file for the specified printer
 *  @return    If success, return PT_ERR_NONE, else return PT_ERR_FAIL
 *  @param[in] ppd the pointer to the printer's ppd path
 *  @param[in] printer the printer entry
 */
int pt_get_printer_ppd(pt_printer_mgr_t *printer);
ppd_size_t *pt_utils_paper_size_pts(const char *name);

#endif /*__PT_PPD_H__*/

