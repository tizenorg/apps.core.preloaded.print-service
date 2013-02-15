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

QueueElmt *push(void *data)
{
	QueueElmt *t = calloc(1, sizeof(QueueElmt));

	t->data = data;
	list->head->next->prev = t;
	t->next = list->head->next;
	list->head->next = t;
	t->prev = list->head;

	return NULL;
}

void *pop(void)
{
	char *data = NULL;
	QueueElmt *t = list->head->next;

	list->head->next = t->next;
	t->next->prev = list->head;

	data = t->data;
	PT_IF_FREE_MEM(t);

	return data;
}


void *upsh(void)
{
	char *data = NULL;
	QueueElmt *t = list->tail->prev;

	data = t->data;

	t->prev->next = list->tail;
	list->tail->prev = t->prev;
	PT_IF_FREE_MEM(t);

	return data;
}

List *stackinit(void)
{
	List *lst = calloc(1, sizeof(List));

	lst->head = calloc(1, sizeof(QueueElmt));
	lst->tail = calloc(1, sizeof(QueueElmt));
	lst->tail->next = lst->tail;
	lst->tail->prev = lst->head;
	lst->head->next = lst->tail;
	lst->head->prev = lst->head;
	lst->pop = pop;
	lst->push = push;
	lst->upsh = upsh;

	return lst;
}
