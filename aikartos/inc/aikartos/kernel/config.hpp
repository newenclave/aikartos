/*
 * kernel_config.hpp
 *
 *  Created on: May 9, 2025
 *      Author: newenclave
 */

#pragma once

#include <cstdint>

namespace aikartos::kernel {
	struct config {
		constexpr static std::uint32_t quanta = 10; // 10 milliseconds
		constexpr static std::uint32_t stack_size = 600; // 600 words
		constexpr static std::uint32_t maximum_tasks = 5;
		inline static auto idle_hook = []{};
	};
}
