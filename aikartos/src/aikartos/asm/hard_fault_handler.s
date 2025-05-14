    .syntax unified
    .cpu cortex-m4
    .thumb

    .global HardFault_Handler1
    .type HardFault_Handler1, %function

HardFault_Handler1:
    TST LR, #4		// LR & 4
    ITE NE 			// If not eq
    MRSNE R0, PSP   // them
    MRSEQ R0, MSP   // else

    MOV R1, LR
    PUSH {R1}
    LDR R2, =hard_fault_handler
    BLX R2

    B .
