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

#ifndef PT_OPTIONMAPPING_H
#define PT_OPTIONMAPPING_H

#include "pt_debug.h"
#include "pt_common.h"

typedef struct {
	const char *keyword;
	int quality;
	int papertype;
	int grayscale;
	int duplex;
} pt_choice_keyword;

typedef struct {
	const char *key;
	void (*fn)(ppd_option_t *op);
	const pt_choice_keyword *keywords;
} KeyActions;

typedef enum _pt_resolution_t {
	PT_RESOLUTION_LOWEST,
	PT_RESOLUTION_LOW,
	PT_RESOLUTION_STANDARD,
	PT_RESOLUTION_HIGH,
	PT_RESOLUTION_MAX
} pt_resolution_t;

typedef struct {
	const char *keyword;
	pt_resolution_t weight;
} pt_resolution_keyword;

#undef PT_OPTIONCUBE_TEST_PRINT
#define PT_USER_OPTION_CONFIG_FILE tzplatform_mkpath(TZ_SYS_ETC, "cups/ppd/settings.cfg")

ppd_choice_t *pt_selected_choice(int op, pt_orientation_e p);
void pt_parse_options(ppd_file_t *ppd);
void pt_save_user_choice(void);

#endif // PT_OPTIONMAPPING_H
