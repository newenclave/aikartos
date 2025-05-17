/*
 * uart_write.cpp
 *
 *  Created on: May 15, 2025
 *      Author: newenclave
 *  
 */

#include <stdio.h>
#include <mutex>

#include "aikartos/device/uart.hpp"
#include "aikartos/kernel/config.hpp"
#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_round_robin.hpp"
#include "aikartos/sync/spin_lock.hpp"

#include "tests.hpp"

#ifdef ENABLE_TEST_uart_blocking_write

using namespace aikartos;

namespace {

	struct kernel_yield {
		static void yield() noexcept { kernel::yield(); };
	};

	sync::spin_lock<kernel_yield> uart_lock;

	void task0(void*) {
		char buf[100];
		while (1) {
			{
				std::lock_guard<decltype(uart_lock)> l(uart_lock);
				count[0]++;
				int len = snprintf(buf, 100, "Hello from task0 %lu\r\n", count[0]);
				device::uart::blocking_write(buf, len);
			}
			// it should sleep or yield outside the scope of the "lock_guard"
			kernel::sleep(1000);
		}
	}

	void task1(void*) {
		char buf[100];
		while (1) {
			{
				std::lock_guard<decltype(uart_lock)> l(uart_lock);
				count[1]++;
				int len = snprintf(buf, 100, "Hello from task1 %lu\r\n", count[1]);
				device::uart::blocking_write(buf, len);
			}
			// it should sleep or yield outside the scope of the "lock_guard"
			kernel::sleep(1000);
		}
	}

	void task2(void*) {
		char buf[100];
		while (1) {
			{
				std::lock_guard<decltype(uart_lock)> l(uart_lock);
				count[2]++;
				int len = snprintf(buf, 100, "Hello from task2 %lu\r\n", count[2]);
				device::uart::blocking_write(buf, len);
			}
			// it should sleep or yield outside the scope of the "lock_guard"
			kernel::sleep(1000);
		}
	}
}

namespace tests {

	int test::run(void)
	{
		using config = kernel::config;
		namespace sch_ns = sch::round_robin;
		kernel::init<sch_ns::scheduler, config>();

		device::uart::init_tx();

		kernel::add_task(&task0);
		kernel::add_task(&task1);
		kernel::add_task(&task2);

		kernel::launch(10);
		PANIC("Should not be here");
	}
}

#endif
