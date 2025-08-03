/*
 * syscalls.cpp
 *
 *  Created on: Aug 3, 2025
 *      Author: newenclave
 *  
 */


#include "aikartos/kernel/config.hpp"
#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_round_robin.hpp"
#include "aikartos/syscalls/syscalls.h"

#include "tests.hpp"

using namespace aikartos;

#ifdef ENABLE_TEST_syscalls

namespace {
	void task0(void *)
	{
		 while(1){
			 count[0] += 1;
		 }
	}

	void task1(void *)
	{
		 while(1){
			 count[1] += 1;
			 syscall_yield();
		 }
	}

	void task2(void *)
	{
		 while(1){
			 count[2] += 1;
			 syscall_sleep(100);
		 }
	}

}

namespace tests {

	int test::run(void)
	{

		using config = kernel::config;
		namespace sch_ns = sch::round_robin;
		kernel::init<sch_ns::scheduler, config>();

		syscall_add_task(task0);
		syscall_add_task(task1);
		syscall_add_task(task2);

		kernel::launch(10);
		PANIC("Should not be here");
	}
}

#endif
