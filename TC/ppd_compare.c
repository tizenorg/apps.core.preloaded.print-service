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
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <cups/ppd.h>

typedef struct _cups_array_s cups_array_t;

static void print_ppd_stucture(ppd_file_t *ppdfile)
{
	int index =0;

	fprintf(stderr,"[%20s] %d\n","language_level", ppdfile->language_level);
	fprintf(stderr,"[%20s] %d\n","color_device", ppdfile->color_device);
	fprintf(stderr,"[%20s] %d\n","variable_sizes", ppdfile->variable_sizes);
	fprintf(stderr,"[%20s] %d\n","accurate_screens", ppdfile->accurate_screens);
	fprintf(stderr,"[%20s] %d\n","contone_only", ppdfile->contone_only);
	fprintf(stderr,"[%20s] %d\n","landscape", ppdfile->landscape);
	fprintf(stderr,"[%20s] %d\n","model_number", ppdfile->model_number);
	fprintf(stderr,"[%20s] %d\n","manual_copies", ppdfile->manual_copies);
	fprintf(stderr,"[%20s] %d\n","throughput", ppdfile->throughput);
	fprintf(stderr,"[%20s] %d\n","colorspace", ppdfile->colorspace);

	if (ppdfile->patches != NULL) {
		fprintf(stderr,"[%20s] %s\n","patches", ppdfile->patches);
	}

	fprintf(stderr,"[%20s] %d\n","num_emulations", ppdfile->num_emulations);

	if (ppdfile->emulations != NULL) {
		fprintf(stderr,"[%20s] %s\n","emulations", ppdfile->emulations->name);
	}

	if (ppdfile->jcl_begin != NULL) {
		fprintf(stderr,"[%20s] %s\n","jcl_begin", ppdfile->jcl_begin);
	}

	if (ppdfile->jcl_ps != NULL) {
		fprintf(stderr,"[%20s] %s\n","jcl_ps", ppdfile->jcl_ps);
	}

	if (ppdfile->jcl_end != NULL) {
		fprintf(stderr,"[%20s] %s\n","jcl_end", ppdfile->jcl_end);
	}

	if (ppdfile->lang_encoding != NULL) {
		fprintf(stderr,"[%20s] %s\n","lang_encoding", ppdfile->lang_encoding);
	}

	if (ppdfile->lang_version != NULL) {
		fprintf(stderr,"[%20s] %s\n","lang_version", ppdfile->lang_version);
	}

	if (ppdfile->modelname != NULL) {
		fprintf(stderr,"[%20s] %s\n","modelname", ppdfile->modelname);
	}

	if (ppdfile->ttrasterizer != NULL) {
		fprintf(stderr,"[%20s] %s\n","ttrasterizer", ppdfile->ttrasterizer);
	}

	if (ppdfile->manufacturer != NULL) {
		fprintf(stderr,"[%20s] %s\n","manufacturer", ppdfile->manufacturer);
	}

	if (ppdfile->product != NULL) {
		fprintf(stderr,"[%20s] %s\n","product", ppdfile->product);
	}

	if (ppdfile->nickname != NULL) {
		fprintf(stderr,"[%20s] %s\n","nickname", ppdfile->nickname);
	}

	if (ppdfile->shortnickname != NULL) {
		fprintf(stderr,"[%20s] %s\n","shortnickname", ppdfile->shortnickname);
	}

	fprintf(stderr,"[%20s] %d\n","num_groups", ppdfile->num_groups);

	if (ppdfile->groups != NULL) {
		fprintf(stderr,"[%20s] %s\n","groups", ppdfile->groups->text);
		fprintf(stderr,"[%20s] %s\n","groups", ppdfile->groups->name);
	}

	fprintf(stderr,"[%20s] %d\n","num_sizes", ppdfile->num_sizes);

	if (ppdfile->sizes != NULL) {
		fprintf(stderr,"[%20s] %s\n","sizes", ppdfile->sizes->name);
	}

	fprintf(stderr,"[%20s] %f %f\n","custom_min", ppdfile->custom_min[0], ppdfile->custom_min[0]);
	fprintf(stderr,"[%20s] %f %f\n","custom_max", ppdfile->custom_max[0], ppdfile->custom_max[1]);
	fprintf(stderr,"[%20s] %f %f %f %f\n","custom_margins" \
			, ppdfile->custom_margins[0], ppdfile->custom_margins[1] \
			, ppdfile->custom_margins[2], ppdfile->custom_margins[3]);
	fprintf(stderr,"[%20s] %d\n","num_consts", ppdfile->num_consts);

	if (ppdfile->consts != NULL) {
		fprintf(stderr,"[%20s] %s\n","consts", ppdfile->consts->choice1);
		fprintf(stderr,"[%20s] %s\n","consts", ppdfile->consts->choice2);
		fprintf(stderr,"[%20s] %s\n","consts", ppdfile->consts->option1);
		fprintf(stderr,"[%20s] %s\n","consts", ppdfile->consts->option2);
	}

	fprintf(stderr,"[%20s] %d\n","num_fonts", ppdfile->num_fonts);

	for (index = 0; index < ppdfile->num_fonts; index++) {
		if (ppdfile->fonts[index] != NULL) {
			fprintf(stderr,"[%20s] %s\n","fonts", ppdfile->fonts[index]);
		}
	}

	fprintf(stderr,"[%20s] %d\n","num_profiles", ppdfile->num_profiles);

	if (ppdfile->profiles != NULL) {
		fprintf(stderr,"[%20s] %s\n","profiles", ppdfile->profiles->media_type);
	}

	fprintf(stderr,"[%20s] %d\n","num_filters", ppdfile->num_filters);

	for (index = 0; index < ppdfile->num_filters; index++) {
		if (ppdfile->filters[index] != NULL) {
			fprintf(stderr,"[%20s] %s\n","filters", ppdfile->filters[index]);
		}
	}

	fprintf(stderr,"[%20s] %d\n","flip_duplex", ppdfile->flip_duplex);

	if (ppdfile->protocols != NULL) {
		fprintf(stderr,"[%20s] %s\n","protocols", ppdfile->protocols);
	}

	if (ppdfile->pcfilename != NULL) {
		fprintf(stderr,"[%20s] %s\n","pcfilename", ppdfile->pcfilename);
	}

	fprintf(stderr,"[%20s] %d\n","num_attrs", ppdfile->num_attrs);
	fprintf(stderr,"[%20s] %d\n","cur_attr", ppdfile->cur_attr);

	if (ppdfile->attrs != NULL) {
		for (index = 0; index < ppdfile->num_attrs; index++) {
			fprintf(stderr,"[%20s] %s\n","attrs_name", ppdfile->attrs[index]->name);
			fprintf(stderr,"[%20s] %s\n","attrs_spec", ppdfile->attrs[index]->spec);
			fprintf(stderr,"[%20s] %s\n","attrs_text", ppdfile->attrs[index]->text);
			fprintf(stderr,"[%20s] %s\n","attrs_value", ppdfile->attrs[index]->value);
		}
	}

	if (ppdfile->sorted_attrs != NULL) {
		fprintf(stderr,"[%20s] %d\n","sortedattrs_num", cupsArrayCount(ppdfile->sorted_attrs));
		for (index = 0; index < cupsArrayCount(ppdfile->sorted_attrs); index++) {
			if (cupsArrayIndex(ppdfile->sorted_attrs, index) != NULL) {
				fprintf(stderr,"[%20s] %s\n","sortedattrs_name",	((ppd_attr_t *)(cupsArrayIndex(ppdfile->sorted_attrs, index)))->name);
			}
		}
	}

	if (ppdfile->options != NULL) {
		fprintf(stderr,"[%20s] %d\n","options_num", cupsArrayCount(ppdfile->options));
		for (index = 0; index < cupsArrayCount(ppdfile->options); index++) {
			fprintf(stderr,"[%20s] %s\n","options_keyword", ((ppd_option_t *)(cupsArrayIndex(ppdfile->options, index)))->keyword);
		}
	}

	if (ppdfile->coptions != NULL) {
		fprintf(stderr,"[%20s] %d\n","coptions_num", cupsArrayCount(ppdfile->coptions));
		for (index = 0; index < cupsArrayCount(ppdfile->coptions); index++) {
			fprintf(stderr,"[%20s] %s\n","coptions_keyword", ((ppd_option_t *)(cupsArrayIndex(ppdfile->coptions, index)))->keyword);
		}
	}

	if (ppdfile->marked != NULL) {
		fprintf(stderr,"[%20s] %d\n","marked_num", cupsArrayCount(ppdfile->marked));
		for (index = 0; index < cupsArrayCount(ppdfile->marked); index++) {
			fprintf(stderr,"[%20s] %s\n","marked_keyword", ((ppd_choice_t *)(cupsArrayIndex(ppdfile->marked, index)))->choice);
		}
	}

	if (ppdfile->cups_uiconstraints != NULL) {
		fprintf(stderr,"[%20s] %d\n","cups_uiconstraints_num", cupsArrayCount(ppdfile->cups_uiconstraints));
		for (index = 0; index < cupsArrayCount(ppdfile->cups_uiconstraints); index++) {
			fprintf(stderr,"[%20s] %s\n","cups_uiconstraints_choice", ((ppd_choice_t *)(cupsArrayIndex(ppdfile->cups_uiconstraints, index)))->choice);
		}
	}

}

