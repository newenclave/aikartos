/*
 * constants.hpp
 *
 *  Created on: Apr 22, 2025
 *      Author: newenclave
 */

#pragma once

#include <cstdint>
#include "aikartos/device/device.hpp"

namespace aikartos::constants {
	constexpr std::uint32_t system_clock_frequency = PLATFORM_DEFAULT_SYSTEM_CLOCK_FREQUENCY;
	constexpr std::uint32_t uart_clock_frequency = PLATFORM_DEFAULT_SYSTEM_CLOCK_FREQUENCY;
	constexpr std::uint32_t default_baud_rate = 115'200u;
	constexpr std::uint32_t quanta_infinite = 0xFFFF'FFFF; // No forced preemption (cooperative task)
}
