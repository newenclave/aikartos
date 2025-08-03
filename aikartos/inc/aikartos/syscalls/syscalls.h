/*
 * syscalls.h
 *
 *  Created on: Aug 3, 2025
 *      Author: newenclave
 *  
 */


#pragma once 

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum SysCallCodes {
	SYSCALL_NONE = 0,
	SYSCALL_YIELD,
	SYSCALL_SLEEP,
	SYSCALL_ADD_TASK,

	SYSCALL_MAX,
};

typedef void (*syscall_task_type)(void*);

static inline uint32_t syscall_impl(SysCallCodes code, uint32_t arg0 = 0, uint32_t arg1 = 0, uint32_t arg2 = 0) {
	register uint32_t r0 __asm("r0") = code;
	register uint32_t r1 __asm("r1") = arg0;
	register uint32_t r2 __asm("r2") = arg1;
	register uint32_t r3 __asm("r3") = arg2;

	__asm volatile(
			"SVC 0"						// call the supervisor
			: "+r"(r0)					// input (r0 — return)
			: "r"(r1), "r"(r2), "r"(r3)	// args
			: "memory"					// memory can change
	);
	return r0;
}

static inline int32_t syscall_yield() {
	return syscall_impl(SYSCALL_YIELD);
}

static inline int32_t syscall_sleep(uint32_t milliseconds) {
	return syscall_impl(SYSCALL_SLEEP, milliseconds);
}

static inline int32_t syscall_add_task(syscall_task_type task, void *parameter = (void *)0) {
	return syscall_impl(SYSCALL_ADD_TASK, (uint32_t)task, (uint32_t)parameter);
}

#ifdef __cplusplus
}
#endif
