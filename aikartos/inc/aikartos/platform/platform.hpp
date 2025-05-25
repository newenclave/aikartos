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

namespace aikartos::platform {
	inline void init_vector_table() {
	    SCB->VTOR = reinterpret_cast<std::uintptr_t>(&g_pfnVectors);
	}
}



