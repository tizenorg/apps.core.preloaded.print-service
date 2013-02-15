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

#include <stdbool.h>
#include <cups/cups.h>
#include <cups/ppd.h>
#include <pt_debug.h>
#include <pt_test.h>

#define GR "\E[32m"
#define RD "\E[31m"
#define YE "\E[33m"
#define RS "\E[0m"
#define BOLD ""
#define RS_BOLD ""

static int REAL_PAPERSIZE_MAX;

static counter X[] = {
	{ .id = PT_OPTION_ID_QUALITY, .desc = "QL", .current = PT_QUALITY_DRAFT,.min = PT_QUALITY_DRAFT, .max = PT_QUALITY_ANY },
	{ .id = PT_OPTION_ID_PAPERSIZE, .desc = "PS" , .current = PT_PAPER_A4, .min = PT_PAPER_A4, .max = PT_PAPER_SIZE_MAX },
	{ .id = PT_OPTION_ID_GRAYSCALE,.desc = "GL", .current = PT_GRAYSCALE_GRAYSCALE, .min = PT_GRAYSCALE_GRAYSCALE, .max = PT_GRAYSCALE_ANY },
	{ .id = PT_OPTION_ID_PAPER, .desc = "PP", .current = PT_PAPER_NORMAL, .min = PT_PAPER_NORMAL, .max = PT_PAPER_ANY },
	{NULL, NULL, NULL, NULL, NULL}
};

const static int N = (sizeof(X) / sizeof(counter)) - 1; // Remove {NULL} value

bool reliable_set_choice(int id, int choice) {
	int old_choice = -1, new_choice = -1, result = -1;
	old_choice = pt_get_selected(id);
	result = pt_set_choice(id, choice);
	new_choice = pt_get_selected(id);

	if ((old_choice == -1) ||
		(result == -1) ||
		(new_choice == -1))
	{
		PT_DEBUG("OptionMapping:");
		PT_DEBUG("Internal Error");
		exit(EXIT_FAILURE);
	}

	if ((result == choice) &&
		(result == new_choice)) // choice is enabled
	{
		return true;
	} else if ((old_choice == result) &&
			   (result == new_choice)) { //choice is disabled
		return false;
	} else { //OM internal error
		PT_DEBUG("OptionMapping:");
		PT_DEBUG("[Error] pt_set_choice doesn't work correctly");
		exit(EXIT_FAILURE);
	}
}

void check_state(counter *array, int count) {
	if (count == N)
	{
		check_all();
	} else {
		int i;
		for (i = array[count].min; i < array[count].max; ++i)
		{
			int old_choice = -1, new_choice = -1, result = -1;
			bool set_choice = false;
			array[count].current = i;
			state st = save_state();
			print_groups_before(count);

			set_choice = reliable_set_choice(array[count].id, i);
			if (set_choice)
			{
				printf(YE "%s: " RS, array[count].desc);
				printf(GR "%d " RS, array[count].current);
				print_groups_after(count);
				check_state(array, ++count);
			} else {
				printf(YE "%s: " RS, array[count].desc);
				printf(RD "%d " RS, i);
				print_groups_after(count);
				continue;
			}
			restore_state(st, &array[--count]);
		}
	}
}

void swap(int a,int b)
{
	counter t=X[a];
	X[a]=X[b];
	X[b]=t;
}

void generate(int k)
{
	static int cnt = 0;
	if (k==N)
	{
		cnt++;
		int i;
		state st;
		const char* string = BOLD YE "%s %s %s %s\n" RS RS_BOLD;
		printf("GROUP %d [DEFAULT VALUES]\n", cnt);
		st = get_current_state();
		// FIXME It's hack! As order of GROUPS can be changed in future
		printf(YE "%d %d %d %d\n" RS, st.quality,st.papersize,st.grayscale,st.paper);
		printf("\n");
		check_state(X, 0);
	}
	else
	{
		int j;
		for(j=k;j<N;j++)
		{
			swap(k,j);
			generate(k+1);
			swap(k,j);
		}
	}
}

