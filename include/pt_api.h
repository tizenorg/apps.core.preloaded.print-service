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

#ifndef __PRINT_SERVICE_API_H__
#define __PRINT_SERVICE_API_H__

#ifndef API
#define API __attribute__ ((visibility("default")))
#endif

#include <eina_list.h>
#define PT_MAX_LENGTH		512

typedef enum _pt_err_e {
	PT_ERR_NONE = 0,							/** success */
	PT_ERR_FAIL,								/** operate fail */
	PT_ERR_INVALID_PARAM,						/** invalid parameter */
	PT_ERR_NO_MEMORY,							/** no memory */
	PT_ERR_NOT_NETWORK_ACCESS,					/** network is not available*/
	PT_ERR_NOT_USB_ACCESS,						/** USB is not available */
	PT_ERR_NO_DEFAULT_PRINTER,					/** no default printer */
	PT_ERR_SET_OPTION_FAIL,						/** option is not fit for the printer */
	PT_ERR_INVALID_USER_CB,						/** not USB access */
	PT_ERR_UNSUPPORTED, 						/** unsupported */
	PT_ERR_UNKNOWN,								/** unknown error */
} pt_err_e;

typedef enum _pt_connection_type_e {
	PT_CONNECTION_USB =0x01,						/** usb mode*/
	PT_CONNECTION_WIFI =0x02,						/** wifi mode */
	PT_CONNECTION_WIFI_DIRECT=0x04,				/** wifi direct mode */
} pt_connection_type_e;

typedef enum _pt_duplex_e {
	PT_DUPLEX_OFF = 0,
	PT_DUPLEX_ON,
} pt_duplex_e;

typedef enum _pt_collate_e {
	PT_COLLATE_OFF = 0,
	PT_COLLATE_ON,
} pt_collate_e;

typedef enum _pt_paper_size_e {
	PT_PAPER_A4 = 0,                           		/** A4:                 210.0 mm x 297.0 mm */
	PT_PAPER_L,                                 		/** L :                 89.0 mm x 127.0 mm */
	PT_PAPER_2L,                                		/** 2L:                 127.0 mm x 178.0 mm */
	PT_PAPER_HAGAKI_POSTCARD,                   /** Hagaki Postcard:    100.0 mm x 148.0 mm */
	PT_PAPER_BUISNESSCARD,                      	/** Buisness card:      54.0 mm x 85.6 mm */
	PT_PAPER_4X6,                              	 	/** 4" x 6":            101.6 mm x 152.4 mm */
	PT_PAPER_8X10,                              		/** 8" x 10":           203.2 mm x 254.0 mm */
	PT_PAPER_LETTER,                            		/** Letter:             216.0 mm x 279.4 mm */
	PT_PAPER_11X17,                             		/** 11" x 17":          279.4 mm x 431.8 mm */
	PT_PAPER_SIZE_MAX,
} pt_paper_size_e;

typedef enum _pt_scaling_e {
	PT_SCALING_FIT_TO_PAGE = 0,                 /** Fit to page */
//    PT_SCALING_ORIGINAL_SIZE,                   /** Original image size 100% */
	PT_SCALING_2_PAGES,                         /** 2 pages in 1 sheet */
	PT_SCALING_4_PAGES                          /** 4 pages in 1 sheet */
//    PT_SCALING_8_PAGES,                         /** 8 pages in 1 sheet */
} pt_scaling_e;

typedef enum _pt_image_size_option_e {
	PT_SIZE_FIT_TO_PAPER = 0,
	PT_SIZE_5X7,
	PT_SIZE_4X6,
	PT_SIZE_3_5X5,
	PT_SIZE_WALLET,
	PT_SIZE_CUSTOM,
	PT_SIZE_MAX,
} pt_image_size_option_e;

typedef enum _pt_orientation_e {
	//PT_ORIENTATION_AUTO = 0, 		                		/** Auto rotation */
	PT_ORIENTATION_PORTRAIT,						/** Portrait mode */
	PT_ORIENTATION_LANDSCAPE,						/** Landscape mode */
} pt_orientation_e;

