/*
 * kernel_api.hpp
 *
 *  Created on: May 10, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/device/device.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/tasks/control_block.hpp"

namespace aikartos::kernel {
	class api {
	public:
		using task_block = tasks::control_block<>;
		static void yield() {
			SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
		}

		static void sleep(std::uint32_t millieconds) {
			auto *current_tcb = get_current_tcb();
			DEBUG_ASSERT(current_tcb != nullptr, "Bad current task...");
			sleep_task_for(current_tcb, millieconds);
		}

		static void sleep_task_for(task_block *task, std::uint32_t millieconds) {
			task->task.timing.next_run = get_tick_count() + millieconds;
			task->task.state = tasks::descriptor::state_type::WAIT;
			yield();
		}

		static void terminate_current(bool need_yield = true) {
			auto *task = get_current_tcb();
			DEBUG_ASSERT(current_tcb != nullptr, "Bad current task...");
			task->task.state = tasks::descriptor::state_type::DONE;
			if(need_yield && !is_in_interrupt()) {
				yield();
			}
		}

		static bool is_in_interrupt() {
		    uint32_t ipsr;
		    asm volatile ("MRS %0, IPSR" : "=r"(ipsr));
		    return ipsr != 0;
		}

		static task_block *get_current_tcb();
	private:
		static std::uint32_t get_tick_count();
	};
}
