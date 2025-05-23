/*
 * device_uart_echo.cpp
 *
 *  Created on: May 23, 2025
 *      Author: newenclave
 *  
 */

#include "aikartos/kernel/config.hpp"
#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_round_robin.hpp"
#include "aikartos/device/uart.hpp"

#include "tests.hpp"

using namespace aikartos;

#ifdef ENABLE_TEST_device_aurt_echo

namespace {

	struct kernel_yielder {
		static void yield() noexcept {
			kernel::yield();
		}
	};

	void reader_task(void *)
	{
		 while(1){
			 auto b = device::uart::blocking_read<kernel_yielder>();
			 if(b == '\n' || b == '\r') {
				 device::uart::printf("\r\n");
			 }
			 else {
				 device::uart::printf("%c", b);
			 }
			 count[0]++;
		 }
	}

	void task1(void *)
	{
		 while(1) {
			 count[1]++;
		 }
	}

	void task2(void *)
	{
		 while(1){
			 count[2]++;
		 }
	}
}

namespace tests {

	int test::run(void)
	{
		using config = kernel::config;
		namespace sch_ns = sch::round_robin;
		kernel::init<sch_ns::scheduler, config>();
		device::uart::init_rxtx(true);

		kernel::add_task(&reader_task);
		kernel::add_task(&task1);
		kernel::add_task(&task2);

		kernel::launch(10);
		PANIC("Should not be here");
	}
}

#endif