typedef enum _pt_paper_e {
	PT_PAPER_NORMAL = 0, 		                /** Normal paper */
	PT_PAPER_GLOSSY,							/** Glossy paper */
	PT_PAPER_PHOTO,								/** Photographic paper */
	PT_PAPER_ANY,                               /** Any kind of paper - doesn't matter */
	PT_PAPER_MAX
} pt_papertype_e;

typedef enum _pt_quality_e {
	PT_QUALITY_DRAFT = 0, 		                /** Draft print */
	PT_QUALITY_STANDARD,						/** Standard */
	PT_QUALITY_HIGH,							/** High quality print */
	PT_QUALITY_ANY,                             /** Any quality - doesn't matter */
	PT_QUALITY_MAX
} pt_quality_e;

typedef enum _pt_range_e {
	PT_RANGE_ALL = 0, 			                /** Print all pages */
	PT_RANGE_CURRENT,							/** Print current page */
	PT_RANGE_MAX,
} pt_range_e;

typedef enum _pt_grayscale_e {
	PT_GRAYSCALE_GRAYSCALE = 0,					/** Grayscale print */
	PT_GRAYSCALE_COLOUR,	                    /** Coloured print */
//    PT_GRAYSCALE_BLACKNWHITE,					/** Black & White print */
	PT_GRAYSCALE_ANY,                           /** Color Or Grayscale - doesn't matter */
	PT_GRAYSCALE_MAX
} pt_grayscale_e;

typedef enum _pt_printer_state_e {
	PT_PRINTER_IDLE = 0,						/** printer is in idle mode */
	PT_PRINTER_BUSY,							/** printer is busy for printing */
	PT_PRINTER_STOPPED,							/** printer has stopped printing */
	PT_PRINTER_OFFLINE,							/** printer offline */
} pt_printer_state_e;

typedef enum _pt_job_state_e {
	PT_JOB_ERROR = -1,							/** invalid job status */
	PT_JOB_PENDING = 3,							/** waiting to be printed */
	PT_JOB_HELD,								/** held for printing */
	PT_JOB_PROCESSING,							/** currently printing */
	PT_JOB_STOPPED,								/** stopped */
	PT_JOB_CANCELED,							/** canceled */
	PT_JOB_ABORTED,								/** aborted due to error */
	PT_JOB_COMPLETED							/** completed successfully */
} pt_job_state_e;

typedef struct {
	Eina_List *printerlist;						/** printer list*/
	void  *userdata;							/** user data*/
} pt_response_data_t;

typedef enum _pt_event_e {
	PT_EVENT_JOB_ERROR,
	PT_EVENT_JOB_PENDING,
	PT_EVENT_JOB_STARTED,
	PT_EVENT_JOB_HELD,
	PT_EVENT_JOB_PROCESSING,
	PT_EVENT_JOB_PROGRESS,
	PT_EVENT_JOB_STOPPED,
	PT_EVENT_JOB_CANCELED,
	PT_EVENT_JOB_ABORTED,
	PT_EVENT_JOB_COMPLETED,
	PT_EVENT_ALL_THREAD_COMPLETED,
	PT_EVENT_PRINTER_OFFLINE,
	PT_EVENT_PRINTER_ONLINE,
	PT_EVENT_USB_PRINTER_OFFLINE,
	PT_EVENT_USB_PRINTER_ONLINE,
} pt_event_e;

typedef enum _pt_vendor_e {
	PT_VENDOR_EPSON,
	PT_VENDOR_GENERIC,
	PT_VENDOR_HP,
	PT_VENDOR_SAMSUNG,
	PT_VENDOR_MAX,
} pt_vendor_e;

typedef enum _pt_print_option_e {
	PT_OPTION_ID_PAPERSIZE = 0,
	PT_OPTION_ID_COPIES,
	PT_OPTION_ID_RANGE,
	PT_OPTION_ID_QUALITY,
	PT_OPTION_ID_PAPER,
	PT_OPTION_ID_GRAYSCALE,
	PT_OPTION_ID_MAX,
} pt_print_option_e;

typedef struct {
	int job_id;
	int progress;
	int page_printed;
} pt_progress_info_t;

typedef void (*get_printers_cb)(pt_response_data_t *resp);    /** callback function*/
typedef void (*pt_event_cb)(pt_event_e event, void *user_data, pt_progress_info_t *progress_info);    /** event callback function*/

