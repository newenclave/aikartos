/*
 * lottery.hpp
 *
 *  Created on: May 11, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/kernel/core.hpp"
#include "aikartos/rnd/lfsr.hpp"
#include "aikartos/rnd/xorshift32.hpp"
#include "aikartos/sch/waiting_tasks_queue.hpp"
#include "aikartos/sync/policies/no_mutex.hpp"
#include "aikartos/tasks/config.hpp"
#include "aikartos/tasks/control_block.hpp"

namespace aikartos::sch {

	namespace lottery {

		enum class config_flags: std::uint32_t {
			tickets = (1 << 0),
		};

		template <typename ConfigT, typename TasksEventsType>
		class scheduler {
		public:
			using config = ConfigT;

			constexpr static std::size_t maximum_tasks = config::maximum_tasks;

			using control_block  = tasks::control_block;
			using tasks_events_type = TasksEventsType;

			struct scheduler_data_type {
				std::uint8_t tickets = 1;
			};

			using scheduler_data_allocator = utils::object_pool<scheduler_data_type, maximum_tasks, 4>;
			using waiting_queue = waiting_tasks_queue<maximum_tasks, sync::policies::no_mutex>;
			using ready_array = std::array<control_block *, maximum_tasks>;

			void configure_task(control_block *task, const tasks::config &cfg) {
				auto *sch_data = data_allocator_.alloc();
				task->scheduler_data = static_cast<void *>(sch_data);

				cfg.update_value<config_flags::tickets>(sch_data->tickets);
				ASSERT(sch_data->tickets > 0, "Bad value for 'lottery_tickets'");

			}

			void clear_task(control_block *task) {
				remove_task(task);
				data_allocator_.free(get_data(task));
			}

			control_block* get_next_task() {
				process_waiting_queue();

				if(total_tickets_ > 0) {
					const auto next_win = (rng_.next() % total_tickets_);
					std::uint32_t current_tickets = 0;
					std::size_t task_checked = 0;
					for(std::size_t i = 0; (i < ready_.size()) && (task_checked < ready_count_); i++) {
						if(nullptr != ready_[i]) {
							task_checked++;
							auto *task = ready_[i];
							const auto tickets = get_tickets(task);
							switch(task->task.state) {
							case tasks::descriptor::state_type::READY:
								[[fallthrough]];
							case tasks::descriptor::state_type::RUNNING:
								current_tickets += tickets;
								if(next_win < current_tickets) {
									return task;
								}
								break;
							case tasks::descriptor::state_type::DONE:
								tasks_events_type::on_task_done(task);
								break;
							case tasks::descriptor::state_type::WAIT:
								remove_task(task);
								waiting_tasks_.try_push(task);
								break;
							default:
								break;
							}
						}
					}

				}

				return nullptr;
			}

			void add_task(control_block *task) {
				auto tickets = get_tickets(task);
				for(std::size_t i = 0; i < ready_.size(); i++) {
					if(nullptr == ready_[i]) {
						ready_count_++;
						total_tickets_ += tickets;
						ready_[i] = task;
						break;
					}
				}
			}

		private:

			void remove_task(control_block *task) {
				auto tickets = get_tickets(task);
				for(std::size_t i = 0; i < ready_.size(); i++) {
					if(task == ready_[i]) {
						ready_count_--;
						total_tickets_ -= tickets;
						ready_[i] = nullptr;
						break;
					}
				}
			}

			static scheduler_data_type *get_data(control_block *task) {
				return task->template get_scheduler_data<scheduler_data_type>();
			}

			static std::uint8_t get_tickets(control_block *task) {
				return get_data(task)->tickets;
			}

			void process_waiting_queue() {
				waiting_tasks_.process([this](auto *task){ add_task(task); });
			}

			rnd::xorshift32 rng_;
			std::size_t ready_count_;
			std::uint32_t total_tickets_ = 0;
			waiting_queue waiting_tasks_;
			ready_array ready_ {};
			scheduler_data_allocator data_allocator_;
		};
	}

}
