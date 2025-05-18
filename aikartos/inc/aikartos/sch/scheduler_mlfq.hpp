/**
 * @file scheduler_mlfq.hpp
 * @brief Multilevel Feedback Queue (MLFQ) scheduler with configurable per-task quanta and boost logic.
 *
 * - Each task defines its own quantum levels for multiple priority queues.
 * - Tasks start at the highest level and degrade if they consume full quanta.
 * - Tasks that yield or block voluntarily can stay at higher levels longer.
 * - A global boost mechanism periodically resets task levels to prevent starvation.
 *
 * This scheduler dynamically adapts to task behavior, balancing fairness and responsiveness.
 *
 *  Created on: May 17, 2025
 *      Author: newenclave
 *  
 */

#pragma once 

#include "aikartos/kernel/core.hpp"
#include "aikartos/sch/events.hpp"
#include "aikartos/sch/waiting_tasks_queue.hpp"
#include "aikartos/sch/statistic.hpp"

#include "aikartos/sync/policies/no_mutex.hpp"
#include "aikartos/sync/priority_queue.hpp"
#include "aikartos/sync/irq_critical_section.hpp"

#include "aikartos/tasks/config.hpp"
#include "aikartos/tasks/object.hpp"
#include "aikartos/utils/object_pool.hpp"

namespace aikartos::sch {

	namespace mlfq {

		enum class config_flags : std::uint32_t {
			levels = (1u << 0u),
			boost_quanta = (1u << 1u),
		};

		enum class statistics_fields : std::uint32_t {
			level = 0u,
			state = 1u,
			task_entry = 2u,
			task_param = 3u,
		};

		struct quantum_levels {
			std::uint8_t high = 10;
			std::uint8_t middle = 20;
			std::uint8_t low = 40;
		};

		template <typename ConfigT, typename TasksEventsType>
		class scheduler {

			struct systick_hook {
				static bool call(void *param) {
					auto *inst = static_cast<scheduler *>(param);
					return inst->process_cuurent();
				}
			};

			struct quantum_level_less;

		public:
			using config = ConfigT;

			constexpr static std::size_t maximum_tasks = config::maximum_tasks;
			constexpr static std::size_t maximum_levels = 3;
			constexpr static std::uint32_t global_boost_value = 500;

			using control_block = tasks::control_block;
			using tasks_events_type = TasksEventsType;

			struct scheduler_data_type {
				std::uint8_t levels[maximum_levels];
				std::uint32_t quantum_used = 0;
				std::uint32_t level = 0;
				std::uint32_t last_boost = 0;
				std::uint32_t boost_quanta = 500;
				scheduler_data_type() noexcept {
					levels[0] = 10;
					levels[1] = 20;
					levels[2] = 40;
				}
			};

			using data_object_pool = utils::object_pool<scheduler_data_type, maximum_tasks, 4>;
			using waiting_queue = waiting_tasks_queue<maximum_tasks, sync::policies::no_mutex>;
			using level_queue_type = sync::priority_queue<control_block*, maximum_tasks, quantum_level_less, sync::policies::no_mutex>;
			using levels_array = std::array<level_queue_type, maximum_levels>;

			void configure_task(control_block *task, const tasks::config &cfg) {
				task->scheduler_data = static_cast<void *>(data_pool_.alloc());
				auto *sch_data = get_data(task);

				std::uintptr_t ql_ptr_value = 0;
				cfg.update_value<config_flags::levels>(ql_ptr_value);
				quantum_levels *ql_ptr = reinterpret_cast<quantum_levels *>(ql_ptr_value);

				if(ql_ptr) {
					sch_data->levels[0] = ql_ptr->high;
					sch_data->levels[1] = ql_ptr->middle;
					sch_data->levels[2] = ql_ptr->low;
				};
				cfg.update_value<config_flags::boost_quanta>(sch_data->boost_quanta);
			}

			void clear_task(control_block *task) {
				data_pool_.free(get_data(task));
			}

