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

#include "main.h"
#define BACKSIZE 10

void init_signals(void)
{
	void (*err)(int);

	err = signal(SIGSEGV, backtrace_handler);
	if (err) {
		fprintf(stderr, "SIGSEGV handler is not set\n");
		exit(EXIT_FAILURE);
	}

	err = signal(SIGABRT, backtrace_handler);
	if (err) {
		fprintf(stderr, "SIGABRT handler is not set\n");
		exit(EXIT_FAILURE);
	}

	return;
}

void backtrace_handler(int signum)
{
	int ok;
	int i;
	int size;
	char **strings;
	void *array[BACKSIZE];

	bfd_init();

	abfd = bfd_openr(program_invocation_name, 0);
	if (!abfd) {
		perror("bfd_openr failed");
		exit(EXIT_FAILURE);
	}

	char *sig = strsignal(signum);
	if (sig == NULL) {
		perror("bfd_check_format failed");
		exit(EXIT_FAILURE);
	}

	printf("%s crashed with signal \"%s\"...\n", program_invocation_name, sig);
	size = backtrace(array, BACKSIZE);
	strings = backtrace_symbols(array, size);

	for (i = 1; i < size; i++) {
		bfd_vma offset;
		const char *file;
		const char *func;
		unsigned line = 0;
		unsigned storage_needed = 0;

		/* oddly, this is required for it to work... */
		ok = bfd_check_format(abfd, bfd_object);
		if (ok == 0) {
			fprintf(stderr, "bfd_check_format failed - %s:%d\n", __FILE__, __LINE__ - 2);
			exit(EXIT_FAILURE);
		}

		storage_needed = (unsigned) bfd_get_symtab_upper_bound(abfd);
		if (storage_needed == 0) {
			fprintf(stderr, "bfd_get_symtab_upper_bound failed - %s:%d\n", __FILE__, __LINE__ - 2);
			exit(EXIT_FAILURE);
		}

		syms = malloc(storage_needed);
		if (syms == NULL) {
			fprintf(stderr, "cannot allocate memory - %s:%d\n", __FILE__, __LINE__ - 2);
			exit(EXIT_FAILURE);
		}

		ok = bfd_canonicalize_symtab(abfd, syms);
		if (ok <= 0) {
			fprintf(stderr, "bfd_canonicalize_symtab failed - %s:%d\n", __FILE__, __LINE__ - 2);
			exit(EXIT_FAILURE);
		}

		text = bfd_get_section_by_name(abfd, ".text");
		if (text == NULL) {
			fprintf(stderr, "sections is empty - %s:%d\n", __FILE__, __LINE__ - 2);
			exit(EXIT_FAILURE);
		}

		offset = (bfd_vma)((long) array[i] - text->vma);
		if (offset > 0) {
			if ((bfd_find_nearest_line(abfd, text, syms, offset, &file, &func, &line) && file) != 0) {
				printf("%d %s:%u %p\n", i - 1, file, line, array[i]);
			}
		}

		free(syms);
	}

	free(strings);

	exit(EXIT_FAILURE);
}