#define NOTI_CONVERT_PDF_COMPLETE "mobileprint_convert_pdf_complete"
#define NOTI_CONVERT_PDF_ERROR "mobileprint_convert_pdf_error"
#define NOTI_USB_PRINTER_CHECK_ONLINE "mobileprint_usb_printer_online"
#define NOTI_USB_PRINTER_CHECK_OFFLINE "mobileprint_usb_printer_offline"

typedef struct {
	char keyword[PT_MAX_LENGTH];
	char value[PT_QUALITY_ANY][PT_MAX_LENGTH];
} pt_quality_value_t;

typedef struct {
	char keyword[PT_MAX_LENGTH];
	char value[PT_PAPER_ANY][PT_MAX_LENGTH];
} pt_paper_value_t;

typedef struct {
	char keyword[PT_MAX_LENGTH];
	char value[PT_GRAYSCALE_ANY][PT_MAX_LENGTH];
} pt_color_value_t;

typedef struct {
	char name[PT_MAX_LENGTH];					/** the description of the printer */
	char address[PT_MAX_LENGTH];				/** ip address */
	char mdl[PT_MAX_LENGTH];      				/* mdl name */
	char mfg[PT_MAX_LENGTH];       				/* mfg name */
	char ppd[PT_MAX_LENGTH];                    /** ppd file name (with full path) */
	int	authrequired;							/** reserved, the type of authentication required for printing:
												  *  0- none,
												  *  1- username,password,
												  *  2- domain,username,password,
												  *  4- negotiate
												  */
	int	support_color;									/** reserved
												  *  0- the printer unsupports color mode,
												  *  1- the printer supports color mode.
												  */
	int  support_duplex;								/** reserved
												  *  0- the printer unsupports duplex mode,
												  *  1- the printer supports duplex mode.
												  */
	pt_printer_state_e  status;				/** printer status */
	int actived;								/** actived printer */

	int	 copies;								/** print copy number */
	int            size;						/** paper size */
	pt_orientation_e landscape;					/** orientation */
	pt_scaling_e scaling;						/** scaling mode */
	pt_image_size_option_e imagesize;					/** image size */
	pt_range_e range;							/** printing range */
//	pt_grayscale_t grayscale;					/** colour mode */
//	pt_quality_t quality;						/** quality */
//	pt_paper_t paper;							/** paper type */
} pt_printer_mgr_t;

typedef struct {
	char name[PT_MAX_LENGTH];
	double x;
	double y;
} pt_pagesize_t;

typedef struct {
	pt_image_size_option_e imagesize;
	double		resolution_width;
	double		resolution_height;
	double		ratio;
	int			unit; // 1:cm 2:inch
} pt_imagesize_t;

/**
 *	This API let the app initialize the environment(eg.ip address) by type
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] type connection type enumerated in pt_connection_type
 *	@param[in] type event callback function
 */
API int pt_init(pt_event_cb evt_cb, void *user_data);

/**
 *	This API let the app finalize the environment
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 */
API int pt_deinit(void);

/**
 *	This API let the app get the printer list
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] callback the pointer to the function which will be excuted after search
 *	@param[in] userdata the pointer to the user data
 *	@code
	static void test_cb(pt_response_data_t *resp)
	{
		if (resp == NULL)
			return;

		if (NULL == resp->printerlist)
			return;

		Eina_List* cursor = NULL;
		pt_printer_mgr_t* printer = NULL;

		EINA_LIST_FOREACH(resp->printerlist,cursor,printer)
		{
				...
		}

		....
	}

	void main_test()
	{
		...
		pt_get_printers(test_cb, userdata);
		...
	}
 *	@endcode
 */
API int pt_get_printers(get_printers_cb callback, void *userdata);

/**
 *	This API let the app cancel getting the printer list
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 */
API int pt_cancel_get_printers();

/**
 *	This API let the app get the current default printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[out] printer the pointer to the printer object
 *	@code
	void main_test()
	{
		...
		pt_printer_mgr_t *pt;
		int ret = pt_get_default_printer(&pt);
		if(ret != PT_ERR_NONE)
			return;
		...
	}
 *	@endcode
 */
API int pt_get_default_printer(pt_printer_mgr_t **printer);

/**
 *	This API let the app set a printer as the default printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] printer the pointer to the printer object
 */
