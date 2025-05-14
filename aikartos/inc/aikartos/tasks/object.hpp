/*
 * task_block.hpp
 *
 *  Created on: May 1, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/tasks/control_block.hpp"

namespace aikartos::tasks {

	template <std::size_t StackSize, typename WordType = std::uint32_t>
	struct alignas(8) object {

		static_assert(StackSize >= 32, "The size of the stack should be at least 32 words");

		using word_type = WordType;
		using control_type = control_block<word_type>;

		constexpr void reset_stack() {
			tcb.stack = &stack[StackSize];
#ifdef DEBUG
		    for (std::size_t i = 0; i < StackSize; ++i) {
		        stack[i] = 0xDEADBEEF;
		    }
#endif
		}

		object() {
			reset_stack();
		}

		control_type tcb;
		alignas(8) word_type stack[StackSize];
	};
}

