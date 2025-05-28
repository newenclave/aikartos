/*
 * platform.hpp
 *
 *  Created on: May 25, 2025
 *      Author: newenclave
 *  
 */


#pragma once 

#include "aikartos/device/device.hpp"

extern "C" char g_pfnVectors;

#if defined(PLATFORM_USE_FPU)
#	if defined (__FPU_PRESENT) && (__FPU_PRESENT == 1)
#		define AIKARTOS_ENABLE_FPU 1
#	endif
#endif

namespace aikartos::platform {
	inline void init_vector_table() {
	    SCB->VTOR = reinterpret_cast<std::uintptr_t>(&g_pfnVectors);
	}
}



