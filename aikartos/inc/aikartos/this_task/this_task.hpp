/*
 * this_task.hpp
 *
 *  Created on: May 28, 2025
 *      Author: newenclave
 *  
 */

#pragma once 

#include "aikartos/kernel/api.hpp"
#include "aikartos/device/device.hpp"

namespace aikartos::this_task {

	inline auto terminate(bool need_yield = true) {
		return kernel::api::terminate_current(need_yield);
	}

	inline bool is_in_interrupt() {
		return kernel::api::is_in_interrupt();
	}

	inline void sleep(std::uint32_t millieconds) {
		return kernel::api::sleep(millieconds);
	}

	inline void sleep_for(std::uint32_t millieconds) {
		auto *task = kernel::api::get_current_tcb();
		DEBUG_ASSERT(current_tcb != nullptr, "Bad current task...");
		return kernel::api::sleep_task_for(task, millieconds);
	}

#if defined(PLATFORM_USE_FPU)

	inline auto enable_fpu() {
		return kernel::api::enable_fpu_for_task(kernel::api::get_current_tcb());
	}

	inline auto disable_fpu() {
		return kernel::api::disable_fpu_for_task(kernel::api::get_current_tcb());
	}

#endif
}



