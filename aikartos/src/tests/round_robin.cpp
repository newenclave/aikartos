/*
 * test_round_robin.cpp
 *
 *  Created on: May 14, 2025
 *      Author: newenclave
 */

#include "aikartos/kernel/config.hpp"
#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_round_robin.hpp"

#include "tests.hpp"

using namespace aikartos;

namespace {
	void task0(void *)
	{
		 while(1){
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

	int round_robin(void)
	{
		using config = kernel::config;
		namespace sch_ns = sch::round_robin;
		kernel::init<sch_ns::scheduler, config>();

		kernel::add_task(&task0);
		kernel::add_task(&task1);
		kernel::add_task(&task2);

		kernel::launch(10);
		PANIC("Should not be here");
	}
}

