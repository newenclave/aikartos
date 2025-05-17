/*
 * sch_cfs_like.cpp
 *
 *  Created on: May 17, 2025
 *      Author: newenclave
 *  
 */

#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_cfs_like.hpp"

#include "tests.hpp"

#ifdef ENABLE_TEST_sch_cfs_like

using namespace aikartos;

namespace {
	void task0(void *)
	{
		 while(1){
			 if(count[0]++ % 10000 == 0) {
				 kernel::yield();
			 }
		 }
	}

	void task1(void *)
	{
		 while(1) {
			 if(count[1]++ % 100000 == 0) {
				 kernel::sleep(500);
			 }
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

	int test::run() {
		using config = kernel::config;
		namespace sch_ns = sch::cfs_like;
		kernel::init<sch_ns::scheduler, config>();

		kernel::add_task(&task0);
		kernel::add_task(&task1);
		kernel::add_task(&task2);

		kernel::launch(10);
		PANIC("Should not be here");
	}
}


#endif
