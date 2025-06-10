/**
 * @file tests/memory_allocator_tlsf.cpp
 * @brief TLSF allocator test using in-place region-based memory.
 *
 * - Initializes a `tlsf::impl::region` allocator over a contiguous memory block at runtime.
 * - Allocator metadata (bucket index table) is stored inside the managed memory region.
 * - Performs randomized allocations and deallocations with stress test conditions.
 * - Demonstrates block splitting on allocation and full coalescing on free.
 * - Validates allocator integrity and heap structure through visual inspection tools.
 *
 *  Created on: May 23, 2025
 *      Author: newenclave
 */



#include "aikartos/kernel/config.hpp"
#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_round_robin.hpp"

#include "aikartos/memory/memory.hpp"
#include "aikartos/memory/allocator_tlsf.hpp"
#include "aikartos/device/uart.hpp"

#include "aikartos/rnd/xorshift32.hpp"

#include "tests.hpp"

#ifdef ENABLE_TEST_memory_allocator_tlsf

using namespace aikartos;

namespace {
}

namespace tests {

	using allocator_type = memory::allocator_tlsf<>;
	using config = kernel::config;
	namespace sch_ns = sch::round_robin;

	constexpr std::size_t POINTERS_COUNT = 8 * 1024;
	constexpr std::size_t MAXIMUM_BLOCK = 1000;
	void *allocated[POINTERS_COUNT];

	int test::run(void)
	{
		kernel::init<sch_ns::scheduler, config>();
		memory::init<allocator_type>();
		device::uart::init_tx();

		auto uart_printer = device::uart::printf<256>;

		uart_printer("MEMORY BLOCK USED: %u\r\n", memory::get_allocator()->total());
		rnd::xorshift32 rng(kernel::core::get_systick_val());

		// Alloc
		for(std::size_t i=0; i<POINTERS_COUNT; ++i) {
			auto next = rng.next() % MAXIMUM_BLOCK + 1;
			allocated[i] = calloc(next, 1);
		}
		uart_printer("After malloc:\r\n\r\n");
		memory::get_allocator()->dump_heap(uart_printer);

		// Free
		for(std::size_t i=0; i<POINTERS_COUNT; ++i) {
			auto next = rng.next() % POINTERS_COUNT;
			free(allocated[next]);
			allocated[next] = nullptr;
		}
		uart_printer("After random free:\r\n\r\n");
		memory::get_allocator()->dump_heap(uart_printer);

		// ReAlloc
		for(std::size_t i=0; i<POINTERS_COUNT; ++i) {
			auto next = rng.next() % MAXIMUM_BLOCK + 1;
			if(allocated[i]) {
				if(auto new_ptr = realloc(allocated[i], next)) {
					allocated[i] = new_ptr;
				}
			}
		}
		uart_printer("After random realloc:\r\n\r\n");
		memory::get_allocator()->dump_heap(uart_printer);

		// free!
		for(std::size_t i=0; i<POINTERS_COUNT; ++i) {
			free(allocated[i]);
		}
		uart_printer("After full free:\r\n\r\n");
		memory::get_allocator()->dump_heap(uart_printer);

		kernel::launch(10);
		PANIC("Should not be here");
	}
}

#endif