void init_groups(counter *array) {
	int cnt = 0;
	while(array[cnt].desc) {
		if (!strcmp(array[cnt].desc, "QL"))
		{
			array[cnt].current = pt_get_selected(PT_OPTION_ID_QUALITY);
		} else if (!strcmp(array[cnt].desc, "PS"))
		{
			array[cnt].max = pt_get_print_option_papersize_num();
			REAL_PAPERSIZE_MAX = array[cnt].max;
			if (REAL_PAPERSIZE_MAX < PT_PAPER_A4)
			{
				PT_DEBUG("[papersize] max value [%d] is out of the correct values: [%d-%d]",\
						REAL_PAPERSIZE_MAX, PT_PAPER_A4, PT_PAPER_SIZE_MAX);
				exit(EXIT_FAILURE);
			}
			array[cnt].current = pt_get_selected(PT_OPTION_ID_PAPERSIZE);
		} else if (!strcmp(array[cnt].desc, "GL"))
		{
			array[cnt].current = pt_get_selected(PT_OPTION_ID_GRAYSCALE);
		} else if (!strcmp(array[cnt].desc, "PP"))
		{
			array[cnt].current = pt_get_selected(PT_OPTION_ID_PAPER);
		} else {
			printf("Unknown description of option!");
			exit(EXIT_FAILURE);
		}
		cnt++;
	}
}



int main(int argc, char** argv) {
	ppd_file_t *ppd;
	option_list *option_list;
	if (!argv[1]) {
		PT_DEBUG("No any ppd file");
		exit(EXIT_FAILURE);
	}
	ppd = open_ppd_file(argv[1]);
	pt_parse_options(ppd);
	init_groups(X);

	// Print current state of OptionMapping
	// before any manipulation
	state st = get_current_state();
	printf(BOLD YE "DEFAULT:\n" RS RS_BOLD);

	printf(BOLD YE "%s %s %s %s\n" RS RS_BOLD, X[0].desc,X[1].desc,X[2].desc,X[3].desc);
	// FIXME It's hack! As order of GROUPS can be changed in future
	printf(YE "%d %d %d %d\n" RS, st.quality,st.papersize,st.grayscale,st.paper);
	generate(0);
	exit(EXIT_SUCCESS);
}

state save_state() {
	state st;
	st.quality = pt_get_selected(PT_OPTION_ID_QUALITY);
	st.paper = pt_get_selected(PT_OPTION_ID_PAPER);
	st.grayscale = pt_get_selected(PT_OPTION_ID_GRAYSCALE);
	st.papersize = pt_get_selected(PT_OPTION_ID_PAPERSIZE);

	PT_DEBUG("SAVED:");
	print_state(st);

	if ((st.quality == -1) || (st.paper == -1) ||
		(st.grayscale == -1) || (st.papersize == -1) )
	{
		PT_DEBUG("Returned value not correct:");
		PT_DEBUG("[GRAYSCALE]: %d", st.grayscale);
		PT_DEBUG("[QUALITY]: %d", st.quality);
		PT_DEBUG("[PAPER]: %d", st.paper);
		PT_DEBUG("[PAPERSIZE]: %d", st.papersize);
		exit(EXIT_FAILURE);
	}
	return st;
}

state get_current_state() {
	state st;
	st.quality = pt_get_selected(PT_OPTION_ID_QUALITY);
	st.paper = pt_get_selected(PT_OPTION_ID_PAPER);
	st.grayscale = pt_get_selected(PT_OPTION_ID_GRAYSCALE);
	st.papersize = pt_get_selected(PT_OPTION_ID_PAPERSIZE);

	if ((st.quality < PT_QUALITY_DRAFT) || (st.quality >= PT_QUALITY_ANY))
	{
		PT_DEBUG("[quality] choice [%d] is out of the correct values: [%d-%d]",\
				st.quality, PT_QUALITY_DRAFT, (PT_QUALITY_ANY-1));
		exit(EXIT_FAILURE);
	}
	if ((st.paper < PT_PAPER_NORMAL) || (st.paper >= PT_PAPER_ANY))
	{
		PT_DEBUG("[paper] choice [%d] is out of the correct values: [%d-%d]",\
				st.paper, PT_PAPER_NORMAL, (PT_PAPER_ANY-1));
		exit(EXIT_FAILURE);
	}
	if ((st.grayscale < PT_GRAYSCALE_GRAYSCALE) || (st.grayscale >= PT_GRAYSCALE_ANY))
	{
		PT_DEBUG("[grayscale] choice [%d] is out of the correct values: [%d-%d]",\
				st.grayscale, PT_GRAYSCALE_GRAYSCALE, (PT_GRAYSCALE_ANY-1));
		exit(EXIT_FAILURE);
	}
	if ((st.papersize < PT_PAPER_A4) || (st.grayscale >= REAL_PAPERSIZE_MAX))
	{
		PT_DEBUG("[papersize] choice [%d] is out of the correct values: [%d-%d]",\
				st.papersize, PT_PAPER_A4, (REAL_PAPERSIZE_MAX-1));
		exit(EXIT_FAILURE);
	}
	return st;
}

