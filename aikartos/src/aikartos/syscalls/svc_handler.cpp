/*
 * svc_handler.cpp
 *
 *  Created on: Aug 3, 2025
 *      Author: newenclave
 *  
 */

#include <cstdint>
#include "aikartos/syscalls/syscalls.h"
#include "aikartos/kernel/kernel.hpp"

namespace aikartos::syscalls {

	namespace {

	}

	void svc_dispatch_impl(std::uint32_t *stack_frame) {
		// stack: r0, r1, r2, r3, r12, lr, pc, xPSR (8 words total)

		const auto call = static_cast<SysCallCodes>(stack_frame[0]);
		[[maybe_unused]] const std::uint32_t arg0 = stack_frame[1];
		[[maybe_unused]] const std::uint32_t arg1 = stack_frame[2];
		[[maybe_unused]] const std::uint32_t arg2 = stack_frame[3];

		switch(call) {
		case SysCallCodes::SYSCALL_YIELD:
			kernel::yield();
			break;
		case SysCallCodes::SYSCALL_SLEEP:
			kernel::sleep(arg0);
			break;
		case SysCallCodes::SYSCALL_ADD_TASK:
			kernel::add_task(reinterpret_cast<kernel::core::task_entry>(arg0), reinterpret_cast<kernel::core::task_parameter>(arg1));
			break;
		case SYSCALL_NONE:
		case SYSCALL_MAX:
		default:
			stack_frame[0] = static_cast<std::uint32_t>(-1);;
			return;
		}

		stack_frame[0] = 0; // return r0
	}
}

extern "C" void svc_dispatch(std::uint32_t *stack_frame) {
	aikartos::syscalls::svc_dispatch_impl(stack_frame);
}

extern "C" __attribute__((naked)) void SVC_Handler(void) {
	__asm volatile ("TST 	lr, #4");  // select PSP or MSP stack
	__asm volatile ("ITE 	eq");
	__asm volatile ("MRSEQ 	r0, msp");
	__asm volatile ("MRSNE 	r0, psp");
	__asm volatile ("B 		svc_dispatch");
}

