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
#include "aikartos/tasks/config.hpp"
#include "aikartos/tasks/object.hpp"

namespace aikartos::kernel {
	class impl_base {
	public:

		using control_block = tasks::control_block<>;
		using task_entry = control_block::task_entry;
		using task_parameter = control_block::task_parameter;

		virtual ~impl_base() = default;
		virtual control_block *add_task(task_entry, task_parameter, const tasks::config &) = 0;
		virtual std::tuple<control_block *, sch::scheduler_specific_event> get_next_task() = 0;

		sch::events::handler_type set_scheduler_event_handler_ = nullptr;
	};
}
