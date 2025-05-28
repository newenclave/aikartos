/*
 * kernel_core_base.hpp
 *
 *  Created on: May 9, 2025
 *      Author: newenclave
 */

#pragma once

#include <tuple>

#include "aikartos/kernel/config.hpp"
#include "aikartos/sch/events.hpp"
#include "aikartos/sch/statistic.hpp"
#include "aikartos/tasks/config.hpp"
#include "aikartos/tasks/object.hpp"

namespace aikartos::kernel {

	class core;
	class impl_base {
	public:

		using control_block = tasks::control_block;
		using task_entry = control_block::task_entry;
		using task_parameter = control_block::task_parameter;

		using systick_hook_parameter_type = void *;
		using systick_hook_type = bool(*)(systick_hook_parameter_type);

		virtual ~impl_base() = default;
		virtual control_block *add_task(task_entry, task_parameter, const tasks::config &) = 0;
		virtual std::tuple<control_block *, sch::scheduler_specific_event> get_next_task() = 0;
		virtual bool get_scheduler_statistic(sch::statistic_base &) = 0;

#if defined(AIKARTOS_ENABLE_FPU)
		inline static void set_task_fpu_default(bool value) { default_fpu_ = value; }
		inline static bool get_task_fpu_default() { return default_fpu_; }
#endif

	protected:
		friend class kernel::core;
		sch::events::handler_type scheduler_event_handler_ = nullptr;
		systick_hook_type systick_hook_ = nullptr;
		systick_hook_parameter_type systick_hook_parameter_ = nullptr;
		inline static std::uint32_t quanta_ = 0;
		inline static std::uint32_t default_quanta_ = 0;
#if defined(AIKARTOS_ENABLE_FPU)
		inline static bool volatile default_fpu_ = false;
#endif
	};
}
