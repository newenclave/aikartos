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

#ifdef ENABLE_TEST_round_robin

namespace {
	void task0(void *)
	{
		 while(1){
			 count[0] += 1;
		 }
	}

	void task1(void *)
	{
		 while(1) {
			 count[1] += 1;
		 }
	}

	void task2(void *)
	{
		 while(1){
			 count[2] += 1;
		 }
	}

	void task3(void *)
	{
		 while(1){
			 count[3] += 1;
		 }
	}

	void task4(void *)
	{
		 while(1){
			 count[4] += 1;
		 }
	}
}

namespace tests {

	int test::run(void)
	{
		SCB_DisableICache();
		//SCB_DisableDCache();

		using config = kernel::config;
		namespace sch_ns = sch::round_robin;
		kernel::init<sch_ns::scheduler, config>();

		kernel::add_task(&task0);
		kernel::add_task(&task1);
		kernel::add_task(&task2);
		kernel::add_task(&task3);
		kernel::add_task(&task4);

		kernel::launch(10);
		PANIC("Should not be here");
	}
}

#endif
