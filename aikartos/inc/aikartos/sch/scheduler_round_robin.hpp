/*
 * round_robin.hpp
 *
 *  Created on: May 6, 2025
 *      Author: newenclave
 */

#pragma once

#include <tuple>

#include "aikartos/kernel/core.hpp"
#include "aikartos/sch/events.hpp"
#include "aikartos/sch/waiting_tasks_queue.hpp"
#include "aikartos/sync/circular_queue.hpp"
#include "aikartos/sync/policies/no_mutex.hpp"
#include "aikartos/sync/priority_queue.hpp"
#include "aikartos/tasks/config.hpp"
#include "aikartos/tasks/object.hpp"
#include "aikartos/sync/circular_queue.hpp"

namespace aikartos::sch {

	namespace round_robin {

		enum class config_flags: std::uint32_t {};

		template <typename ConfigT, typename TasksEventsType>
		class scheduler {
		public:
			using config = ConfigT;

			constexpr static std::size_t maximum_tasks = config::maximum_tasks;

			using control_block  = tasks::control_block;
			using tasks_events_type = TasksEventsType;

			using task_block_queue_type = sync::circular_queue<control_block *, maximum_tasks, sync::policies::no_mutex>;
			using waiting_queue = waiting_tasks_queue<maximum_tasks, sync::policies::no_mutex>;

			void configure_task(control_block *, const tasks::config &) {}
			void clear_task(control_block *) {}

			control_block* get_next_task() {
				process_waiting_queue();

				while(auto next_task = ready_tasks_.try_pop()) {
					auto *task = *next_task;

					switch(task->task.state) {
					case tasks::descriptor::state_type::READY:
						[[fallthrough]];
					case tasks::descriptor::state_type::RUNNING:
						ready_tasks_.try_push(task);
						return task;
					case tasks::descriptor::state_type::DONE:
						tasks_events_type::on_task_done(task);
						break;
					case tasks::descriptor::state_type::WAIT:
						waiting_tasks_.try_push(task);
						break;
					default:
						break;
					}
				}

				return nullptr;
			}

			void add_task(control_block *value) {
				ready_tasks_.try_push(value);
			}

		private:

			void process_waiting_queue() {
				waiting_tasks_.process([this](auto *task){ ready_tasks_.try_push(task); });
			}

			task_block_queue_type ready_tasks_;
			waiting_queue waiting_tasks_;
		};

	}

}
