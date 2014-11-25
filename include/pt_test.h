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

#ifndef __PRINT_TEST_H__
#define __PRINT_TEST_H__

#include <cups/cups.h>
#include <cups/ppd.h>
#include "pt_api.h"
#include "pt_utils.h"

#define TEST_MAX_GROUPS 10
#define ITER_MAX_VAL_IN_GROUP 100

struct _option_list {
	ppd_option_t option;
	struct _option_list *next;
	struct _option_list *prev;
	struct _option_list *first;
	struct _option_list *last;
};

typedef struct _option_list option_list;

struct _state {
	int quality;
	int paper;
	int grayscale;
	int papersize;
};

typedef struct _state state;

struct _counter {
	int id;
	char *desc;
	int current;
	int min;
	int max;
};

typedef struct _counter counter;

/* Open ppd file which additional checks */
ppd_file_t *open_ppd_file(const char *filename);

/* Save current state of choices */
state save_state();

/* Print state on STDOUT */
void print_state(state st);

/* Compare old state with current state */
void check_state(counter *array, int count);

/* Revert choice of selected option to previous state */
void restore_state(state st, counter *option);

/* Run all checks */
void check_all();

/* Check that every option has selected choice */
void check_selected_choice();

/* Check that at least one selected choice is enabled */
void check_selected_choice_is_enabled();

/* Check that at all choices in option are disabled */
bool check_all_choices_disabled();

/* Check that at all choices in option papersize are disabled */
int check_all_choices_disabled_papersize();

/* Check that at all choices in option grayscale are disabled */
int check_all_choices_disabled_grayscale();

/* Check that at all choices in option quality are disabled */
int check_all_choices_disabled_quality();

/* Check that at all choices in option paper are disabled */
int check_all_choices_disabled_paper();

void set_custom_choice(int qual, int paper, int gr_scale, int paper_size);

/* Reliable way to set choice */
bool reliable_set_choice(int id, int choice);

/* Get current selected choices of OptionMapping */
state get_current_state();

/* Print groups after current */
void print_groups_after(int num);

/* Print groups before current */
void print_groups_before(int num);

#endif /* __PRINT_TEST_H__ */
