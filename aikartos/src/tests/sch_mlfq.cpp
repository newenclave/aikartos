/*
 * sch_mlfq.cpp
 *
 *  Created on: May 17, 2025
 *      Author: newenclave
 *  
 */



#include "aikartos/device/uart.hpp"
#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_mlfq.hpp"

#include "tests.hpp"

#ifdef ENABLE_TEST_sch_mlfq

using namespace aikartos;

std::uint32_t last_delta[tests::COUNT_SIZE];

namespace {

	void task_yielder(void *) {
		while (1) {
			count[0]++;
			kernel::yield();
		}
	}

	void task_angry(void *) {
	    while (1) {
	        count[1]++;
	    }
	}

	void task_sleeper(void *) {
	    while (1) {
	        count[2]++;
	        kernel::sleep(200);
	    }
	}

	void task_spammy(void *) {
	    while (1) {
	        count[3]++;
	        if (count[3] % 10000 == 0)
	            kernel::yield();
	    }
	}

	void task_monitor(void *) {
	    while (1) {
	        device::uart::printf<128>(
	            "Y:%lu (%lu) M:%lu (%lu) S:%lu (%lu) X:%lu (%lu)\r\n",
	            count[0], count[0] - last_delta[0],
				count[1], count[1] - last_delta[1],
				count[2], count[2] - last_delta[2],
				count[3], count[3] - last_delta[3]
	        );
	        last_delta[0] = count[0];
	        last_delta[1] = count[1];
	        last_delta[2] = count[2];
	        last_delta[3] = count[3];
	        kernel::sleep(1000);
	    }
	}

}

namespace tests {

	int test::run() {
		device::uart::init_tx();
		using config = kernel::config;
		namespace sch_ns = sch::mlfq;
		kernel::init<sch_ns::scheduler, config>();
		using config_flags = sch_ns::config_flags;

		sch_ns::quantum_levels ql1 = { .high = 3, .middle = 6, .low = 9 };

		kernel::add_task(&task_yielder);
		kernel::add_task(&task_angry, tasks::config{}
			.set<config_flags::levels>(reinterpret_cast<std::uintptr_t>(&ql1))
			.set<config_flags::levels>(2000));
		kernel::add_task(&task_sleeper);
		kernel::add_task(&task_spammy);
		kernel::add_task(&task_monitor);

		kernel::launch(10);
		PANIC("Should not be here");
	}
}


#endif
