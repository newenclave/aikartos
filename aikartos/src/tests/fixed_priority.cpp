/*
 * fixed_priority.cpp
 *
 *  Created on: May 14, 2025
 *      Author: newenclave
 */

#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_fixed_priority.hpp"

#include "tests.hpp"

#ifdef ENABLE_TEST_fixed_priority

using namespace aikartos;

namespace {

	constexpr static std::uint32_t THRESHOLD = 100'000'000;

	void task0(void *)
	{
		 while(count[0]++ < THRESHOLD) {
		 }
	}

	void task1(void *)
	{
		 while(count[1]++ < THRESHOLD) {
		 }
	}

	void task2(void *)
	{
		 while(count[2]++ < THRESHOLD){
		 }
	}
}

namespace tests {

	int test::run(void)
	{

		using config = kernel::config;
		namespace sch_ns = sch::fixed_priority;
		kernel::init<sch_ns::scheduler, config>();
		using config_flags = sch_ns::config_flags;

		config_flags cf;
		(void)cf;

		kernel::add_task(&task0, tasks::config{}.set<config_flags::priority>(2));
		kernel::add_task(&task1, tasks::config{}.set<config_flags::priority>(1));
		kernel::add_task(&task2, tasks::config{}.set<config_flags::priority>(0));

	//	kernel::add_task(&task2, tasks::config{.priority = tasks::config::priority_class::LOW, .aging_threshold = 10});
		kernel::launch(10);
		PANIC("Should not be here");
	}

}

#endif
