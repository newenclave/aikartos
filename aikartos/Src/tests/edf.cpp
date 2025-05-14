/*
 * edf.cpp
 *
 *  Created on: May 14, 2025
 *      Author: newenclave
 */


#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_edf.hpp"

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

	std::uint32_t killed_tasks = 0;

	int edf(void)
	{
		using config = kernel::config;
		namespace sch_ns = sch::edf;
		kernel::init<sch_ns::scheduler, config>();
		using config_flags = sch_ns::config_flags;

		kernel::add_task(&task0, tasks::config{}.set<config_flags::relative_deadline>(1000));
		kernel::add_task(&task1);
		kernel::add_task(&task2, tasks::config{}.set<config_flags::relative_deadline>(10000));

		kernel::set_scheduler_event_handler([](std::uint32_t event) {
			kernel::terminate_current();
			killed_tasks++;
			kernel::add_task(&task0, tasks::config{}.set<config_flags::relative_deadline>(1000));
			return sch::decision::RETRY;
		});

		kernel::launch(10);
		PANIC("Should not be here");
	}
}
