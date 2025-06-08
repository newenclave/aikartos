/**
 *  modules example
 *  Created on: May 23, 2025
 *      Author: newenclave
 */

#include <memory.h>

#include "aikartos/kernel/config.hpp"
#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_round_robin.hpp"

#include "aikartos/memory/memory.hpp"
#include "aikartos/memory/allocator_tlsf.hpp"
#include "aikartos/device/uart.hpp"

#include "aikartos/rnd/xorshift32.hpp"
#include "aikartos/modules/module.hpp"
#include "aikartos/modules/bundle.hpp"

#include "tests.hpp"

#ifdef ENABLE_TEST_modules_001

using namespace aikartos;

namespace {
	using printer_type = void (*)(const char*, ...);
	using sleep_type = void (*)(std::uint32_t);
}

extern "C" std::uint8_t _modules_begin;

namespace tests {

	using allocator_type = memory::allocator_tlsf<>;
	using config = kernel::config;
	namespace sch_ns = sch::round_robin;

	constexpr std::size_t POINTERS_COUNT = 8 * 1024;
	constexpr std::size_t MAXIMUM_BLOCK = 1000;
	void *allocated[POINTERS_COUNT];

	struct module_param {
		const char *name;
		printer_type printer;
		sleep_type sleep;
	};

	using module_call = int(*)(module_param *);

	int test::run(void)
	{
		kernel::init<sch_ns::scheduler, config>();
		memory::init<allocator_type>();
		device::uart::init_tx();

		auto uart_printer = device::uart::printf<256>;

		std::uintptr_t bin_data = reinterpret_cast<std::uintptr_t>(&_modules_begin);

		[[maybe_unused]] const bool is_module = modules::module::is_module_address(bin_data);
		[[maybe_unused]] const bool is_bundle = modules::bundle::is_bundle_address(bin_data);

		module_param m1 = {
			.name = "Module 1",
			.printer = uart_printer,
			.sleep = &kernel::sleep,
		};

		module_param m2 = {
			.name = "Module 2",
			.printer = uart_printer,
			.sleep = &kernel::sleep,
		};


		if(is_module) {
			modules::module test_m(bin_data);
			[[maybe_unused]] const bool crc_ok = test_m.check_crc32();
			[[maybe_unused]] auto desc = test_m.get_description();

			void* exec_memory = (void*)malloc(test_m.get_image_size());

			test_m.load(reinterpret_cast<std::uintptr_t>(exec_memory));

			kernel::add_task(test_m.get_entry_point<tasks::control_block::task_entry>(),
					(void *)&m1);

			kernel::add_task(test_m.get_entry_point<tasks::control_block::task_entry>(),
					(void *)&m2);

		}
		else if(is_bundle) {
			modules::bundle bdl { bin_data };
			auto mod1 = bdl.get_module(0);
			auto mod2 = bdl.get_module(1);

			auto m1_entry = reinterpret_cast<std::uintptr_t>(malloc(mod1->get_image_size()));
			auto m2_entry = reinterpret_cast<std::uintptr_t>(malloc(mod2->get_image_size()));

			mod1->load(m1_entry);
			mod2->load(m2_entry);

			kernel::add_task(mod1->get_entry_point<tasks::control_block::task_entry>(), (void *)&m1);

			kernel::add_task(mod2->get_entry_point<tasks::control_block::task_entry>(), (void *)&m2);

		}



		kernel::launch(10);
		PANIC("Should not be here");
	}
}

#endif
