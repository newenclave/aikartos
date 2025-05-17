/*
 * memory.hpp
 *
 *  Created on: May 17, 2025
 *      Author: newenclave
 *  
 */


#pragma once 
#include "aikartos/memory/core.hpp"

namespace aikartos::memory {
	template <typename AllocT>
	inline auto init() { core::init<AllocT>(); }

	inline static memory::allocator_base *get_allocator() {
		return core::get_allocator();
	}

}
