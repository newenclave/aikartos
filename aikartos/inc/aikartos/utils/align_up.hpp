/*
 * align_up.hpp
 *
 *  Created on: May 22, 2025
 *      Author: newenclave
 *  
 */


#pragma once

#include <cstddef>
#include <cstdint>

namespace aikartos::utils {

	constexpr inline auto align_up(auto value, std::size_t align) {
		return (value + (align - 1)) & ~(align - 1);
	}
}