/*
"*Keyword: Value" Type(single)
*/
static void print_attribute_kvs(ppd_file_t *ppdfile, const char *option)
{
	ppd_attr_t *attr = NULL;

	for (attr = ppdFindAttr(ppdfile, option, NULL); attr != NULL;
			attr = ppdFindNextAttr(ppdfile, option, NULL)) {

		if (attr != NULL) {
			printf("%s\n",attr->value);
		}
	}
}

/*
"*Keyword: Value" Type(multiple)
*/
static void print_attribute_kvm(ppd_file_t *ppdfile, const char *option)
{
	ppd_attr_t *attr = NULL;

	for (attr = ppdFindAttr(ppdfile, option, NULL); attr != NULL;
			attr = ppdFindNextAttr(ppdfile, option, NULL)) {

		if (attr != NULL) {
			printf("[Name]%s [Spec]%s [Text]%s [Value]%s\n"
				   ,attr->name
				   ,attr->spec
				   ,attr->text
				   ,attr->value);
		}
	}
}

/*
"*Keyword Value/Value: Value" Type
*/
static void print_attribute_kvvm(ppd_file_t *ppdfile, const char *option)
{
	ppd_option_t *attr = NULL;
	int index = 0;

	attr = ppdFindOption(ppdfile, option);

	if (attr != NULL) {
		for (index = 0 ; index < attr->num_choices ; index++) {
			printf("[Keyword]%s",attr->keyword);
			printf(" [Choice]%s",attr->choices->choice);
			printf(" [Code]%s",attr->choices->code);
			printf(" [Text]%s\n",attr->choices->text);
		}
	}

}

