/*
 * task.hpp
 *
 *  Created on: May 1, 2025
 *      Author: newenclave
 */
#pragma once

#include <cstdint>
#include <concepts>

namespace aikartos::tasks {
	struct descriptor {
		enum class state_type: std::uint8_t {
			NONE = 0,
			READY = 1,
			RUNNING = 2,
			DONE = 3,
			WAIT = 4,
		};
		struct timing_info {
			std::uint32_t period_ms = 0;
			std::uint32_t next_run = 0;
		};

		using task_parameter = void *;
		using task_entry = void(*)(task_parameter);
		task_entry task = nullptr;
		task_parameter parameter = nullptr;
		timing_info timing;
		state_type state = state_type::NONE;
	};
}
