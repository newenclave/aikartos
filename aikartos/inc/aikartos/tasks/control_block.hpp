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
	template <typename WordType = std::uint32_t>
	struct alignas(8) control_block {
		using word_type = WordType;

		word_type *stack = nullptr;

		using task_entry = tasks::descriptor::task_entry;
		using task_parameter = tasks::descriptor::task_parameter;

		tasks::descriptor task;
		void *scheduler_data = nullptr;

		template <typename T>
		T *get_scheduler_data() {
			return reinterpret_cast<T *>(scheduler_data);
		}

		inline void push(word_type value) {
			*(--stack) = value;
		}

		[[nodiscard]]
		inline word_type pop() {
			return *(stack++);
		}
	};
}

