    .syntax unified
    .cpu cortex-m4
    .thumb


.global kernel_launch_impl_
.type kernel_launch_impl_, %function
.align 2
kernel_launch_impl_:
        // Load the address of currentPt (pointer to the current TCB)
        LDR     R0, =current_tcb_ptr

        // Dereference currentPt to get the current TCB
        LDR     R1, [R0]

        // Load the saved stack pointer of the current task (TCB->stackPt)
        LDR     R2, [R1]

        // Set Process Stack Pointer (PSP) to the saved task stack
        MSR     PSP, R2

        // Set CONTROL register: use PSP (bit 1 = 1), remain in privileged mode (bit 0 = 0)
        MOVS    R0, #2
        MSR     CONTROL, R0

        // Ensure the instruction sequence is correctly synchronized
        ISB

        // Restore callee-saved registers R4–R11 from task stack
        POP     {R4-R11}

        // Restore auto-saved registers R0–R3 from task stack
        POP     {R0-R3}

        // Restore R12
        POP     {R12}

        // Restore LR (Link Register)
        POP     {LR}

        // Restore PC (Program Counter) — jump to task entry point
        POP     {PC}
