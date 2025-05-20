/*
 * allocator_buddy.hpp
 *
 *  Created on: May 20, 2025
 *      Author: newenclave
 *  
 */


#pragma once 


#include <cstddef>
#include "aikartos/memory/allocator_base.hpp"
#include "aikartos/memory/allocator/buddy/impl/region.hpp"
#include "aikartos/memory/allocator/buddy/impl/fixed.hpp"

namespace aikartos::memory {

	template<std::size_t MinimumLog2Order = 5, std::size_t AlignValue = 8>
	using allocator_buddy = memory::allocator::buddy::impl::region<MinimumLog2Order, AlignValue>;

	template<std::size_t MemorySize, std::size_t MinimumLog2Order = 5,
			std::size_t AlignValue = 8>
	using allocator_buddy_fixed = memory::allocator::buddy::impl::fixed<MemorySize, MinimumLog2Order, AlignValue>;
}
