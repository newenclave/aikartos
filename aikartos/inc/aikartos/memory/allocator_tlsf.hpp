/*
 * allocator_buddy.hpp
 *
 *  Created on: May 20, 2025
 *      Author: newenclave
 *  
 */


#pragma once 


#include <aikartos/memory/allocator/tlsf/impl/fixed.hpp>
#include <aikartos/memory/allocator/tlsf/impl/region.hpp>
#include <cstddef>
#include "aikartos/memory/allocator_base.hpp"

namespace aikartos::memory {

	template <std::size_t SubclassBits = 2,
			std::size_t MinClassLog2 = 5,
			std::size_t Align = 8>
	using allocator_tlsf = memory::allocator::tlsf::impl::region<SubclassBits, MinClassLog2, Align>;

	template <std::size_t MaximumMemory,
			std::size_t SubclassBits = 2,
			std::size_t MinClassLog2 = 5,
			std::size_t Align = 8>
	using allocator_tlsf_fixed = memory::allocator::tlsf::impl::fixed<MaximumMemory, SubclassBits, MinClassLog2, Align>;
}