			control_block* get_next_task() {
				[[maybe_unused]] static bool _ = [this] {
					kernel::core::register_systick_hook(&systick_hook::call, this);
					return true;
				}();

				const auto current_ticks = kernel::core::get_tick_count();
				if((current_ticks - last_boost_)>= global_boost_value) {
					last_boost_ = current_ticks;
					boost_levels();
				}
				process_waiting_queue();

				for(std::size_t id = 0; id < levels_.size(); ++id) {
					if((current_ = get_next_task_impl(id))) {
						return current_;
					}
				}

				return nullptr;
			}

			void add_task(control_block *task) {
				levels_[get_data(task)->level].try_push(task);
			}

			bool get_statistic(sch::statistic_base &stat) {
				// disabling IRQs here
				sync::irq_critical_section irqd;
				std::size_t current_task_id = 0;

				const auto get_stat = [this, &current_task_id, &stat](auto *task) {
					auto *data = get_data(task);
					stat.add_field(current_task_id, static_cast<std::size_t>(statistics_fields::level),
							static_cast<std::uintptr_t>(data->level));
					stat.add_field(current_task_id, static_cast<std::size_t>(statistics_fields::state),
							static_cast<std::uintptr_t>(task->task.state));
					stat.add_field(current_task_id, static_cast<std::size_t>(statistics_fields::task_entry),
							reinterpret_cast<std::uintptr_t>(task->task.task));
					stat.add_field(current_task_id, static_cast<std::size_t>(statistics_fields::task_param),
							reinterpret_cast<std::uintptr_t>(task->task.parameter));
					current_task_id++;
				};

				// check all the levels
				for(std::size_t id = 0; id < levels_.size(); ++id) {
					levels_[id].foreach(get_stat);
				}
				waiting_tasks_.foreach(get_stat);
				return true;
			}

		private:

			void boost_levels() {
				const auto current_ticks = kernel::core::get_tick_count();
				for(std::size_t id = 1; id < levels_.size(); ++id) {
					levels_[id].foreach([id, current_ticks](control_block *task){
						auto *sch_data = get_data(task);
						if((current_ticks - sch_data->last_boost) >= sch_data->boost_quanta) {
							sch_data->last_boost = current_ticks;
							sch_data->level = 0;
							sch_data->quantum_used = 0;
						}
					});
				}
			}

			control_block *get_next_task_impl(std::size_t level) {
				while(auto next = levels_[level].try_pop()) {
					auto *task = *next;
					switch(task->task.state) {
					case tasks::descriptor::state_type::READY:
						[[fallthrough]];
					case tasks::descriptor::state_type::RUNNING: {
						const auto level = get_data(task)->level;
						levels_[level].try_push(task);
						}
						return task;
					case tasks::descriptor::state_type::DONE:
						tasks_events_type::on_task_done(task);
						break;
					case tasks::descriptor::state_type::WAIT:
						get_data(task)->quantum_used = 0;
						waiting_tasks_.try_push(task);
						break;
					default:
						break;
					}
				}
				return nullptr;
			}

			struct quantum_level_less {
				bool operator ()(control_block *lhs, control_block *rhs) const {
					return get_data(rhs)->quantum_used < get_data(lhs)->quantum_used;
				}
			};

			static scheduler_data_type *get_data(control_block *task) {
				return task->get_scheduler_data<scheduler_data_type>();
			}

			bool process_cuurent() {
				if(!current_) {
					return false;
				}
				auto *sch_data = get_data(current_);
				if(++sch_data->quantum_used >= sch_data->levels[sch_data->level]) {
					sch_data->quantum_used = 0;
					if(sch_data->level < (maximum_levels - 1)) {
						sch_data->level++;
					}
					return true;
				}
				return false;
			}

			void process_waiting_queue() {
				waiting_tasks_.process([this](auto *task) { add_task(task); });
			}

			std::uint32_t last_boost_ = 0;
			control_block *current_ = nullptr;
			levels_array levels_;
			waiting_queue waiting_tasks_;
			data_object_pool data_pool_;
		};

	}
}
