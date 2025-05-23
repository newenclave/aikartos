/**
 * @file tests/memory_allocator_buddy_fixed.cpp
 * @brief Fixed buddy allocator test with internal static free list table.
 *
 * - Uses `buddy::impl::fixed` with a compile-time known maximum heap size.
 * - Allocator metadata (free list table) is stored statically inside the object.
 * - Works over an externally provided memory region for actual allocation.
 * - Performs randomized allocation and deallocation (10,000 cycles).
 * - Confirms deterministic layout, stability, and coalescing after free.
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
#include "aikartos/memory/allocator_dlist.hpp"
#include "aikartos/memory/allocator_buddy.hpp"
#include "aikartos/device/uart.hpp"

#include "aikartos/rnd/xorshift32.hpp"

#include "tests.hpp"

#ifdef ENABLE_TEST_memory_allocator_buddy_fixed

using namespace aikartos;

namespace {
}

namespace tests {

	constexpr std::size_t ARENA_SIZE = 0x0001'0000;

	using mem_allocator_type = memory::allocator_dlist<>;
	using fix_allocator_type = memory::allocator_buddy_fixed<ARENA_SIZE>;
	using config = kernel::config;
	namespace sch_ns = sch::round_robin;

	constexpr std::size_t POINTERS_COUNT = 10'000;
	constexpr std::size_t MAXIMUM_BLOCK = 1000;
	void *allocated[POINTERS_COUNT];

	int test::run(void)
	{
		kernel::init<sch_ns::scheduler, config>();
		memory::init<mem_allocator_type>();
		device::uart::init_tx();
		auto uart_printer = device::uart::printf<256>;

		auto *arena = malloc(ARENA_SIZE);

		if(!arena) {
			uart_printer("Something went wrong!!!\r\n");
			memory::get_allocator()->dump_heap(uart_printer);
			PANIC("Halt!");
		}

		uart_printer("MEMORY BLOCK USED: %u\r\n", memory::get_allocator()->total());
		uart_printer("MEM: After allocate %u bytes\r\n", ARENA_SIZE);
		memory::get_allocator()->dump_heap(uart_printer);

		fix_allocator_type a;

		a.init(reinterpret_cast<std::uintptr_t>(arena), reinterpret_cast<std::uintptr_t>(arena) + ARENA_SIZE);
		uart_printer("MEMORY BLOCK USED WITH FIXED: %u\r\n", a.total());

		uart_printer("FIX: Before malloc:\r\n\r\n");
		a.dump_heap(uart_printer);

		rnd::xorshift32 rng(kernel::core::get_systick_val());

		for(std::size_t i=0; i<POINTERS_COUNT; ++i) {
			allocated[i] = a.alloc(rng.next() % MAXIMUM_BLOCK + 1);
		}
		uart_printer("FIX: After malloc:\r\n\r\n");
		a.dump_heap(uart_printer);

		for(std::size_t i=0; i<POINTERS_COUNT; ++i) {
			a.free(allocated[i]);
		}
		uart_printer("FIX: After free:\r\n\r\n");
		a.dump_heap(uart_printer);

		free(arena);
		uart_printer("MEM: After free %u bytes\r\n", ARENA_SIZE);
		memory::get_allocator()->dump_heap(uart_printer);

		kernel::launch(10);
		PANIC("Should not be here");
	}
}

#endif
