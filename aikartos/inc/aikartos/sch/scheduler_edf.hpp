/*
 * edf.hpp
 *
 *  Created on: May 11, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/sch/waiting_tasks_queue.hpp"
#include "aikartos/sync/policies/no_mutex.hpp"
#include "aikartos/sync/priority_queue.hpp"
#include "aikartos/tasks/config.hpp"
#include "aikartos/tasks/control_block.hpp"

namespace aikartos::sch {

	namespace edf {

		enum class config_flags: std::uint32_t {
			relative_deadline = (1 << 0),
		};

		template <typename ConfigT, typename TasksEventsType>
		class scheduler {
		public:
			using config = ConfigT;

			constexpr static std::size_t maximum_tasks = config::maximum_tasks;

			using control_block  = tasks::control_block<>;
			using tasks_events_type = TasksEventsType;

			struct scheduler_data_type {
				std::uint32_t deadline = 0;
			};

			using scheduler_data_allocator = utils::object_pool<scheduler_data_type, maximum_tasks, 4>;

			void configure_task(control_block *task, const tasks::config &cfg) {

				auto *sch_data = data_allocator_.alloc();
				task->scheduler_data = static_cast<void *>(sch_data);
				cfg.update_value<config_flags::relative_deadline>(sch_data->deadline);
				sch_data->deadline += kernel::core::get_tick_count();
			}

			void clear_task(control_block *value) {
				data_allocator_.free(get_data(value));
			}

			struct deadline_compare {
				bool operator ()(control_block *lhs, control_block *rhs) const {
					return get_data(rhs)->deadline < get_data(lhs)->deadline;
				}
			};

			using deadline_queue = sync::priority_queue<control_block *, maximum_tasks, deadline_compare, sync::policies::no_mutex>;
			using waiting_queue = waiting_tasks_queue<maximum_tasks, sync::policies::no_mutex>;

			void add_task(control_block *value) {
				deadline_queue_.try_push(value);
			}

			std::tuple<control_block *, sch::scheduler_specific_event> get_next_task() {
				process_waiting_queue();

				const auto current_ticks = kernel::core::get_tick_count();

				while(auto next_task = deadline_queue_.try_pop()) {
					auto *task = *next_task;
					auto task_deadline = get_data(task)->deadline;

					if((task->task.state != tasks::descriptor::state_type::DONE)
							&& (task_deadline <= current_ticks)) {
						deadline_queue_.try_push(task);
						return { task, 100 };
					}

					switch(task->task.state) {
					case tasks::descriptor::state_type::READY:
						[[fallthrough]];
					case tasks::descriptor::state_type::RUNNING:
						/// process expired deadline somehow?
						deadline_queue_.try_push(task);
						return { task, sch::events::OK };
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

				return { nullptr, sch::events::OK };
			}

		private:

			void process_waiting_queue() {
				waiting_tasks_.process([this](auto *task) { add_task(task); });
			}

			static scheduler_data_type *get_data(control_block *value) {
				return value->template get_scheduler_data<scheduler_data_type>();
			}

			scheduler_data_allocator data_allocator_;
			deadline_queue deadline_queue_;
			waiting_queue waiting_tasks_;
		};

	}
}
