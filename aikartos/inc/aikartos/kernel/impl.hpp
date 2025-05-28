/*
 * kernel_core.hpp
 *
 *  Created on: May 9, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/kernel/impl_base.hpp"

#include "aikartos/kernel/panic.hpp"
#include "aikartos/sync/irq_critical_section.hpp"

#include "aikartos/utils/container_of.hpp"
#include "aikartos/utils/object_pool.hpp"

#include "aikartos/kernel/api.hpp"
#include "aikartos/kernel/core.hpp"

namespace aikartos::kernel {

	template <
		template<typename, typename> typename SchedulerT,
		typename ConfigT
	>
	class impl: public impl_base {
	public:

		using config_type = ConfigT;
		constexpr static std::uint32_t stack_size = config_type::stack_size;
		constexpr static std::uint32_t maximum_tasks = config_type::maximum_tasks;

		using task_object = tasks::object<stack_size>;

	private:
		struct scheduler_callbacks {
			static void on_task_done(tasks::control_block *object) {
				scheduler_.clear_task(object);

				sync::irq_critical_section dirq;
				pool_.free(utils::container_of<task_object>(object, &task_object::tcb));
			}
			static void on_quanta_change(std::uint32_t quanta) {
				kernel::impl_base::quanta_ = quanta;
			};
		};

	public:

		impl() {
			task_object_staсk_init(idle_.tcb, reinterpret_cast<std::uint32_t>(&impl::task_idle));
		}

		using scheduler_type = SchedulerT<config_type, scheduler_callbacks>;

		using control_block = tasks::control_block;
		using task_entry = impl_base::task_entry;
		using task_parameter = impl_base::task_parameter;

		control_block *add_task(task_entry task, task_parameter parameter, const tasks::config &config) override {

			sync::irq_critical_section dirq;

			auto object = pool_.alloc();
			task_object_staсk_init(object->tcb, reinterpret_cast<std::uint32_t>(task_wrapper));

			object->tcb.task.state = tasks::descriptor::state_type::READY;
			object->tcb.task.task = task;
			object->tcb.task.parameter = parameter;

			scheduler_.configure_task(&object->tcb, config);
			scheduler_.add_task(&object->tcb);

			return &object->tcb;
		}

		std::tuple<control_block *, sch::scheduler_specific_event> get_next_task() override {
			if constexpr (std::is_same_v<decltype(scheduler_.get_next_task()), control_block *>) {
				auto next_tcb = scheduler_.get_next_task();
				return { next_tcb ? next_tcb : &idle_.tcb, sch::events::OK };
			}
			else {
				auto [next_tcb, event] = scheduler_.get_next_task();
				return { next_tcb ? next_tcb : &idle_.tcb, event };
			}
		}

		bool get_scheduler_statistic(sch::statistic_base &stat) override {
			if constexpr (sch::HasGetStatistic<scheduler_type>) {
				return scheduler_.get_statistic(stat);
			} else {
				return false;
			}
		};

	private:

		static void task_idle() {
		    while (true) {
		    	config::idle_hook();
		        __WFI();
		    }
		}

		static void task_wrapper() {
			auto tcb = kernel::api::get_current_tcb();
			ASSERT(tcb, "No current TCB...");
			if(tcb->task.task) {
				tcb->task.state = tasks::descriptor::state_type::RUNNING;
				tcb->task.task(tcb->task.parameter);
			}
			tcb->task.state = tasks::descriptor::state_type::DONE;
			kernel::api::yield();
		}

		void task_object_staсk_init(control_block &tcb, int32_t task) {

			tcb.push<std::uint32_t>(xPSR_T_Msk); // 0x01000000
			tcb.push<std::uint32_t>(task);
			tcb.push<std::uint32_t>(0xFFFFFFFD);  //LR
			//tcb.push<std::uint32_t>(0x14141414);  //R14
			tcb.push<std::uint32_t>(0x12121212);  //R12
			tcb.push<std::uint32_t>(0x03030303);  //R3
			tcb.push<std::uint32_t>(0x02020202);  //R2
			tcb.push<std::uint32_t>(0x01010101);  //R1
			tcb.push<std::uint32_t>(0x00000000);  //R0

			/*  We have to save manually  */
			tcb.push<std::uint32_t>(0x11111111); //R11
			tcb.push<std::uint32_t>(0x10101010); //R10
			tcb.push<std::uint32_t>(0x09090909); //R9
			tcb.push<std::uint32_t>(0x08080808); //R8
			tcb.push<std::uint32_t>(0x07070707); //R7
			tcb.push<std::uint32_t>(0x06060606); //R6
			tcb.push<std::uint32_t>(0x05050505); //R5
			tcb.push<std::uint32_t>(0x04040404); //R4
		}

		inline static utils::object_pool<task_object, maximum_tasks> pool_;
		inline static scheduler_type scheduler_;
		inline static tasks::object<400> idle_;
	};
}
