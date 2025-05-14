/*
 * stack_overflow.cpp
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

	int recursive(int i) {
		char boom[512];
		(void)boom;
		if(i > 0) {
			recursive(i - 1);
		}
		return 0;
	}

	void task0(void*) {
		recursive(10);
	}
}

namespace tests {

	int stack_overflow(void)
	{
		using config = kernel::config;
		namespace sch_ns = sch::round_robin;
		kernel::init<sch_ns::scheduler, config>();

		kernel::add_task(&task0);

		kernel::launch(10);
		PANIC("Should not be here");
	}
}