static void print_options(ppd_file_t *ppdfile)
{
	int index = 0;

	if (ppdfile->options != NULL) {
		for (index = 0; index < cupsArrayCount(ppdfile->options); index++) {
			printf("%s\n", ((ppd_option_t *)(cupsArrayIndex(ppdfile->options, index)))->keyword);
		}
	}

	if (ppdfile->coptions != NULL) {
		for (index = 0; index < cupsArrayCount(ppdfile->coptions); index++) {
			printf("%s\n", ((ppd_option_t *)(cupsArrayIndex(ppdfile->coptions, index)))->keyword);
		}
	}

}

static void print_option_choices(ppd_file_t *ppdfile, const char *keyword, const char *ch)
{
	ppd_option_t *opt = NULL;
	ppd_choice_t *choice = NULL;

	opt = ppdFindOption(ppdfile, keyword);
	if (opt == NULL) {
		return;
	}

	choice = ppdFindChoice(opt, ch);
	if (choice == NULL) {
		return;
	}

	printf("[Choice]%s",choice->choice);
	printf(" [Code]%s",choice->code);
	printf(" [Text]%s\n",choice->text);

}

int main(int argc, char *argv[])
{
	ppd_file_t *ppd_source = NULL;

	if (argc == 5) {
		ppd_source = ppdOpenFile(argv[4]);
	} else if (argc ==4) {
		ppd_source = ppdOpenFile(argv[3]);
	} else if (argc == 3) {
		ppd_source = ppdOpenFile(argv[2]);
	} else {
		fprintf(stderr, "Usage : %s [Type] [Option] [original_ppd_file]\n", argv[0]);
		fprintf(stderr, "Usage : %s CHOICE [Option] [Choice] [original_ppd_file]\n", argv[0]);
		fprintf(stderr, "Usage : %s PPD [original_ppd_file]\n", argv[0]);
		fprintf(stderr, "Usage : %s OPT [original_ppd_file]\n", argv[0]);
		fprintf(stderr, "Type(KVS) : *Keyword: Value(Single)\n");
		fprintf(stderr, "Type(KVM) : *Keyword: Value(Multiple)\n");
		fprintf(stderr, "Type(KVVM) : *Keyword Value/Value: Value(Multiple)\n");
		return (1);
	}

	if (ppd_source == NULL) {
		fprintf(stderr, "Failed to open ppd(%s)\n",argv[3]);
		return (1);
	}

	if (!strcasecmp(argv[1], "KVS")) {
		print_attribute_kvs(ppd_source, argv[2]);
	} else if (!strcasecmp(argv[1], "KVM")) {
		print_attribute_kvm(ppd_source, argv[2]);
	} else if (!strcasecmp(argv[1], "KVVM")) {
		print_attribute_kvvm(ppd_source, argv[2]);
	} else if (!strcasecmp(argv[1], "PPD")) {
		print_ppd_stucture(ppd_source);
	} else if (!strcasecmp(argv[1], "OPT")) {
		print_options(ppd_source);
	} else if (!strcasecmp(argv[1], "CHOICE")) {
		print_option_choices(ppd_source, argv[2], argv[3]);
	} else {
		return (1);
	}

	ppdClose(ppd_source);
	return (0);
}

