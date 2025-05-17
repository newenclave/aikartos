/*
 * tcb.hpp
 *
 *  Created on: May 1, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/tasks/descriptor.hpp"
#include <cstdint>

namespace aikartos::tasks {

	struct alignas(8) control_block {

		std::uintptr_t stack = 0;

		using task_entry = tasks::descriptor::task_entry;
		using task_parameter = tasks::descriptor::task_parameter;

		tasks::descriptor task;
		void *scheduler_data = nullptr;

		template <typename T>
		T *get_scheduler_data() {
			return reinterpret_cast<T *>(scheduler_data);
		}

		template <typename WordType = std::uint32_t>
		inline void push(WordType value) {
			auto wstack = reinterpret_cast<WordType *>(stack);
			*(--wstack) = value;
			stack = reinterpret_cast<std::uintptr_t>(wstack);
		}

	};
}