API int pt_set_default_printer(pt_printer_mgr_t *printer);

/**
 *	This API let the app get the current active printer
 *	allocates memory for printer info structure! Please free after use!
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[out] printer the pointer to the printer object
 */
int pt_get_active_printer(pt_printer_mgr_t **printer);

/**
 *	This API let the app select a specify printer as active printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] printer the pointer to the printer object
 */
API int pt_set_active_printer(pt_printer_mgr_t *printer);

/**
 *	This API let the app set copies option for active printer, if not find active printer, will use default printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] copies the copy number
 */
API int pt_set_print_option_copies(int copies);

/**
 *	This API let the app set paper option for active printer, if not find active printer, will use default printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] papersize the paper size
 */
API int pt_set_print_option_papersize(void);

/**
 *	This API let the app set orientation option for active printer, if not find active printer, will use default printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] orientation the orientation value(0:portrait, 1:landscape)
 */
API int pt_set_print_option_orientation(pt_orientation_e orientation);

/**
 *	This API let the app set image fit for the page size, if not find active printer, will use default printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 */
API int pt_set_print_option_imagesize(pt_imagesize_t *crop_image_info, const char *filepath, int res_x, int res_y);

/**
 *	This API let the app set image fit for the page size, if not find active printer, will use default printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 */
API int pt_set_print_option_scaleimage(pt_scaling_e scaling);

/**
 *	This API let the app set the color mode (if supported by printer)
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 */
API int pt_set_print_option_color(void);

/**
 *	This API let the app set the print quality (if supported by printer)
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 */
API int pt_set_print_option_quality(void);

/**
 *	This API let the app set the paper type (if supported by printer)
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 */
API int pt_set_print_option_paper_type(void);


/**
 *	This API let the app set the print range (from page - to page), if not find active printer, will use default printer
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 */
API int pt_set_print_option_print_range(int from, int to);

/**
 *	This API let the app start the print job on the active printer, if not find active printer, will use default printer
 *	now only support image and txt file,
 *	will support pdf,office,eml,html in the future
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] files is the print files path address
 *	@param[in] num is the print files number
 *	@param[out] jobid the job id
 */
API int pt_start_print(const char **files, int num);

/**
 *	This API let the app cancel the print job by jobid
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 *	@param[in] jobid the job id
 */
API int pt_cancel_print(int jobid);

/**
 *	This API let the app get the connect status
 *	@return   If success, return PT_ERR_CONNECTION_USB_ACCESS or PT_ERR_CONNECTION_WIFI_ACCESS,
 *	else return the other error code as defined in pt_err_t
 */
API int pt_get_connection_status(int *connection_type);

/**
 *	This API let the app get the job status
 *	@return   return job status
 *	@param[in] job id
 */
API pt_job_state_e pt_get_current_job_status(int jobid);

/**
 *	This API check the printer status
 *	@return   If success, return PT_ERR_NONE, else return the other error code as defined in pt_err_t
 */
API pt_printer_state_e pt_check_printer_status(pt_printer_mgr_t *printer);

/**
 *	This API returns the human-readable paper size. "papersize_num" is it's position in the PPD file.
 *	@return   The pointer to the string or NULL if problem
 */
API char *pt_get_print_option_papersize(int papersize_num);

/**
 *	This API returns the printer instruction for setting paper size. "papersize_num" is it's position in the PPD file.
 *	@return   The pointer to the string or NULL if problem
 */
API char *pt_get_print_option_papersize_cmd(int papersize_num);

/**
 *	This API returns the number of paper sizes in the PPD.
 */
API int pt_get_print_option_papersize_num(void);

/**
 *	This API returns the position of the default paper size in the PPD.
 */
//API int pt_get_print_option_default_papersize(void);

/**
 *	This API selects(marks) the choice number ch of the option op.
 */

API int pt_set_choice(int op, int ch);

/**
 *	This API returns the number of the selected choice of the option op.
 */
API int pt_get_selected(int op);

/**
 *	This API checks if the choice ch of the option op can be selected (is enabled).
 */
API int pt_is_enabled(int op, int ch);

/**
 *	This API returns the selected page size in points.
 */
API int pt_get_selected_paper_size_pts(pt_pagesize_t *s);

#endif /* __PRINT_SERVICE_API_H__ */
