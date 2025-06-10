/*
 * tests.hpp
 *
 *  Created on: May 14, 2025
 *      Author: newenclave
 */

#pragma once

#include <cstdint>

//#define ENABLE_TEST_round_robin
//#define ENABLE_TEST_edf
//#define ENABLE_TEST_fixed_priority
//#define ENABLE_TEST_weighted_lottery
//#define ENABLE_TEST_coop_preemptive
//#define ENABLE_TEST_lottery
//#define ENABLE_TEST_priority_aging
//#define ENABLE_TEST_stack_overflow

//#define ENABLE_TEST_uart_blocking_write
//#define ENABLE_TEST_producer_consumer

//#define ENABLE_TEST_sch_cfs_like
//#define ENABLE_TEST_sch_mlfq

//#define ENABLE_TEST_memory_allocation_free_list
//#define ENABLE_TEST_memory_allocation_bump_list
//#define ENABLE_TEST_memory_allocation_dlist

//#define ENABLE_TEST_memory_allocator_buddy
//#define ENABLE_TEST_memory_allocator_buddy_fixed

//#define ENABLE_TEST_memory_allocator_tlsf
#define ENABLE_TEST_memory_allocator_tlsf_fixed

//#define ENABLE_TEST_device_aurt_echo
//#define ENABLE_TEST_the_snake_

//#define ENABLE_TEST_FPU_demo_01
//#define ENABLE_TEST_FPU_demo_02
//#define ENABLE_TEST_FPU_demo_03

#define ENABLE_TEST_modules_001

#include <atomic>

namespace tests {

	constexpr std::uint32_t COUNT_SIZE = 5;

	struct test {
		static int run();
	};

}

extern std::uint32_t count[tests::COUNT_SIZE];
//extern std::atomic<std::uint32_t> count[tests::COUNT_SIZE];
