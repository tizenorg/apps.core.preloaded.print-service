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

#define _GNU_SOURCE

#include <bfd.h>
#include <errno.h>
#include <execinfo.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define PT_RET_IF(expr, fmt, args...) \
	do { \
		if(expr) { \
			fprintf(stderr,"[%s] Return, message "fmt, #expr, ##args );\
			return; \
		} \
	} while (0)

#define PT_RETV_IF(expr, val, fmt, args...) \
	do { \
		if(expr) { \
			fprintf(stderr,"[%s] Return value, message "fmt, #expr, ##args );\
			return (val); \
		} \
	} while (0)

#define PT_IF_FREE_MEM(mem) \
	do { \
		if(mem) { \
			free(mem);\
			mem = NULL; \
		} \
	} while (0)

typedef struct node {
	char *data;
	struct node *next;
	struct node *prev;
} QueueElmt;

typedef struct List {
	QueueElmt *head;
	QueueElmt *tail;
	void (*stackinit)(void);
	QueueElmt *(*push)(void *);
	void *(*upsh)(void);
	void *(*pop)(void);
} List;

List *list;

extern char *program_invocation_name;
extern const int DEBUG;

asection	*text;
asymbol		**syms;
bfd 		*abfd;

/* init_signals: init signal handlers */
void init_signals(void);
/* backtrace_handler: signal handler */
void backtrace_handler(int signum);
QueueElmt *push(void *data);
void *pop(void);
void *upsh(void);
List *stackinit(void);
