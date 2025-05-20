/*
 * mem_alloc_buddy.cpp
 *
 *  Created on: May 20, 2025
 *      Author: newenclave
 *  
 */




#include "aikartos/kernel/config.hpp"
#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_round_robin.hpp"

#include "aikartos/memory/memory.hpp"
#include "aikartos/memory/allocator_buddy.hpp"
#include "aikartos/device/uart.hpp"

#include "aikartos/rnd/xorshift32.hpp"

#include "tests.hpp"

#ifdef ENABLE_TEST_memory_allocator_buddy

using namespace aikartos;

namespace {
}

namespace tests {

	using allocator_type = memory::allocator_buddy<>;
	using config = kernel::config;
	namespace sch_ns = sch::round_robin;

	constexpr std::size_t POINTERS_COUNT = 10'000;
	constexpr std::size_t MAXIMUM_BLOCK = 1000;

	int test::run(void)
	{
		kernel::init<sch_ns::scheduler, config>();
		memory::init<allocator_type>();
		device::uart::init_tx();
		auto uart_printer = device::uart::printf<256>;

		uart_printer("MEMORY BLOCK USED: %u\r\n", memory::get_allocator()->total());
		rnd::xorshift32 rng(kernel::core::get_systick_val());

		void *allocated[POINTERS_COUNT];

		for(std::size_t i=0; i<POINTERS_COUNT; ++i) {
			allocated[i] = malloc(rng.next() % MAXIMUM_BLOCK + 1);
		}
		uart_printer("After malloc:\r\n\r\n");
		memory::get_allocator()->dump_heap(uart_printer);

		for(std::size_t i=0; i<POINTERS_COUNT; ++i) {
			free(allocated[i]);
		}
		uart_printer("After free:\r\n\r\n");
		memory::get_allocator()->dump_heap(uart_printer);

		kernel::launch(10);
		PANIC("Should not be here");
	}
}

#endif
