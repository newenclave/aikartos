/*
 * priority_aging.hpp
 *
 *  Created on: May 7, 2025
 *      Author: newenclave
 */

#pragma once
#include "aikartos/kernel/core.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/waiting_tasks_queue.hpp"
#include "aikartos/sync/circular_queue.hpp"
#include "aikartos/sync/policies/no_mutex.hpp"
#include "aikartos/sync/priority_queue.hpp"
#include "aikartos/tasks/object.hpp"
#include "aikartos/utils/object_pool.hpp"
#include <array>


namespace aikartos::sch {

	namespace priority_aging {

		enum class config_flags: std::uint32_t {
			priority 		= (1 << 0),
			aging_threshold = (1 << 1),
		};

		template <typename ConfigT,  typename TasksEventsType>
		class scheduler {
		public:
			constexpr static std::size_t maximum_tasks = ConfigT::maximum_tasks;

			using tasks_events_type = TasksEventsType;
			using control_block = tasks::control_block<>;

			constexpr static std::size_t maximum_priority = 3;
			using ready_block_queue_type = sync::circular_queue<control_block *, maximum_tasks, sync::policies::no_mutex>;
			using ready_array_type = std::array<ready_block_queue_type, maximum_priority>;

			struct scheduler_data_type {
				std::uint8_t current_priority = 1;
				std::uint8_t base_priority = 1;
				std::uint8_t aging_threshold = 1;
				std::uint8_t aging_score = 0;
			};

			using scheduler_data_allocator = utils::object_pool<scheduler_data_type, maximum_tasks, 4>;

			void configure_task(control_block *task, const tasks::config &cfg) {

				auto *sch_data = data_allocator_.alloc();
				task->scheduler_data = static_cast<void *>(sch_data);
				cfg.update_value<config_flags::priority>(sch_data->current_priority);
				ASSERT(sch_data->current_priority < maximum_priority, "Bad priority value");
				sch_data->base_priority = sch_data->current_priority;
				cfg.update_value<config_flags::aging_threshold>(sch_data->aging_threshold);
			}

			void clear_task(control_block *value) {
				data_allocator_.free(value->template get_scheduler_data<scheduler_data_type>());
			}

			using waiting_queue = waiting_tasks_queue<maximum_tasks, sync::policies::no_mutex>;

			void add_task(control_block *value) {
				const auto priority_id = static_cast<std::size_t>(value->template get_scheduler_data<scheduler_data_type>()->current_priority);
				DEBUG_ASSERT(priority_id < maximum_priority, "Bad task priority.");
				ready_tasks_[priority_id].try_push(value);
			}

			control_block *get_next_task() {
				process_waiting_queue();
				control_block *next_result = get_next_tcb_impl();
				tasks_aging();
				return next_result;
			}

		private:

			control_block *get_next_tcb_impl() {

				for(auto &queue: ready_tasks_) {
					while(auto next_task = queue.try_pop()) {
						auto *task = *next_task;

						switch(task->task.state) {
						case tasks::descriptor::state_type::READY:
							[[fallthrough]];
						case tasks::descriptor::state_type::RUNNING:
							add_task_impl(task, reset_priority(task));
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

			auto get_data(control_block *task) {
				return task->template get_scheduler_data<scheduler_data_type>();
			}

			void tasks_aging() {
				for(std::size_t queue = 1; queue < ready_tasks_.size(); ++queue) {
					const auto qsize = ready_tasks_[queue].size();
					for(std::size_t id = 0; id < qsize; ++id) {
						auto next = ready_tasks_[queue].try_pop();
						auto *task = *next;
						auto *data = get_data(task);
						std::uint8_t priority = static_cast<std::uint8_t>(data->current_priority);
						if(++data->aging_score >= data->aging_threshold) {
							data->aging_score = 0;
							data->current_priority = priority - 1;
							ready_tasks_[queue-1].try_push(task);
						} else {
							ready_tasks_[queue].try_push(task);
						}
					}
				}
			}

			void add_task_impl(control_block *task, std::uint8_t priority) {
				ready_tasks_[priority].try_push(task);
			}

			auto reset_priority(control_block *task) {
				auto *data = task->template get_scheduler_data<scheduler_data_type>();
				data->current_priority = data->base_priority;
				return data->current_priority;
			}

			void process_waiting_queue() {
				waiting_tasks_.process([this](auto *task) { add_task(task); });
			}

			ready_array_type ready_tasks_;
			waiting_queue waiting_tasks_;
			scheduler_data_allocator data_allocator_;
		};

	}

}
