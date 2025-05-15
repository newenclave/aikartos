/*
 * coop_preemptive.cpp
 *
 *  Created on: May 15, 2025
 *      Author: newenclave
 *  
 */


#include "aikartos/kernel/config.hpp"
#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_coop_preemptive.hpp"

#include "tests.hpp"

using namespace aikartos;

namespace {

	void task_quanta_1(void *)
	{
		 while(1){
			 count[4]++;
		 }
	}

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

	// This task has a quanta of 0, meaning it will run indefinitely without yielding or sleeping.
	void task3(void *) {
		while(1){
			for(int i=0; i<3'000'000; ++i) {
				count[3]++;
			}
			kernel::sleep(1000);
		}
	}
}

namespace tests {

	int coop_preemptive(void)
	{
		using config = kernel::config;
		namespace sch_ns = sch::coop_preemptive;
		using flags = sch_ns::config_flags;
		kernel::init<sch_ns::scheduler, config>();

		kernel::add_task(&task0, tasks::config{}.set<flags::quanta>(10));
		kernel::add_task(&task1, tasks::config{}.set<flags::quanta>(20));
		kernel::add_task(&task2, tasks::config{}.set<flags::quanta>(40));
		kernel::add_task(&task3, tasks::config{}.set<flags::quanta>(constants::quanta_infinite)); // 0 == INFINITE
		kernel::add_task(&task_quanta_1, tasks::config{}.set<flags::quanta>(0));

		kernel::launch(10);
		PANIC("Should not be here");
	}
}
