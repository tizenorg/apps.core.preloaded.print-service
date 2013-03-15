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
//#include <glib/gprintf.h>
//#include <vconf.h>
//#include <vconf-keys.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <linux/types.h>
//#include <linux/netlink.h>
//#include <errno.h>
//#include <unistd.h>
//#include <arpa/inet.h>
//#include <netinet/in.h>
//#include <sys/ioctl.h>
//#include <app.h>

#include "pt_common.h"
#include "pt_debug.h"
#include "pt_optionmapping.h"

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
