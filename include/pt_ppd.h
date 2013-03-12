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

#ifndef __PT_PPD_H__
#define __PT_PPD_H__

#define MAX_PATH_SIZE		 512
#define MAX_COMMAND_SIZE	 128*128
#define MANUFACTURER_NUM	 3
#define PPD_DIR				"/opt/etc/cups/ppd"

/**
 *  This function let the app get ppd file for the specified printer
 *  @return    If success, return PT_ERR_NONE, else return PT_ERR_FAIL
 *  @param[in] ppd the pointer to the printer's ppd path
 *  @param[in] printer the printer entry
 */
int pt_get_printer_ppd(pt_printer_mgr_t *printer);

#endif /*__PT_PPD_H__*/

