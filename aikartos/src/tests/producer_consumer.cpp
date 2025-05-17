/*
 * producer_consumer.cpp
 *
 *  Created on: May 15, 2025
 *      Author: newenclave
 *  
 */

#include <stdio.h>
#include <mutex>

#include "aikartos/device/uart.hpp"
#include "aikartos/kernel/config.hpp"
#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_round_robin.hpp"
#include "aikartos/sync/circular_queue.hpp"
#include "aikartos/sync/spin_lock.hpp"
#include "aikartos/sync/spin_conditional_variable.hpp"

#include "tests.hpp"

#ifdef ENABLE_TEST_producer_consumer

using namespace aikartos;

namespace {

	struct kernel_yield {
		static void yield() noexcept { kernel::yield(); };
	};
	using spin_lock_type = sync::spin_lock<kernel_yield>;

	struct queue_data {
		const char *message;
		std::uint32_t len;
		std::atomic_flag lock = ATOMIC_FLAG_INIT;
	};

	using queue_data_ptr = queue_data *;
	using queue_type = sync::circular_queue<queue_data_ptr, 8, spin_lock_type>;

	queue_type g_queue;

	void consumer(void *) {
		while(1) {
			if(auto next = g_queue.try_pop()) {
				auto *data = *next;
				device::uart::blocking_write(data->message, data->len);
				data->lock.clear(std::memory_order_release);
				count[0]++;
			}
			else {
				kernel::yield();
			}
		}
	}

	void producer1(void *) {
		char buf[100];
		queue_data data;
		data.message = &buf[0];
		while(1) {
			auto len = snprintf(buf, 100, "Producer 1, count[1] = %lu\r\n", count[1]++);
			data.len = static_cast<std::uint32_t>(len);
			while(data.lock.test_and_set(std::memory_order_acquire)) {
				kernel::yield();
			}
			while(!g_queue.try_push(&data)) {
				kernel::yield();
			}
			kernel::sleep(100);
		}
	}

	void producer2(void *) {
		char buf[100];
		queue_data data;
		data.message = &buf[0];
		while(1) {
			auto len = snprintf(buf, 100, "Producer 2, count[2] = %lu\r\n", count[2]++);
			data.len = static_cast<std::uint32_t>(len);
			while(data.lock.test_and_set(std::memory_order_acquire)) {
				kernel::yield();
			}
			while(!g_queue.try_push(&data)) {
				kernel::yield();
			}
			kernel::sleep(100);
		}
	}

	void producer3(void *) {
		char buf[100];
		queue_data data;
		data.message = &buf[0];
		while(1) {
			auto len = snprintf(buf, 100, "Producer 3, count[3] = %lu\r\n", count[3]++);
			data.len = static_cast<std::uint32_t>(len);
			while(data.lock.test_and_set(std::memory_order_acquire)) {
				kernel::yield();
			}
			while(!g_queue.try_push(&data)) {
				kernel::yield();
			}
			kernel::sleep(100);
		}
	}

}


namespace tests {

	int test::run(void)
	{
		using config = kernel::config;
		namespace sch_ns = sch::round_robin;
		kernel::init<sch_ns::scheduler, config>();

		device::uart::init_tx();

		kernel::add_task(&consumer);
		kernel::add_task(&producer1);
		kernel::add_task(&producer2);
		kernel::add_task(&producer3);

		kernel::launch(10);
		PANIC("Should not be here");
	}
}

#endif
