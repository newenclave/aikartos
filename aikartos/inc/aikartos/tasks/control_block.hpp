/*
 * tcb.hpp
 *
 *  Created on: May 1, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/platform/platform.hpp"
#include "aikartos/tasks/descriptor.hpp"
#include <cstdint>

namespace aikartos::tasks {

	enum class task_flags: std::uint32_t {
#if defined(AIKARTOS_ENABLE_FPU)
		use_fpu = (1u << 0),
		fpu_saved = (1u << 1),
#endif
	};

	struct alignas(8) control_block {

		std::uintptr_t stack = 0;

#if defined(AIKARTOS_ENABLE_FPU)
		std::uint32_t flags = 0;
		inline bool is_fpu_used() const {
			return flags & static_cast<std::uint32_t>(task_flags::use_fpu);
		}
		inline void enable_fpu() {
			flags |= static_cast<std::uint32_t>(task_flags::use_fpu);
		}
		inline void disable_fpu() {
			flags &= ~static_cast<std::uint32_t>(task_flags::use_fpu);
		}
#endif

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