void print_cur_state() {
	state st;
	st.quality = pt_get_selected(PT_OPTION_ID_QUALITY);
	st.paper = pt_get_selected(PT_OPTION_ID_PAPER);
	st.grayscale = pt_get_selected(PT_OPTION_ID_GRAYSCALE);
	st.papersize = pt_get_selected(PT_OPTION_ID_PAPERSIZE);
	PT_DEBUG("CURRENT STATE");
	print_state(st);
}

void print_groups_before(int num) {
	int val = 0;
	while((val < num) && (X[val].desc != NULL)) {
		printf(YE "%s: " RS, X[val].desc);
		printf(GR "%d: " RS, X[val].current);
		val++;
	}
}

void print_groups_after(int num) {
	//int cnt = num;
	while(X[++num].desc) {
		printf(YE "%s: " RS, X[num].desc);
		printf("%d: ", X[num].current);
	}
	printf("\n");
}

void print_state(state st) {
	PT_DEBUG("[GRAYSCALE]: %d", st.grayscale);
	PT_DEBUG("[QUALITY]: %d", st.quality);
	PT_DEBUG("[PAPER]: %d", st.paper);
	PT_DEBUG("[PAPERSIZE]: %d, %s", st.papersize, \
		pt_get_print_option_papersize(st.papersize));
	// TEST
	int i = 0;
	for (i = 0; i < pt_get_print_option_papersize_num(); ++i)
	{
		PT_DEBUG("TEST [PAPERSIZE]: %d, %s", i, \
		pt_get_print_option_papersize(i));
	}
}

void restore_state(state st, counter *group) {
	int set_choice = false;
	pt_print_option_e option = group->id;

	switch (option) {
		case PT_OPTION_ID_QUALITY:
			set_choice = reliable_set_choice(option, st.quality);
			group->current = st.quality;
			break;
		case PT_OPTION_ID_PAPER:
			set_choice = reliable_set_choice(option, st.paper);
			group->current = st.paper;
			break;
		case PT_OPTION_ID_PAPERSIZE:
			set_choice = reliable_set_choice(option, st.papersize);
			group->current = st.papersize;
			break;
		case PT_OPTION_ID_GRAYSCALE:
			set_choice = reliable_set_choice(option, st.grayscale);
			group->current = st.grayscale;
			break;
		default:
			PT_DEBUG("[Error] Got unknown state");
			exit(EXIT_FAILURE);
	}

	if (!set_choice)
	{
		PT_DEBUG("Can't restore choice back");
		exit(EXIT_FAILURE);
	}
	PT_DEBUG("RESTORED:");
	print_state(st);
}

void check_all() {
	check_selected_choice();
	check_selected_choice_is_enabled();
	check_all_choices_disabled();
 }

void check_selected_choice() {
	if(!pt_selected_choice(PT_OPTION_ID_PAPERSIZE)) {
		PT_DEBUG("No any selected choice in PT_OPTION_ID_PAPERSIZE option");
		exit(EXIT_FAILURE);
	}
	if(!pt_selected_choice(PT_OPTION_ID_QUALITY)) {
		PT_DEBUG("No any selected choice in PT_OPTION_ID_QUALITY option");
		exit(EXIT_FAILURE);
	}
	if(!pt_selected_choice(PT_OPTION_ID_PAPER)) {
		PT_DEBUG("No any selected choice in PT_OPTION_ID_PAPER option");
		exit(EXIT_FAILURE);
	}
	if(!pt_selected_choice(PT_OPTION_ID_GRAYSCALE)) {
		PT_DEBUG("No any selected choice in PT_OPTION_ID_GRAYSCALE option");
		exit(EXIT_FAILURE);
	}
}

