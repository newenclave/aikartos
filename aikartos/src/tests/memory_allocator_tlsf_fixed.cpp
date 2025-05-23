/**
 * @file tests/memory_allocator_tlsf_fixed.cpp
 * @brief TLSF allocator test using fixed-size internal metadata.
 *
 * - Initializes a `tlsf::impl::fixed` allocator with statically defined heap size and index table.
 * - All allocator metadata is stored internally in a compile-time sized `std::array`.
 * - Performs randomized allocations and deallocations with high fragmentation potential.
 * - Verifies correct handling of alignment, block splitting, and merging.
 * - Demonstrates full recovery of heap space after complete deallocation.
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

#ifdef ENABLE_TEST_memory_allocator_tlsf_fixed

using namespace aikartos;

namespace {
}

namespace tests {

	using allocator_type = memory::allocator_tlsf_fixed<256*1024>;
	//using allocator_type = memory::allocator_tlsf_fixed<64*1024>;
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

		for(std::size_t i=0; i<POINTERS_COUNT; ++i) {
			auto next = rng.next() % MAXIMUM_BLOCK + 1;
			allocated[i] = malloc(next);
		}
		uart_printer("After malloc:\r\n\r\n");
		memory::get_allocator()->dump_heap(uart_printer);

		for(std::size_t i=0; i<POINTERS_COUNT; ++i) {
			auto next = rng.next() % POINTERS_COUNT;
			free(allocated[next]);
			allocated[next] = nullptr;
		}
		uart_printer("After random free:\r\n\r\n");
		memory::get_allocator()->dump_heap(uart_printer);

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
