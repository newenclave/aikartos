/*
 * sch_mlfq.cpp
 *
 *  Created on: May 17, 2025
 *      Author: newenclave
 *  
 */

#include <cstring>
#include <ranges>

#include "aikartos/device/uart.hpp"
#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_mlfq.hpp"
#include "aikartos/sch/statistic.hpp"

#include "tests.hpp"

#ifdef ENABLE_TEST_sch_mlfq

using namespace aikartos;

std::uint32_t last_delta[tests::COUNT_SIZE];

namespace {

	using config = kernel::config;
	namespace sch_ns = sch::mlfq;
	using stat_fields = sch_ns::statistics_fields;

	sch::statistic<config::maximum_tasks> stat;
	struct sch_task_info {
		void *task_entry = nullptr;
		const char *task_param = nullptr;
		std::uint32_t level = 0;
		tasks::descriptor::state_type state = {};
	};

	struct task_info_less {
		bool operator ()(const sch_task_info &lhs, const sch_task_info &rhs) const {
			if(lhs.task_param == nullptr) return true;
			if(rhs.task_param == nullptr) return false;
			return std::strcmp(lhs.task_param, rhs.task_param) < 0;
		}
	};

	std::array<sch_task_info, config::maximum_tasks> sch_tasks_data;

	void update_task_data(std::size_t id) {
		sch_tasks_data[id].task_entry = reinterpret_cast<void *>(stat.get_field(id, static_cast<std::size_t>(stat_fields::task_entry)));
		sch_tasks_data[id].task_param = reinterpret_cast<const char *>(stat.get_field(id, static_cast<std::size_t>(stat_fields::task_param)));
		sch_tasks_data[id].level = static_cast<std::uint32_t>(stat.get_field(id, static_cast<std::size_t>(stat_fields::level)));
		sch_tasks_data[id].state = static_cast<tasks::descriptor::state_type>(stat.get_field(id, static_cast<std::size_t>(stat_fields::state)));
	}

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
	    	if(kernel::core::get_scheduler_statisctic(stat)) {
	    		for(std::size_t id = 0; id<stat.size(); ++id) {
	    			update_task_data(id);
	    		}
    			std::ranges::sort(sch_tasks_data, task_info_less{});
	    	}

	    	device::uart::printf<265>("'%s' - %u, '%s' - %u, '%s' - %u, '%s' - %u, '%s' - %u\r\n",
	    			sch_tasks_data[0].task_param, sch_tasks_data[0].level,
	    			sch_tasks_data[1].task_param, sch_tasks_data[1].level,
	    			sch_tasks_data[2].task_param, sch_tasks_data[2].level,
	    			sch_tasks_data[3].task_param, sch_tasks_data[3].level,
	    			sch_tasks_data[4].task_param, sch_tasks_data[4].level
	    	);
	        device::uart::printf<128>(
	            "Y:%lu (%lu) A:%lu (%lu) S:%lu (%lu) X:%lu (%lu)\r\n",
	            count[0], count[0] - last_delta[0],
				count[1], count[1] - last_delta[1],
				count[2], count[2] - last_delta[2],
				count[3], count[3] - last_delta[3]
	        );
	        last_delta[0] = count[0];
	        last_delta[1] = count[1];
	        last_delta[2] = count[2];
	        last_delta[3] = count[3];
	        kernel::sleep(500);
	    }
	}

}

namespace tests {

	int test::run() {
		device::uart::init_tx();
		kernel::init<sch_ns::scheduler, config>();
		using config_flags = sch_ns::config_flags;

		sch_ns::quantum_levels ql1 = { .high = 3, .middle = 6, .low = 9 };

		kernel::add_task(&task_yielder, (void *)("0 task_yielder"));
		kernel::add_task(&task_angry, tasks::config{}
			.set<config_flags::levels>(reinterpret_cast<std::uintptr_t>(&ql1))
			.set<config_flags::boost_quanta>(2000),
			(void *)("1 task_angry"));
		kernel::add_task(&task_sleeper, (void *)("2 task_sleeper"));
		kernel::add_task(&task_spammy, (void *)("3 task_spammy"));
		kernel::add_task(&task_monitor, (void *)("4 task_monitor"));

		kernel::launch(10);
		PANIC("Should not be here");
	}
}


#endif