void check_selected_choice_is_enabled() {
	// TEST PAPERSIZE
	int choice = -1;
	choice = pt_get_selected(PT_OPTION_ID_PAPERSIZE);
	if(choice == -1) {
		PT_DEBUG("function pt_get_selected returned -1");
		exit(EXIT_FAILURE);
	}
	if(!pt_is_enabled(PT_OPTION_ID_PAPERSIZE, choice)) {
		PT_DEBUG("We've got selected choice which is not enabled");
		exit(EXIT_FAILURE);
	}

	// TEST QUALITY
	choice = -1;
	choice = pt_get_selected(PT_OPTION_ID_QUALITY);
	if(choice == -1) {
		PT_DEBUG("function pt_get_selected returned -1");
		exit(EXIT_FAILURE);
	}
	if(!pt_is_enabled(PT_OPTION_ID_QUALITY, choice)) {
		PT_DEBUG("We've got selected choice which is not enabled");
		exit(EXIT_FAILURE);
	}

	// TEST PAPER
	choice = -1;
	choice = pt_get_selected(PT_OPTION_ID_PAPER);
	if(choice == -1) {
		PT_DEBUG("function pt_get_selected returned -1");
		exit(EXIT_FAILURE);
	}
	if(!pt_is_enabled(PT_OPTION_ID_PAPER, choice)) {
		PT_DEBUG("We've got selected choice which is not enabled");
		exit(EXIT_FAILURE);
	}

	// TEST GRAYSCALE
	choice = -1;
	choice = pt_get_selected(PT_OPTION_ID_GRAYSCALE);
	if(choice == -1) {
		PT_DEBUG("function pt_get_selected returned -1");
		exit(EXIT_FAILURE);
	}
	if(!pt_is_enabled(PT_OPTION_ID_GRAYSCALE, choice)) {
		PT_DEBUG("We've got selected choice which is not enabled");
		exit(EXIT_FAILURE);
	}
}

void check_all_choices_disabled() {
	bool papersize = true, quality = true;
	bool grayscale = true, paper = true;
	bool general = true;
	papersize  = check_all_choices_disabled_papersize();
	grayscale = check_all_choices_disabled_grayscale();
	quality =  check_all_choices_disabled_quality();
	paper =  check_all_choices_disabled_paper();
	if (papersize || quality || grayscale || paper)	{
		PT_DEBUG("TEST [check_all_choices_disabled] failed");
	} else {
		general = false;
	}
	return (general);
}

int check_all_choices_disabled_papersize() {
	bool enabled = false;
	int i;
	for (i = PT_PAPER_A4; i < PT_PAPER_SIZE_MAX; ++i)
	{
		if (pt_is_enabled(PT_OPTION_ID_PAPERSIZE, i))
		{
			enabled = true;
		}
	}
	if (!enabled)
	{
		PT_DEBUG("All choices are DISABLED in PAPERSIZE");
		exit(EXIT_FAILURE);
	}
	return (!enabled);
}

int check_all_choices_disabled_grayscale() {
	bool enabled = false;
	int i;
	for (i = PT_GRAYSCALE_GRAYSCALE; i < PT_GRAYSCALE_MAX; ++i)
	{
		if (pt_is_enabled(PT_OPTION_ID_GRAYSCALE, i))
		{
			enabled = true;
		}
	}
	if (!enabled)
	{
		PT_DEBUG("All choices are DISABLED in GRAYSCALE");
		exit(EXIT_FAILURE);
	}
	return (!enabled);
}

int check_all_choices_disabled_quality() {
	bool enabled = false;
	int i;
	for (i = PT_QUALITY_DRAFT; i < PT_QUALITY_MAX; ++i)
	{
		if (pt_is_enabled(PT_OPTION_ID_QUALITY, i))
		{
			enabled = true;
		}
	}
	if (!enabled)
	{
		PT_DEBUG("All choices are DISABLED in QUALITY");
		exit(EXIT_FAILURE);
	}
	return (!enabled);
}

int check_all_choices_disabled_paper() {
	bool enabled = false;
	int i;
	for (i = PT_PAPER_NORMAL; i < PT_PAPER_MAX; ++i)
	{
		if (pt_is_enabled(PT_OPTION_ID_PAPER, i))
		{
			enabled = true;
		}
	}
	if (!enabled)
	{
		PT_DEBUG("All choices are DISABLED in PAPER");
		exit(EXIT_FAILURE);
	}
	return (!enabled);
}

void set_custom_choice(int qual, int paper, int gr_scale, int paper_size) {
	pt_set_choice(PT_OPTION_ID_PAPERSIZE, paper);
	check_all();
	pt_set_choice(PT_OPTION_ID_QUALITY, qual);
	check_all();
	pt_set_choice(PT_OPTION_ID_GRAYSCALE, gr_scale);
	check_all();
	pt_set_choice(PT_OPTION_ID_PAPERSIZE, paper_size);
}

ppd_file_t* open_ppd_file(const char *filename) {
	ppd_file_t *ppd = ppdOpenFile(filename);
	if (!ppd)
	{
		PT_DEBUG("PPD file can't be loaded");
		exit(EXIT_FAILURE);
	}
	return ppd;
}