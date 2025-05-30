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
#include "aikartos/rnd/xorshift128.hpp"
#include "aikartos/sync/circular_queue.hpp"

#include "terminal.hpp"
#include "tests.hpp"

using namespace aikartos;

#ifdef ENABLE_TEST_FPU_demo_03

namespace {

	const auto printer = device::uart::printf<32>;
	tests::terminal<decltype(printer)> term(printer);

	constexpr int WIDTH = 20;
	constexpr int HEIGHT = 20;

	float fire[HEIGHT][WIDTH];
	rnd::xorshift128 rng;

	void seed_fire() {
		for (int x = 0; x < WIDTH; ++x) {
			if (rng.next() % 100 < 70) {
				fire[HEIGHT - 1][x] = 1.0f;
			} else {
				fire[HEIGHT - 1][x] = 0.0f;
			}
		}
	}

	void update_fire() {
		for (int y = 1; y < HEIGHT; ++y) {
			for (int x = 0; x < WIDTH; ++x) {
				float sum = fire[y][x];
				if (x > 0) {
					sum += fire[y][x - 1];
				}
				if (x < WIDTH - 1) {
					sum += fire[y][x + 1];
				}
				if (y < HEIGHT - 1) {
					sum += fire[y + 1][x];
				}
				float avg = sum / 4.0f;
				avg -= 0.015f + ((float) (rng.next()) / RAND_MAX) * 0.015f;
				if (avg < 0.0f) {
					avg = 0.0f;
				}
				fire[y - 1][x] = avg;
			}
		}
	}

	int get_color(float t) {
		if (t < 0.2f) {
			return 30;
		} else if (t < 0.4f) {
			return 31;
		} else if (t < 0.6f) {
			return 33;
		} else if (t < 0.8f) {
			return 37;
		} else {
			return 97;
		}
	}

	const char* get_char(float t) {
		if (t < 0.2f) {
			return ".";
		} else if (t < 0.4f) {
			return "o";
		} else if (t < 0.6f) {
			return "O";
		} else if (t < 0.8f) {
			return "O";
		} else {
			return "^";
		}
	}

	void render_fire() {
		term.cursor_to_top();
		for (int y = 0; y < HEIGHT; ++y) {
			for (int x = 0; x < WIDTH; ++x) {
				int color = get_color(fire[y][x]);
				const char *sym = get_char(fire[y][x]);
				term.set_color(color);
				printer("%s", sym);
			}
			printer("\r\n");
		}
	}

	void draw_fire(void*) {
		this_task::enable_fpu();
		term.reset_color();

		term.clear();
		while (1) {
			seed_fire();
			update_fire();
			render_fire();
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

		kernel::add_task(&draw_fire);

		kernel::launch(constants::quanta_infinite);
		PANIC("Should not be here");
	}
}

#endif
