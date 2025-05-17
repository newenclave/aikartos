/*
 * priority_aging.cpp
 *
 *  Created on: May 14, 2025
 *      Author: newenclave
 */


#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_priority_aging.hpp"

#include "tests.hpp"

#ifdef ENABLE_TEST_priority_aging

using namespace aikartos;

namespace {

	void task0(void *)
	{
		 while(1) {
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
		 while(1) {
			 count[2]++;
		 }
	}
}

namespace tests {

	int test::run(void)
	{

		using config = kernel::config;
		namespace sch_ns = sch::priority_aging;
		kernel::init<sch_ns::scheduler, config>();
		using config_flags = sch_ns::config_flags;

		config_flags cf;
		(void)cf;

		kernel::add_task(&task0, tasks::config{}.set<config_flags::priority>(2).set<config_flags::aging_threshold>(4));
		kernel::add_task(&task1, tasks::config{}.set<config_flags::priority>(1));
		kernel::add_task(&task2, tasks::config{}.set<config_flags::priority>(0).set<config_flags::aging_threshold>(2));

		kernel::launch(10);
		PANIC("Should not be here");
	}

}

#endif
