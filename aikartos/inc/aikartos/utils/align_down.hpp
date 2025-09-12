/*
 * align_down.hpp
 *
 *  Created on: Sep 12, 2025
 *      Author: newenclave
 *  
 */


#pragma once 

#include <cstddef>
#include <concepts>

namespace aikartos::utils {

	template <typename T>
	constexpr inline T align_down(T value, std::size_t align)
		requires std::unsigned_integral<T>
	{
		return value & ~static_cast<std::uintptr_t>(align - 1u);
	}
}
