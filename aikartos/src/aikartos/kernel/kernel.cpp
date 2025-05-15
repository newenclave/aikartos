/*
 * kernel.cpp
 *
 *  Created on: May 9, 2025
 *      Author: newenclave
 */

#include "aikartos/kernel/core.hpp"

aikartos::kernel::core::task_block *g_current_tcb_ptr = nullptr;

namespace aikartos::kernel {
	struct handlers_friend {

		static void systick_handler() {
			kernel::core::tick_count_ += 1;
			static std::uint32_t counter = 0;
			static std::uint32_t quanta = core::get_quanta();
			std::uint32_t current_quanta = core::get_quanta();

			if(current_quanta != quanta) {
				quanta = current_quanta;
				counter = 0;
			}

			if((constants::quanta_infinite != quanta) && (++counter >= quanta)) {
				counter = 0;
				SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
			}
		}

		static void pendsv_handler() {
			while (1) {
			    auto [next, event] = kernel::core::instance_->get_next_task();
			    g_current_tcb_ptr = next;
			    if (event != sch::events::OK && kernel::core::instance_->set_scheduler_event_handler_) {
			        auto decision = kernel::core::instance_->set_scheduler_event_handler_(event);
			        if (decision == sch::decision::RETRY) {
			            continue;
			        }
			    }
			    break;
			}
		}
	};

	/// core
	void core::add_task(core::task_entry task, const tasks::config &config, core::task_parameter parameter) {
		auto added = instance_->add_task(task, parameter, config);
		if(nullptr == g_current_tcb_ptr) {
			g_current_tcb_ptr = added;
		}
	}
	// core

	/// kernel::api
	core::task_block *api::get_current_tcb() {
		__disable_irq();
		auto cptr = g_current_tcb_ptr;
		__enable_irq();
		return cptr;
	}

	std::uint32_t api::get_tick_count() {
		return core::get_tick_count();
	}
	/// kernel::api
}


extern "C" {

	void pendsv_handler_impl() {
		aikartos::kernel::handlers_friend::pendsv_handler();
	}

	void SysTick_Handler() {
		aikartos::kernel::handlers_friend::systick_handler();
	}

#if 1
	__attribute__((naked)) void PendSV_Handler() {
		asm volatile ("CPSID   I");

		// Save current task context: read PSP
		asm volatile ("MRS     R0, PSP");

		// Push callee-saved registers R4–R11 onto current task's stack
		asm volatile ("STMDB   R0!, {R4-R11}");

		// Load address of current_tcb_ptr (global pointer to current task)
		asm volatile ("LDR     R1, =g_current_tcb_ptr");

		// Load current TCB (current_tcb_ptr)
		asm volatile ("LDR     R2, [R1]");

		// Save updated PSP into current TCB
		asm volatile ("STR     R0, [R2]");

		// Call C function to switch to next task: current_tcb_ptr = scheduler.get_next()
		asm volatile ("PUSH    {LR}");
		asm volatile ("BL      pendsv_handler_impl");
		asm volatile ("POP     {LR}");

		// Load new current_tcb_ptr after switch
		asm volatile ("LDR     R1, =g_current_tcb_ptr");
		asm volatile ("LDR     R2, [R1]");

		// Load new task's saved stack pointer
		asm volatile ("LDR     R0, [R2, #0]");

		// Restore callee-saved registers for new task
		asm volatile ("LDMIA   R0!, {R4-R11}");

		// Set PSP to point to new task's stack
		asm volatile ("MSR     PSP, R0");

		// Re-enable interrupts
		asm volatile ("CPSIE   I");

		// Return from interrupt — remaining registers restored automatically (R0–R3, R12, LR, PC, xPSR)
		asm volatile ("BX      LR");
	}
#endif

	__attribute__((naked)) void kernel_launch_impl() {
		// Load the address of currentPt (pointer to the current TCB)
		asm volatile ("LDR     R0, =g_current_tcb_ptr");

		// Dereference currentPt to get the current TCB
		asm volatile ("LDR     R1, [R0]");

		// Load the saved stack pointer of the current task (TCB->stackPt)
		asm volatile ("LDR     R2, [R1]");

		// Set Process Stack Pointer (PSP) to the saved task stack
		asm volatile ("MSR     PSP, R2");

		// Set CONTROL register: use PSP (bit 1 = 1), remain in privileged mode (bit 0 = 0)
		asm volatile ("MOVS    R0, #2");
		asm volatile ("MSR     CONTROL, R0");

		// Ensure the instruction sequence is correctly synchronized
		asm volatile ("ISB");

		// Restore callee-saved registers R4–R11 from task stack
		asm volatile ("POP     {R4-R11}");

		// Restore auto-saved registers R0–R3 from task stack
		asm volatile ("POP     {R0-R3}");

		// Restore R12
		asm volatile ("POP     {R12}");

		// Restore LR (Link Register)
		asm volatile ("POP     {LR}");

		// Restore PC (Program Counter) — jump to task entry point
		asm volatile ("POP     {PC}");
	}
}
