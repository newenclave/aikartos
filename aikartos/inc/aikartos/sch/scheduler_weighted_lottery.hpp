/*
 * lottery.hpp
 *
 *  Created on: May 11, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/kernel/core.hpp"
#include "aikartos/rnd/lfsr.hpp"
#include "aikartos/rnd/xorshift128.hpp"
#include "aikartos/rnd/xorshift32.hpp"
#include "aikartos/sch/waiting_tasks_queue.hpp"
#include "aikartos/sync/policies/no_mutex.hpp"
#include "aikartos/tasks/config.hpp"
#include "aikartos/tasks/control_block.hpp"
#include <limits>


namespace aikartos::sch {

	namespace weighted_lottery {

		enum class config_flags: std::uint32_t {
			tickets 		= (1 << 0),
			lose_delta 		= (1 << 1),
			lose_threshold 	= (1 << 2),
			lose_agressive 	= (1 << 3),
			win_delta 		= (1 << 4),
			win_threshold 	= (1 << 5),
			win_agressive 	= (1 << 6),
		};

		template <typename ConfigT, typename TasksEventsType>
		class scheduler {
		public:
			using config = ConfigT;

			constexpr static std::size_t maximum_tasks = config::maximum_tasks;

			using control_block  = tasks::control_block;
			using tasks_events_type = TasksEventsType;

			struct adjustment_info {
				std::uint8_t delta = 1;
				std::uint8_t threshold = 1;
				std::uint32_t rounds = 0;
				bool agressive = false;
			};

			struct scheduler_data_type {
				std::uint8_t tickets = 1;
				std::uint8_t base_tickets = 1;

				adjustment_info win;
				adjustment_info lose;
			};

			constexpr static std::size_t maximum_tikets_value = std::numeric_limits<decltype(scheduler_data_type::tickets)>::max();

			using scheduler_data_allocator = utils::object_pool<scheduler_data_type, maximum_tasks, 4>;
			using waiting_queue = waiting_tasks_queue<maximum_tasks, sync::policies::no_mutex>;
			using ready_array = std::array<control_block *, maximum_tasks>;

			void configure_task(control_block *task, const tasks::config &cfg) {
				auto *sch_data = data_allocator_.alloc();
				task->scheduler_data = static_cast<void *>(sch_data);

				cfg.update_value<config_flags::tickets>(sch_data->tickets);
				ASSERT(sch_data->tickets > 0, "Bad value for 'lottery_tickets'");

				sch_data->base_tickets = sch_data->tickets;

				cfg.update_value<config_flags::lose_delta>(sch_data->lose.delta);
				cfg.update_value<config_flags::lose_threshold>(sch_data->lose.threshold);
				cfg.update_value<config_flags::lose_agressive>(sch_data->lose.agressive);

				cfg.update_value<config_flags::win_delta>(sch_data->win.delta);
				cfg.update_value<config_flags::win_threshold>(sch_data->win.threshold);
				cfg.update_value<config_flags::win_agressive>(sch_data->win.agressive);
			}

			void clear_task(control_block *task) {
				remove_task(task);
				data_allocator_.free(get_data(task));
			}

			control_block* get_next_task() {
				process_waiting_queue();

				control_block *next_task = nullptr;
				while(1) {
					next_task = get_next_task_impl();
					if(!next_task && (0 == ready_count_)) {
						break;
					}
					if(next_task) {
						reset_tickets(next_task);
						adjust_losers(next_task);
						break;
					}
				}
				validate_tickets();
				return next_task;
			}

			void add_task(control_block *task) {
				auto tickets = get_tickets(task);
				for(std::size_t i = 0; i < ready_.size(); i++) {
					if(nullptr == ready_[i]) {
						++ready_count_;
						total_tickets_ += tickets;
						ready_[i] = task;
						break;
					}
				}
				rng_.reset_state(kernel::core::get_systick_val());
			}

		private:

			control_block *get_next_task_impl() {
				if(total_tickets_ > 0) {
					const auto next_win = (rng_.next() % total_tickets_);
					std::uint32_t current_tickets = 0;
					std::size_t task_checked = 0;
					for(std::size_t i = 0; (i < ready_.size()) && (task_checked < ready_count_); i++) {
						if(nullptr != ready_[i]) {
							auto *task = ready_[i];
							const auto tickets = get_tickets(task);
							switch(task->task.state) {
							case tasks::descriptor::state_type::READY:
								[[fallthrough]];
							case tasks::descriptor::state_type::RUNNING:
								current_tickets += tickets;
								if(next_win < current_tickets) {
									decay_winner(task);
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

			void decay_winner(control_block *winner) {
				auto *data = get_data(winner);
				const auto win_rounds = ++data->win.rounds;
				if(data->lose.rounds > 0) {
					data->lose.rounds = 0;
					total_tickets_ -= (data->tickets - data->base_tickets);
					data->tickets = data->base_tickets;
				}
				if(win_rounds >= data->win.threshold) {
					if(data->tickets > data->win.delta) {
						total_tickets_-= data->win.delta;
						data->tickets -= data->win.delta;
					}
					else {
						total_tickets_ -= (data->tickets - 1);
						data->tickets = 1;
					}
					if(!data->win.agressive) {
						data->win.rounds = 0;
					}
				}
			}

			void adjust_losers(control_block *winner) {
				for(std::size_t i = 0; i < ready_.size(); i++) {
					auto *current_task = ready_[i];
					if(current_task && (current_task != winner)) {
						auto *data = get_data(current_task);
						const auto current_tickts = data->tickets;
						const auto threshold = data->lose.threshold;

						if(data->win.rounds > 0) {
							data->win.rounds = 0;
							total_tickets_ += (data->base_tickets - data->tickets);
							data->tickets = data->base_tickets;
						}

						++data->lose.rounds;
						if(data->lose.rounds >= threshold) {
							if((current_tickts + data->lose.delta) <= maximum_tikets_value) {
								total_tickets_ += data->lose.delta;
								data->tickets += data->lose.delta;
							} else {
								const auto delta = maximum_tikets_value - current_tickts;
								total_tickets_ += delta;
								data->tickets = maximum_tikets_value;
							}
						}
						if(!data->lose.agressive) {
							data->lose.rounds = 0;
						}
					}
				}
			}

			void remove_task(control_block *task) {
				auto tickets = get_tickets(task);
				for(std::size_t i = 0; i < ready_.size(); i++) {
					if(task == ready_[i]) {
						--ready_count_;
						total_tickets_ -= tickets;
						ready_[i] = nullptr;
						break;
					}
				}
			}

			inline void validate_tickets() {
#ifdef DEBUG
				std::size_t tickets = 0;
				for(std::size_t i = 0; i < ready_.size(); i++) {
					if(nullptr != ready_[i]) {
						tickets += get_tickets(ready_[i]);
					}
				}
				ASSERT(total_tickets_ == tickets, "Something went wrong!");
#endif
			}

			static scheduler_data_type *get_data(control_block *task) {
				return task->template get_scheduler_data<scheduler_data_type>();
			}

			void reset_tickets(control_block *task) {
				auto *data = get_data(task);
				if(data->tickets > data->base_tickets) {
					total_tickets_ -= (data->tickets - data->base_tickets);
				} else {
					total_tickets_ += (data->base_tickets - data->tickets);
				}
				data->tickets = data->base_tickets;
			}

			static std::uint8_t get_tickets(control_block *task) {
				return get_data(task)->tickets;
			}

			static std::uint8_t get_base_tickets(control_block *task) {
				return get_data(task)->base_tickets;
			}

			void process_waiting_queue() {
				waiting_tasks_.process([this](auto *task){ add_task(task); });
			}

			rnd::xorshift32 rng_;
			std::size_t ready_count_ = 0;
			std::uint32_t total_tickets_ = 0;
			waiting_queue waiting_tasks_;
			ready_array ready_ {};
			scheduler_data_allocator data_allocator_;
		};
	}
}
