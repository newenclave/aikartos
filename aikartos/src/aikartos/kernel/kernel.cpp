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
			static volatile std::uint32_t counter = 0;
			static volatile std::uint32_t quanta = core::get_quanta();

#if 0
			counter += 1;
			if((constants::quanta_infinite != quanta) && (counter >= quanta)) {
				counter = 0;
				SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
			}
#else
			volatile std::uint32_t current_quanta = core::get_quanta();
			if(auto *hook = kernel::core::get_systick_hook()) {
				if(hook(kernel::core::get_systick_hook_parameter())) {
					counter = 0;
					SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
				}
			}
			else
			{
				if(current_quanta != quanta) {
					quanta = current_quanta;
					counter = 0;
				}

				counter += 1;
				if((constants::quanta_infinite != quanta) && (counter >= quanta)) {
					counter = 0;
					SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
				}
			}
#endif
		}

		static void pendsv_handler() {
			while (1) {
			    auto [next, event] = kernel::core::instance_->get_next_task();
			    g_current_tcb_ptr = next;
			    if (event != sch::events::OK && kernel::core::get_scheduler_event_handler()) {
			        auto decision = kernel::core::get_scheduler_event_handler()(event);
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
	void core::init_first_task() {
	    auto [next, _] = instance_->get_next_task();
		g_current_tcb_ptr = next;
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

#if defined(PLATFORM_USE_FPU)
/*
 * Pseudo-code for PendSV_Handler (implemented in assembly):
 *
 *{
 *	disable_irq();
 *
 *	// Save current task context
 *	if (g_current_tcb_ptr->flags & tasks::task_flags::use_fpu) {
 *		store(PSP, {s16–s31});
 *		// remember: we store the registers for this task and will need to restore them
 *		g_current_tcb_ptr->flags |= tasks::task_flags::fpu_saved;
 *	}
 *	store(PSP, {R4–R11});
 *	g_current_tcb_ptr->stack = PSP;
 *
 *	// Perform context switch (updates g_current_tcb_ptr)
 *	pendsv_handler_impl();
 *
 *	// Now g_current_tcb_ptr points to the next task
 *
 *	// Restore context of the new task
 *	restore(g_current_tcb_ptr->stack, {R4–R11});
 *
 *	if (g_current_tcb_ptr->flags & tasks::task_flags::use_fpu) {
 *		result = 0xFFFFFFED; // Return with FPU frame
 *	} else {
 *		result = 0xFFFFFFFD; // Return without FPU frame
 *	}
 *
 *	// Restore FPU context if previously saved
 *	if (g_current_tcb_ptr->flags & tasks::task_flags::fpu_saved) {
 *		restore(g_current_tcb_ptr->stack, {s16–s31});
 *	}
 *
 *	PSP = g_current_tcb_ptr->stack;
 *
 *	enable_irq();
 *
 *	return result; // Written to LR
 *
 *}
 *
 **/
	extern "C" __attribute__((naked)) void PendSV_Handler(void) {
		__asm volatile ("CPSID   I");

		// Save current task context: read PSP
		asm volatile ("MRS     R0, PSP");
		asm volatile ("LDR     R1, =g_current_tcb_ptr");
		asm volatile ("LDR     R2, [R1]"); 	   // R2 = g_current_tcb_ptr
		asm volatile ("LDR     R3, [R2, #4]"); // R3 = g_current_tcb_ptr->flags

		asm volatile ("TST     R3, #(1 << 0)"); // if(flags & tasks::task_flags::use_fpu)
		asm volatile ("BEQ     no_fpu_use");

		asm volatile ("VSTMDB  R0!, { s16 - s31 }");

		asm volatile ("ORR     R3, R3, #(1 << 1)"); // flags |= tasks::task_flags::fpu_saved
		asm volatile ("STR     R3, [R2, #4]");
		asm volatile ("B       no_fpu_use_final");

		asm volatile ("no_fpu_use:");
		asm volatile ("BIC     R3, R3, #(1 << 1)"); // flags &= ~tasks::task_flags::fpu_saved
		asm volatile ("STR     R3, [R2, #4]");

	asm volatile ("no_fpu_use_final:");

		// Push callee-saved registers R4–R11 onto current task's stack
		asm volatile ("STMDB   R0!, {R4-R11}");

		// Save updated PSP into current TCB
		asm volatile ("STR     R0, [R2]");

		// Call C function to switch to next task
		asm volatile ("PUSH    {LR}");
		asm volatile ("BL      pendsv_handler_impl");
		asm volatile ("POP     {LR}");

		// New task...
		// Load new current_tcb_ptr after switch
		asm volatile ("LDR     R1, =g_current_tcb_ptr");
		asm volatile ("LDR     R2, [R1]"); // R2 = g_current_tcb_ptr

		// Load new task's saved stack pointer
		asm volatile ("LDR     R0, [R2, #0]"); // R0 = g_current_tcb_ptr->stack

		// Restore callee-saved registers for new task
		asm volatile ("LDMIA   R0!, {R4-R11}");

		asm volatile ("LDR     R3, [R2, #4]");  // R3 = g_current_tcb_ptr->flags

		/******/
		asm volatile ("TST     R3, #(1 << 0)"); //if (flags & tasks::task_flags::use_fpu)
		asm volatile ("BEQ     not_using_fpu");

		//; FPCA = 1
		//	    asm volatile ("MOVW    LR, #0xFFED");
		//	    asm volatile ("MOVT    LR, #0xFFFF");
		asm volatile ("LDR     LR, =0xFFFFFFED"); //EXC_RETURN with FPU
		asm volatile ("B       done_fpca");

	asm volatile ("not_using_fpu:");

		// FPCA = 0
		//	    asm volatile ("MOVW    LR, #0xFFFD");
		//	    asm volatile ("MOVT    LR, #0xFFFF");
		asm volatile ("LDR     LR, =0xFFFFFFFD"); //EXC_RETURN with NO FPU

	asm volatile ("done_fpca :");
		/******/

		asm volatile ("TST     R3, #(1 << 1)");     // if (flags & tasks::task_flags::fpu_saved)
		asm volatile ("BEQ     no_fpu_saved");

		// flags & TCB_FLAG_USE_FPU != 0
		asm volatile ("VLDMIA  R0!, {s16-s31}");

		// flags & TCB_FLAG_USE_FPU == 0
	asm volatile ("no_fpu_saved:");

		// Set PSP to point to new task's stack
		asm volatile ("MSR     PSP, R0");

		// Re-enable interrupts
		asm volatile ("CPSIE   I");

		// Return from interrupt - remaining registers restored automatically (R0–R3, R12, LR, PC, xPSR)
		asm volatile ("BX      LR");
	}

#else
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

		// Return from interrupt - remaining registers restored automatically (R0–R3, R12, LR, PC, xPSR)
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
