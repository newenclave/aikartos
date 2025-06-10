/*
 * api.h
 *
 *  Created on: Jul 5, 2025
 *      Author: newenclave
 *  
 */

#ifndef AIKARTOS_SDK_AIKARTOS_API_H_
#define AIKARTOS_SDK_AIKARTOS_API_H_

#include <stdint.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif

	typedef void* (*memory_malloc_fn)(size_t);
	typedef void* (*memory_realloc_fn)(void*, size_t);
	typedef void (*memory_free_fn)(void*);

	typedef void (*kernel_task_type)(void*);
	typedef void (*kernel_add_task_fn)(kernel_task_type, void *);

	typedef size_t (*device_uart_read_fn)(char*, size_t);
	typedef void (*device_uart_write_fn)(const char*, size_t);

	typedef void (*this_task_sleep_fn)(uint32_t);
	typedef void (*this_task_yield_fn)(void);

	typedef void (*fpu_enable_fn)(void);
	typedef void (*fpu_disable_fn)(void);


	struct memory_api {
		memory_malloc_fn malloc = nullptr;
		memory_realloc_fn realloc = nullptr;
		memory_free_fn free = nullptr;
	};

	struct kernel_api {
		kernel_add_task_fn add_task = nullptr;
	};

	struct device_api {
		device_uart_read_fn uart_read = nullptr;
		device_uart_write_fn uart_write = nullptr;
	};


	struct this_task_api {
		this_task_sleep_fn sleep = nullptr;
		this_task_yield_fn yield = nullptr;
	};

	struct fpu_api {
		fpu_enable_fn enable = nullptr;
		fpu_disable_fn disable = nullptr;
	};

	struct module_info {
		uintptr_t module_base;
		uintptr_t module_size;
	};

	struct aikartos_api {
		module_info module;
		memory_api memory;
		kernel_api kernel;
		device_api device;
		this_task_api this_task;
		fpu_api fpu;
	};

#ifdef __cplusplus
}
#endif

#endif /* AIKARTOS_SDK_AIKARTOS_API_H_ */
