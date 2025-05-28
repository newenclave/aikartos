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
#if defined(AIKARTOS_ENABLE_FPU)

	inline auto enable_fpu() {
		return kernel::api::enable_fpu(kernel::api::get_current_tcb());
	}

	inline auto disable_fpu() {
		return kernel::api::disable_fpu(kernel::api::get_current_tcb());
	}

#endif
}



