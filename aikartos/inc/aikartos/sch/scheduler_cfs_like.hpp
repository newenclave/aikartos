/*
 *
 * @file scheduler_cfs_like.hpp
 * @brief Fair scheduler based on accumulated virtual runtime (vruntime), inspired by the Linux CFS.
 *
 * - Each task maintains a vruntime counter that tracks how much CPU time it has consumed.
 * - On every context switch, the current task's vruntime is incremented based on its execution time.
 * - The scheduler always selects the task with the smallest vruntime to run next.
 * - Sleeping tasks do not accumulate vruntime and thus appear "poorer" when they return - gaining priority.
 *
 *
 *  Created on: May 17, 2025
 *      Author: newenclave
 *
 *
 */

#pragma once

#include "aikartos/kernel/core.hpp"
#include "aikartos/sch/events.hpp"
#include "aikartos/sch/waiting_tasks_queue.hpp"
#include "aikartos/sync/policies/no_mutex.hpp"
#include "aikartos/sync/stable_priority_queue.hpp"
#include "aikartos/tasks/config.hpp"
#include "aikartos/tasks/object.hpp"
#include "aikartos/utils/object_pool.hpp"

namespace aikartos::sch {

	namespace cfs_like {

		enum class config_flags : std::uint32_t {
		};

		template<typename ConfigT, typename TasksEventsType>
		class scheduler {
			struct vruntime_less;
		public:
			using config = ConfigT;

			constexpr static std::size_t maximum_tasks = config::maximum_tasks;

			using control_block = tasks::control_block;
			using tasks_events_type = TasksEventsType;

			struct scheduler_data_type {
				std::uint32_t vruntime = 0;
				std::uint32_t start = 0;
			};

			using data_object_pool = utils::object_pool<scheduler_data_type, maximum_tasks, 4>;
			using ready_tasks_queue = sync::stable_priority_queue<control_block *, maximum_tasks, vruntime_less, sync::policies::no_mutex>;
			using waiting_queue = waiting_tasks_queue<maximum_tasks, sync::policies::no_mutex>;

			void configure_task(control_block *task, const tasks::config&) {
				task->scheduler_data = static_cast<void*>(data_pool_.alloc());
			}

			void clear_task(control_block *task) {
				data_pool_.free(get_data(task));
			}

			control_block* get_next_task() {
				process_waiting_queue();
				const auto current_ticks = kernel::core::get_tick_count();

				// we have the current task on the top.
				// recalculating task;
				if (auto current = ready_tasks_.try_pop()) {
					auto *task = *current;
					auto data = get_data(task);
					if (data->start != 0) {
						data->vruntime += (current_ticks - data->start);
					}
					ready_tasks_.try_push(task);
				}
#ifdef DEBUG
				fill_vrun_times();
#endif
				// try the next task
				while (auto next_task = ready_tasks_.peek()) {
					auto *task = *next_task;

					switch (task->task.state) {
					case tasks::descriptor::state_type::READY:
						[[fallthrough]];
					case tasks::descriptor::state_type::RUNNING:
						get_data(task)->start = current_ticks;
						return task;
					case tasks::descriptor::state_type::DONE:
						// It's done. Remove it from the queue
						get_data(task)->start = 0;
						ready_tasks_.try_pop();
						tasks_events_type::on_task_done(task);
						break;
					case tasks::descriptor::state_type::WAIT:
						// It's waiting. Remove it from the queue and put it in waiting_tasks_.
						get_data(task)->start = 0;
						ready_tasks_.try_pop();
						waiting_tasks_.try_push(task);
						break;
					default:
						break;
					}
				}

				return nullptr;
			}

			void add_task(control_block *task) {
				ready_tasks_.try_push(task);
#ifdef DEBUG
				fill_vrun_times();
#endif
			}

		private:

			struct vruntime_less {
				bool operator ()(control_block *lhs, control_block *rhs) const {
					return get_data(rhs)->vruntime < get_data(lhs)->vruntime;
				}
			};

			static scheduler_data_type* get_data(control_block *task) {
				return task->get_scheduler_data<scheduler_data_type>();
			}

			void process_waiting_queue() {
				waiting_tasks_.process([this](auto *task) {
					add_task(task);
				});
			}

#ifdef DEBUG
			void fill_vrun_times() {
				std::size_t id = 0;
				ready_tasks_.foreach([this, &id](control_block *task) {
					current_vruns[id++] = get_data(task)->vruntime;
				});
				for (; id < maximum_tasks; ++id) {
					current_vruns[id] = 0;
				}
			}
			std::uint32_t current_vruns[maximum_tasks];
#endif
			ready_tasks_queue ready_tasks_;
			waiting_queue waiting_tasks_;
			data_object_pool data_pool_;
		};

	}
}
