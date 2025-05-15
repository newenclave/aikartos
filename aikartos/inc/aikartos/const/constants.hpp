/*
 * constants.hpp
 *
 *  Created on: Apr 22, 2025
 *      Author: newenclave
 */

#pragma once

#include <cstdint>

namespace aikartos::constants {
	constexpr std::uint32_t system_clock_frequency = 16'000'000u;
	constexpr std::uint32_t quanta_infinite = 0xFFFF'FFFF; // No forced preemption (cooperative task)
}
