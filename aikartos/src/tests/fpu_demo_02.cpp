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
#include "aikartos/sch/scheduler_coop_preemptive.hpp"
#include "aikartos/device/uart.hpp"
#include "aikartos/rnd/xorshift32.hpp"
#include "aikartos/sync/circular_queue.hpp"

#include "terminal.hpp"
#include "tests.hpp"

using namespace aikartos;

#ifdef ENABLE_TEST_FPU_demo_02

namespace {

	const auto printer = device::uart::printf<32>;
	tests::console<decltype(printer)> term = { .printer = printer };

	constexpr int WIDTH = 80;
	constexpr int HEIGHT = 25;
	constexpr int RADIUS = 10;

	void draw_point(int x, int y, char c) {
		if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
			term.set_position(x, y);
			printer("%c", c);
		}
	}

	void draw_hand(float angle_deg, int length, char symbol, int cx, int cy) {
		float angle = angle_deg * 3.14159f / 180.0f;

		for (int i = 1; i <= length; ++i) {
			int x = static_cast<int>(cx + std::cos(angle) * i);
			int y = static_cast<int>(cy + std::sin(angle) * i);
			draw_point(x, y, symbol);
		}
	}

	void draw_clock(void*) {
		this_task::enable_fpu();
		term.set_color(97);

		const int cx = WIDTH / 2;
		const int cy = HEIGHT / 2;

		term.clear();
		std::uint32_t current_sec = 1748511220ul;

		float old_sec_angle = 0.0f;
		float old_min_angle = 0.0f;
		float old_hour_angle = 0.0f;

		term.clear();
		while (true) {

			for (int i = 0; i < 12; ++i) {
				float angle = i * 30.0f * 3.14159f / 180.0f;
				int x = static_cast<int>(cx + std::cos(angle) * RADIUS);
				int y = static_cast<int>(cy + std::sin(angle) * RADIUS);
				draw_point(x, y, 'o');
			}

			// get current time
			std::time_t now_sec = current_sec++;

			std::uint32_t sec_value = (now_sec % 60);
			std::uint32_t min_value = (now_sec / 60) % 60;
			std::uint32_t hour_value = (((now_sec / 60) % 60) / 24) % 12;

			float sec_angle = sec_value * 6.0f;
			float min_angle = min_value * 6.0f + sec_value * 0.1f;
			float hour_angle = hour_value * 30.0f + min_value * 0.5f;

			if(old_hour_angle != hour_angle) {
				draw_hand(old_hour_angle - 90.0f, 5, ' ', cx, cy);
			}
			if(old_min_angle != min_angle) {
				draw_hand(old_min_angle - 90.0f, 7, ' ', cx, cy);
			}
			if(old_sec_angle != sec_angle) {
				draw_hand(old_sec_angle - 90.0f, 9, ' ', cx, cy);
			}

			draw_hand(hour_angle - 90.0f, 5, '#', cx, cy);
			draw_hand(min_angle - 90.0f, 7, '+', cx, cy);
			draw_hand(sec_angle - 90.0f, 9, '.', cx, cy);

			old_hour_angle = hour_angle;
			old_min_angle = min_angle;
			old_sec_angle = sec_angle;

			draw_point(cx, cy, '@');

			kernel::sleep(100);
		}
	}

}

namespace tests {

	int test::run(void)
	{

		using config = kernel::config;
		namespace sch_ns = sch::coop_preemptive;
		kernel::init<sch_ns::scheduler, config>();
		device::uart::init_tx();

		[[maybe_unused]] bool fpu_enabled = kernel::enable_fpu_hardware();
		kernel::set_task_fpu_default(false);

		kernel::add_task(&draw_clock);

		kernel::launch(constants::quanta_infinite);
		PANIC("Should not be here");
	}
}

#endif
