/*
 * memory_allocator_bump.cpp
 *
 *  Created on: May 17, 2025
 *      Author: newenclave
 *  
 */

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
#include "aikartos/memory/allocator_bump.hpp"

#include "aikartos/sch/scheduler_round_robin.hpp"

#include "tests.hpp"

#ifdef ENABLE_TEST_memory_allocation_bump_list

using namespace aikartos;

namespace tests {

	int test::run(void)
	{
		namespace sch_ns = sch::round_robin;

		device::uart::init_tx();
		memory::init<memory::allocator_bump<>>();

		char buf[16];
		auto len = snprintf(buf, 8, "%u\r\n", memory::core::total_memory());
		device::uart::blocking_write(buf, len);

		auto ptr1 = malloc(10);
		auto ptr2 = malloc(1000);
		auto ptr3 = malloc(100000);

		free(ptr1);
		free(ptr2);
		free(ptr3);

		while(1){}
	}
}

#endif
