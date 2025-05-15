/*
 * scheduler_coop_preemptive.hpp
 *
 *  Created on: May 15, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/sch/waiting_tasks_queue.hpp"
#include "aikartos/sync/policies/no_mutex.hpp"
#include "aikartos/sync/circular_queue.hpp"
#include "aikartos/tasks/config.hpp"
#include "aikartos/tasks/control_block.hpp"
#include "aikartos/utils/object_pool.hpp"

namespace aikartos::sch {

	namespace coop_preemptive {

		enum class config_flags: std::uint32_t {
			quanta = (1 << 0),
		};

		template <typename ConfigT, typename TasksEventsType>
		class scheduler {
		public:
			using config = ConfigT;

			constexpr static std::size_t maximum_tasks = config::maximum_tasks;

			using control_block  = tasks::control_block<>;
			using tasks_events_type = TasksEventsType;

			using task_block_queue_type = sync::circular_queue<control_block *, maximum_tasks, sync::policies::no_mutex>;
			using waiting_queue = waiting_tasks_queue<maximum_tasks, sync::policies::no_mutex>;

			struct scheduler_data_type {
				std::uint32_t quanta = config::quanta;
			};

			using scheduler_data_allocator = utils::object_pool<scheduler_data_type, maximum_tasks, 4>;

			void configure_task(control_block *task, const tasks::config &cfg) {
				auto *data = data_allocator_.alloc();
				task->scheduler_data = static_cast<void *>(data);
				cfg.update_value<config_flags::quanta>(data->quanta);
			}

			void clear_task(control_block *task) {
				data_allocator_.free(task->get_scheduler_data<scheduler_data_type>());
			}

			control_block* get_next_task() {
				process_waiting_queue();

				while(auto next_task = ready_tasks_.try_pop()) {
					auto *task = *next_task;

					switch(task->task.state) {
					case tasks::descriptor::state_type::READY:
						[[fallthrough]];
					case tasks::descriptor::state_type::RUNNING:
						ready_tasks_.try_push(task);
						tasks_events_type::on_quanta_change(get_quanta(task));
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

			static std::uint32_t get_quanta(control_block *task) {
				return task->get_scheduler_data<scheduler_data_type>()->quanta;
			}

			void process_waiting_queue() {
				waiting_tasks_.process([this](auto *task){ ready_tasks_.try_push(task); });
			}

			task_block_queue_type ready_tasks_;
			waiting_queue waiting_tasks_;
			scheduler_data_allocator data_allocator_;
		};

	}
}
