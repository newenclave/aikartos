/*
 * fpu_demo_01.cpp
 *
 *  Created on: May 28, 2025
 *      Author: newenclave
 *  
 */

#include <cmath>

#include "aikartos/device/device.hpp"
#include "aikartos/kernel/config.hpp"
#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/this_task/this_task.hpp"
#include "aikartos/sch/scheduler_round_robin.hpp"
#include "aikartos/device/uart.hpp"
#include "aikartos/rnd/xorshift32.hpp"
#include "aikartos/sync/circular_queue.hpp"

#include "tests.hpp"
#include "console.hpp"

using namespace aikartos;

#ifdef ENABLE_TEST_FPU_demo_01

volatile float sin_value_f = 0.0f;

namespace {

	std::uint32_t get_seed() {
		uint32_t seed = 0;
		seed ^= kernel::core::get_systick_val();
		seed ^= *((volatile uint32_t*) 0x20000000);
		return seed;
	}

	const auto printer = device::uart::printf<32>;

	constexpr float sinus_wide = 30.0f;

	void calc_sinus(void*) {
		this_task::enable_fpu();
		float a = 0.0f;
		while (1) {
			a += 0.2f;
			sin_value_f = std::sin(a);
			if (a >= 10.0f) {
				a = 0.0f;
			}
		}
	}

	float wrap(float value, float limit) {
		value = std::fmod(value, limit);
		if (value < 0)
			value += limit;
		return value;
	}

	struct point {
		int x = 0;
		int y = 0;
	};

	void draw2(void*) {
		this_task::enable_fpu();
		rnd::xorshift32 rng(get_seed());
		tests::console<decltype(printer)> con = { .printer = printer };

		constexpr int WIDTH = 80;
		constexpr int HEIGHT = 25;
		constexpr int TRAIL_LEN = 6;
		constexpr int HEAD_LEN = 3;

		float x = WIDTH / 2.0f;
		float y = HEIGHT / 2.0f;
		float angle = 0.0f;

		const char *HEAD[HEAD_LEN] = { "@", "O", "o" };

		point trail[TRAIL_LEN] = { };

		con.clear();
		while (true) {
			float delta = ((rng.next() % 100) / 100.0f - 0.5f) * 3.9f;
			angle += delta;

			float dx = std::cos(angle);
			float dy = std::sin(angle);

			x = wrap(x + dx, WIDTH);
			y = wrap(y + dy, HEIGHT);

			for (int i = TRAIL_LEN - 1; i > 0; --i) {
				trail[i] = trail[i - 1];
			}
			trail[0] = { static_cast<int>(x), static_cast<int>(y) };

			con.set_position(trail[TRAIL_LEN - 1].x, trail[TRAIL_LEN - 1].y);
			printer(" ");

			for (int i = 0; i < TRAIL_LEN; ++i) {
				const auto &p = trail[i];
				if (p.x < 0 || p.x >= WIDTH || p.y < 0 || p.y >= HEIGHT) {
					continue;
				}
				con.set_position(p.x, p.y);
				con.set_color(90 + (TRAIL_LEN - 1 - i));
				printer("%s", i < HEAD_LEN ? HEAD[i] : ".");
			}

			kernel::sleep(50);
		}
	}

	void task2(void*) {
		while (1) {
			count[2] += 1;
		}
	}

	void task3(void*) {
		while (1) {
			count[3] += 1;
		}
	}

	void task4(void*) {
		while (1) {
			count[4] += 1;
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

		[[maybe_unused]] bool fpu_enabled = kernel::enable_fpu_hardware();
		kernel::set_task_fpu_default(false);

		kernel::add_task(&calc_sinus);

		kernel::add_task(&task2);
		kernel::add_task(&task3);
		kernel::add_task(&task4);

		kernel::add_task(&draw2);

		kernel::launch(10);
		PANIC("Should not be here");
	}
}

#endif
