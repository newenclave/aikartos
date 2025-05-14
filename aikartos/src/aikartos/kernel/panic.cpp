/*
 * panic.cpp
 *
 *  Created on: May 1, 2025
 *      Author: newenclave
 */

#include "aikartos/device/device.hpp"
#include "aikartos/kernel/panic.hpp"
#include <stdio.h>
//#include "uart.hpp"

extern "C" void hard_fault_handler(std::uint32_t* stack_frame) {

    volatile auto hfsr = SCB->HFSR;	// HardFault status
    volatile auto cfsr = SCB->CFSR;	// Configurable Fault Status Register
    									// SCB_CFSR_USGFAULTSR_Msk/Pos, SCB_CFSR_BUSFAULTSR_Msk/Pos, SCB_CFSR_MEMFAULTSR_Msk/Pos
    volatile auto bfar = SCB->BFAR;	// Bus Fault Address
    volatile auto mmar = SCB->MMFAR;	// MemManage Fault Address

    volatile auto exc_return = stack_frame[5];	// LR (EXC_RETURN)

    static char block[256];
    auto len = snprintf(block, 255,
        "HardFault occurred!\r\n"
        "\tHFSR: %08lX\r\n"
        "\tCFSR: %08lX\r\n"
        "\tBFAR: %08lX\r\n"
        "\tMMAR: %08lX\r\n"
        "\tEXC_RETURN: %08lX\r\n",
        static_cast<std::uint32_t>(hfsr),
        static_cast<std::uint32_t>(cfsr),
        static_cast<std::uint32_t>(bfar),
        static_cast<std::uint32_t>(mmar),
        static_cast<std::uint32_t>(exc_return)
    );

    (void)len;
    //uart::blocking_write(block, len);

    while (1);
}

void panic(const char* reason, const char* file, int line) {
    __disable_irq();
    static char block[64];
    auto len = snprintf(block, 64, "KERNEL PANIC: %s\r\nFile: %s, Line: %d\r\n", reason, file, line);
    (void)len;

    //uart::blocking_write(block, len);
	while (1);
}


extern "C" __attribute__((naked)) void HardFault_Handler() {
	asm volatile ("TST LR, #4");		// LR & 4
	asm volatile ("ITE NE");			// If not eq
	asm volatile ("MRSNE R0, PSP");   // them
	asm volatile ("MRSEQ R0, MSP");   // else

	asm volatile ("MOV R1, LR");
	asm volatile ("PUSH {R1}");
	asm volatile ("LDR R2, =hard_fault_handler");
	asm volatile ("BLX R2");

	asm volatile ("B .");
}

