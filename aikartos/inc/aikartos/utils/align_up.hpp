/*
 * align_up.hpp
 *
 *  Created on: May 22, 2025
 *      Author: newenclave
 *  
 */


#pragma once

#include <cstddef>
#include <concepts>

namespace aikartos::utils {

	template <typename T>
	constexpr inline T align_up(T value, std::size_t align)
		requires std::unsigned_integral<T>
	{
		return (value + (align - 1)) & ~(align - 1);
	}
}
