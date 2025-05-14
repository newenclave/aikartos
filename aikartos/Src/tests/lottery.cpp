/*
 * lottery.hpp
 *
 *  Created on: May 14, 2025
 *      Author: newenclave
 */


#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_lottery.hpp"

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

	int lottery(void)
	{
		using config = kernel::config;
		using config_flags = sch::lottery::config_flags;
		kernel::init<sch::lottery::scheduler, config>();

		kernel::add_task(&task0, tasks::config{}.set<config_flags::tickets>(16));
		kernel::add_task(&task1, tasks::config{}.set<config_flags::tickets>(8));
		kernel::add_task(&task2, tasks::config{}.set<config_flags::tickets>(4));

		kernel::launch(10);
		PANIC("Should not be here");
	}
}
