/*
 * wait_queue.hpp
 *
 *  Created on: May 11, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/kernel/core.hpp"
#include "aikartos/sync/policies/mutex_policy.hpp"
#include "aikartos/sync/priority_queue.hpp"
#include "aikartos/tasks/control_block.hpp"

namespace aikartos::sch {

	template <std::size_t MaximumTasks, sync::policies::MutexPolicy MutexT>
	class waiting_tasks_queue {
	public:

		constexpr static std::size_t maximum_tasks = MaximumTasks;
		using control_block = tasks::control_block<>;
		using mutex_type = MutexT;

		struct wait_less_time {
			bool operator ()(control_block *lhs, control_block *rhs) const {
				return rhs->task.timing.next_run < lhs->task.timing.next_run;
			}
		};
		using wating_tasks_queue_type = sync::priority_queue<control_block *, maximum_tasks, wait_less_time, mutex_type>;

		inline void try_push(control_block *task) {
			queue_.try_push(task);
		}

		template <typename CallBackT>
		inline void process(CallBackT cb) {
			auto const ticks = kernel::core::get_tick_count();
			while(auto next = queue_.peek()) {
				auto *task = next.value();
				if(task->task.timing.next_run <= ticks) {
					queue_.try_pop();
					task->task.state = tasks::descriptor::state_type::READY;
					cb(task);
				}
				else {
					break;
				}
			}
		}

	private:
		wating_tasks_queue_type queue_;
	};

}
