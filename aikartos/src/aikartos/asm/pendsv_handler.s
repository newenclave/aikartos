    .syntax unified
    .cpu cortex-m4
    .thumb

.global PendSV_Handler1
.type PendSV_Handler1, %function
.align 2
PendSV_Handler1:   //;save r0,r1,r2,r3,r12,lr,pc,psr
        // Disable interrupts during context switch
        CPSID   I

        // Save current task context: read PSP
        MRS     R0, PSP
        // Push callee-saved registers R4–R11 onto current task's stack
        STMDB   R0!, {R4-R11}

        // Load address of current_tcb_ptr (global pointer to current task)
        LDR     R1, =g_current_tcb_ptr

        // Load current TCB (current_tcb_ptr)
        LDR     R2, [R1]

        // Save updated PSP into current TCB
        STR     R0, [R2]

        // Call C function to switch to next task: current_tcb_ptr = scheduler.get_next()
        PUSH    {LR}
        BL      pendsv_handler_impl
        POP     {LR}

         // Load new current_tcb_ptr after switch
        LDR     R1, =g_current_tcb_ptr
        LDR     R2, [R1]

        // Load new task's saved stack pointer
        LDR     R0, [R2, #0]

        // Restore callee-saved registers for new task
        LDMIA   R0!, {R4-R11}

        // Set PSP to point to new task's stack
        MSR     PSP, R0

        // Re-enable interrupts
        CPSIE   I

        // Return from interrupt — remaining registers restored automatically (R0–R3, R12, LR, PC, xPSR)
        BX      LR

