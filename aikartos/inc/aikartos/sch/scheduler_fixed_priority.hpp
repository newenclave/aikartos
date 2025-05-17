/*
 * fixed_priority.hpp
 *
 *  Created on: May 6, 2025
 *      Author: newenclave
 */

#pragma once
#include "aikartos/kernel/core.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/waiting_tasks_queue.hpp"
#include "aikartos/sync/circular_queue.hpp"
#include "aikartos/sync/policies/no_mutex.hpp"
#include "aikartos/sync/priority_queue.hpp"
#include "aikartos/tasks/config.hpp"
#include "aikartos/tasks/object.hpp"
#include "aikartos/utils/object_pool.hpp"
#include <array>


namespace aikartos::sch {

	namespace fixed_priority {

		enum class config_flags: std::uint32_t {
			priority = (1 << 0),
		};

		template <typename ConfigT, typename TasksEventsType>
		class scheduler {
		public:

			using config = ConfigT;
			constexpr static std::size_t maximum_tasks = config::maximum_tasks;

			using tasks_events_type = TasksEventsType;
			using control_block = tasks::control_block;

			constexpr static std::size_t maximum_priority = 3;
			using ready_block_queue_type = sync::circular_queue<control_block *, maximum_tasks, sync::policies::no_mutex>;
			using ready_array_type = std::array<ready_block_queue_type, maximum_priority>;

			struct scheduler_data_type {
				std::uint8_t priority = 0;
			};

			using scheduler_data_allocator = utils::object_pool<scheduler_data_type, maximum_tasks, 4>;

			void configure_task(control_block *value, const tasks::config &cfg) {
				auto *sch_data = data_allocator_.alloc();
				value->scheduler_data = static_cast<void *>(sch_data);
				cfg.update_value<config_flags::priority>(sch_data->priority);
				ASSERT(sch_data->priority < maximum_priority, "Bad priority value");
			}

			void clear_task(control_block *value) {
				data_allocator_.free(value->template get_scheduler_data<scheduler_data_type>());
			}

			using waiting_queue = waiting_tasks_queue<maximum_tasks, sync::policies::no_mutex>;

			void add_task(control_block *value) {
				const auto priority_id = static_cast<std::size_t>(value->template get_scheduler_data<scheduler_data_type>()->priority);
				DEBUG_ASSERT(priority_id < maximum_priority, "Bad task priority.");
				ready_tasks_[priority_id].try_push(value);
			}

			control_block *get_next_task() {

				process_waiting_queue();

				for(auto &queue: ready_tasks_) {
					while(auto next_task = queue.try_pop()) {
						auto *task = *next_task;

						switch(task->task.state) {
						case tasks::descriptor::state_type::READY:
							[[fallthrough]];
						case tasks::descriptor::state_type::RUNNING:
							queue.try_push(task);
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
				}
				return nullptr;
			}

		private:

			void process_waiting_queue() {
				waiting_tasks_.process([this](auto *task){ add_task(task); });
			}

			ready_array_type ready_tasks_;
			waiting_queue waiting_tasks_;
			scheduler_data_allocator data_allocator_;
		};
	}
}
