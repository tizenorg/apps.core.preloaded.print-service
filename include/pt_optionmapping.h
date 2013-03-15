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

#ifndef PT_OPTIONMAPPING_H
#define PT_OPTIONMAPPING_H

#include "pt_debug.h"
#include "pt_common.h"

typedef struct {
	const char *keyword;
	int quality;
	int papertype;
	int grayscale;
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

#define PT_OPTIONCUBE_TEST_PRINT

ppd_choice_t *pt_selected_choice(int op);
void pt_parse_options(ppd_file_t *ppd);

#endif // PT_OPTIONMAPPING_H
