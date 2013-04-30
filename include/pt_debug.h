/*
*	Printservice
*
* Copyright 2012-2013  Samsung Electronics Co., Ltd

* Licensed under the Flora License, Version 1.1 (the "License");
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

#ifndef __PT_DEBUG_H__
#define __PT_DEBUG_H__

#define PT_DLOG_OUT

#ifdef PT_DLOG_OUT
#undef LOG_TAG
#define LOG_TAG "PRINT_SERVICE"
#include <dlog.h>

#define PT_VERBOSE(fmt, args...) \
	LOGV(fmt, ##args)
#define PT_INFO(fmt, args...) \
	LOGI(fmt, ##args)
#define PT_DEBUG(fmt, args...) \
	LOGD(fmt, ##args)
#define PT_WARN(fmt, args...) \
	LOGW(fmt, ##args)
#define PT_ERROR(fmt, args...) \
	LOGE(fmt, ##args)
#define PT_FATAL(fmt, args...) \
	LOGF(fmt, ##args)

#define PT_RET_IF(expr, fmt, args...) \
	do { \
		if(expr) { \
			PT_DEBUG("[%s] Return, message "fmt, #expr, ##args );\
			return; \
		} \
	} while (0)

#define PT_RETV_IF(expr, val, fmt, args...) \
	do { \
		if(expr) { \
			PT_DEBUG("[%s] Return value, message "fmt, #expr, ##args );\
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

#define PRINT_SERVICE_FUNC_ENTER PT_INFO("ENTER FUNCTION: %s\n", __FUNCTION__);
#define PRINT_SERVICE_FUNC_LEAVE PT_INFO("EXIT FUNCTION: %s\n", __FUNCTION__);

#else /* PT_DLOG_OUT */

#define PT_PRT(prio, fmt, arg...) \
	do { \
		fprintf((prio ? stderr : stdout), "%s:%s(%d)>"fmt"\n", __FILE__, __func__, __LINE__, ##arg);\
	} while (0)

#define PT_DEBUG(fmt, arg...) \
	do { \
		printf("%s:%s(%d)>"fmt"\n", __FILE__, __func__, __LINE__, ##arg); \
	} while (0)

#define PRINT_SERVICE_FUNC_ENTER PT_PRT(0, "%s enter\n", __FUNCTION__);
#define PRINT_SERVICE_FUNC_LEAVE PT_PRT(0, "%s leave\n", __FUNCTION__);

#define PT_RET_IF(expr, fmt, args...) \
	do { \
		if(expr) { \
			PT_DEBUG("[%s] Return, message "fmt, #expr, ##args );\
			return; \
		} \
	} while (0)

#define PT_RETV_IF(expr, val, fmt, args...) \
	do { \
		if(expr) { \
			PT_DEBUG("[%s] Return value, message "fmt, #expr, ##args );\
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

#endif /* PT_DLOG_OUT */

#endif /* __PT_DEBUG_H__ */
