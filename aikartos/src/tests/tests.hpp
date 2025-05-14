/*
 * tests.hpp
 *
 *  Created on: May 14, 2025
 *      Author: newenclave
 */

#pragma once

#include <cstdint>

namespace tests {
	constexpr std::uint32_t COUNT_SIZE = 5;
	int round_robin();
	int edf();
	int lottery();
	int weighted_lottery();
	int fixed_priority();
	int priority_aging();
	int stack_overflow();
}

extern std::uint32_t count[tests::COUNT_SIZE];
