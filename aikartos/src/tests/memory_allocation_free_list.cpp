/*
 * memory_allocation_free_list.cpp
 *
 *  Created on: May 17, 2025
 *      Author: newenclave
 *  
 */

#include <stdio.h>

#include "aikartos/device/uart.hpp"
#include "aikartos/kernel/config.hpp"
#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"

#include "aikartos/memory/memory.hpp"
#include "aikartos/memory/allocator_free_list.hpp"

#include "aikartos/sch/scheduler_round_robin.hpp"

#include "tests.hpp"

#ifdef ENABLE_TEST_memory_allocation_free_list

using namespace aikartos;

namespace tests {

	int test::run(void)
	{
		device::uart::init_tx();
		memory::init<memory::allocator_free_list<>>();

		auto printer = device::uart::printf<256>;

		printer("TOTAL MEMEORY: %u\r\n", memory::core::total_memory());

		auto ptr1 = malloc(10);
		printer("After ptr1 = alloc(10):\r\n");
		memory::get_allocator()->dump_info(printer);
		printer(".....\r\n");

		auto ptr2 = malloc(1000);
		printer("After ptr2 = alloc(100):\r\n");
		memory::get_allocator()->dump_info(printer);
		printer(".....\r\n");

		auto ptr3 = malloc(555);
		printer("After ptr3 = alloc(555):\r\n");
		memory::get_allocator()->dump_info(printer);
		printer(".....\r\n");

		free(ptr1);
		printer("After free(ptr1):\r\n");
		memory::get_allocator()->dump_info(printer);
		printer(".....\r\n");
		free(ptr2);
		printer("After free(ptr2):\r\n");
		memory::get_allocator()->dump_info(printer);
		printer(".....\r\n");

		auto ptr4 = malloc(50);
		printer("After ptr4 = alloc(50):\r\n");
		memory::get_allocator()->dump_info(printer);
		printer(".....\r\n");

		free(ptr3);
		printer("After free(ptr3):\r\n");
		memory::get_allocator()->dump_info(printer);
		printer(".....\r\n");

		free(ptr4);
		printer("After free(ptr4):\r\n");
		memory::get_allocator()->dump_info(printer);
		printer(".....\r\n");

		while(1){}
		PANIC("Should not be here");
	}
}

#endif
