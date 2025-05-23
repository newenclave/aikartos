/**
 * @file tests/the_snake.cpp
 * @brief Snake game demo for a custom RTOS running on STM32 (UART + ANSI terminal).
 *
 * This is a demonstration of building a simple interactive game on top of a cooperative or preemptive RTOS.
 * It uses UART as input/output and displays everything via ANSI escape codes in a VT100-compatible terminal
 * (e.g., TeraTerm or minicom). The architecture is modular, with each game component running as an independent task.
 *
 * Features:
 * - Real-time input handling via UART (WASD + R to restart).
 * - Efficient partial screen rendering using a change queue.
 * - Snake body stored in a custom circular queue (no dynamic memory allocation).
 * - ANSI color output, fruit generation, scoring, game over state, and auto-restart.
 * - Modular structure: tasks for update, draw, input, and fruit generation.
 *
 * This example demonstrates UART-based user interaction, minimal game logic, and real multitasking behavior
 * on embedded platforms. Useful for learning task separation, screen control, and event handling in RTOS-based systems.
 *
 *  Created on: May 23, 2025
 *      Author: newenclave
 *  
 */


#include "aikartos/kernel/config.hpp"
#include "aikartos/kernel/kernel.hpp"
#include "aikartos/kernel/panic.hpp"
#include "aikartos/sch/scheduler_coop_preemptive.hpp"
#include "aikartos/sync/circular_queue.hpp"
#include "aikartos/device/uart.hpp"
#include "aikartos/rnd/xorshift32.hpp"

#include "tests.hpp"

#ifdef ENABLE_TEST_the_snake_

using namespace aikartos;

namespace {

	constexpr std::size_t FIELD_WIDTH = 80;
	constexpr std::size_t FIELD_HEIGHT = 25;
	constexpr std::size_t STATUS_LINE_X = 25;

	constexpr std::size_t FIELD_X_MIN = 2;
	constexpr std::size_t FIELD_X_MAX = FIELD_WIDTH - 1;
	constexpr std::size_t FIELD_Y_MIN = 2;
	constexpr std::size_t FIELD_Y_MAX = FIELD_HEIGHT - 1;

	constexpr std::size_t SNAKE_FIELD_WIDTH = FIELD_WIDTH - 2;
	constexpr std::size_t SNAKE_FIELD_HEIGHT = FIELD_HEIGHT - 2;

	constexpr std::size_t MAXIMUM_BODY_LEN = 128;

	constexpr char CHAR_HEAD = '@';
	constexpr char CHAR_BODY = 'o';
	constexpr char CHAR_FRUIT = '$';
	constexpr char CHAR_PILL = '*';
	constexpr char CHAR_DEAD = 'x';

	constexpr std::uint32_t COLOR_GREEN = 33;
	constexpr std::uint32_t COLOR_YELLOW = 32;
	constexpr std::uint32_t COLOR_WHITE = 37;
	constexpr std::uint32_t COLOR_BLUE = 34;
	constexpr std::uint32_t COLOR_RED = 91;

	struct position {
		std::size_t x = 0;
		std::size_t y = 0;
		bool operator <=> (const position &) const noexcept = default;
	};

	struct change_info {
		position pos;
		char value = ' ';
		std::uint32_t color_code = COLOR_WHITE;
	};

	enum class direction: std::uint8_t {
		up,
		down,
		left,
		right,
	};

	using body_buffer = sync::circular_queue<position, MAXIMUM_BODY_LEN>;
	using changes_buffer = sync::circular_queue<change_info, MAXIMUM_BODY_LEN>;

	changes_buffer changes;

	class the_snake {
	public:

		the_snake() {
			spawn();
		}

		bool is_opposite(direction new_dir) {
			switch (curr_dir_) {
			case direction::up: return new_dir == direction::down;
			case direction::down: return new_dir == direction::up;
			case direction::left: return new_dir == direction::right;
			case direction::right: return new_dir == direction::left;
			default: return false;
			}
		}

		static bool is_inside(position p) {
			return p.x >= 0 && p.x < SNAKE_FIELD_WIDTH &&
			       p.y >= 0 && p.y < SNAKE_FIELD_HEIGHT;
		}

		void set_direction(direction dir) {
			if(!paused() && !is_opposite(dir)) {
				curr_dir_ = dir;
			}
		}

		auto get_direction() const {
			return curr_dir_;
		}

		std::size_t size() const {
			return body_.size();
		}

		position get_body_element(std::size_t id) const {
			if(auto pos = body_.try_get(id) ) {
				return *pos;
			}
			return {};
		}

		std::size_t get_delay() const {
			return delay_;
		}

		void set_delay(std::size_t val) {
			delay_ = val;
		}

		void set_fruit(position pos) {
			fruit_ = {pos};
		}

		void set_pill(position pos) {
			pill_ = {pos};
		}

		bool has_fruit() const {
			return fruit_.has_value() || pill_.has_value();
		}

		bool collides_with_self(position pos) const {
			for (std::size_t i = 0; i < size(); ++i) {
				if (get_body_element(i) == pos) {
					return true;
				}
			}
			return false;
		}

		bool paused() const {
			return is_paused_;
		}

		void toggle_pause() {
			is_paused_ = !is_paused_;
		}

		void spawn() {
			auto start_x = rng_.next() % SNAKE_FIELD_WIDTH;
			auto start_y = rng_.next() % SNAKE_FIELD_HEIGHT;
			auto start_dir = rng_.next() % 4;
			body_.try_push({start_x, start_y});
			curr_dir_ = static_cast<direction>(start_dir);
		}

		void reset() {
			rng_.reset_state(kernel::get_tick_count());
			body_.clear();
			spawn();
			score_ = 0;
			is_alive_ = true;
			fruit_ = {};
			pill_ = {};
			delay_ = 300;
		}

		std::uint32_t score() const {
			return score_;
		}

		bool alive() const {
			return is_alive_;
		}

		void update() {
			const auto len = size();
			if(!is_alive_) {
				return;
			}
			if(len > 0){
				auto head = get_body_element(len - 1);
				auto new_head = head;
				switch(curr_dir_) {
				case direction::up: new_head.y--;
					break;
				case direction::down:new_head.y++;
					break;
				case direction::left: new_head.x--;
					break;
				case direction::right: new_head.x++;
					break;
				}
				if(is_inside(new_head) && !collides_with_self(new_head)) {
					body_.try_push(new_head);
					const bool fruit_taken = (fruit_ && new_head == *fruit_);
					const bool pill_taken = (pill_ && new_head == *pill_);
					changes.try_push(change_info{ .pos = head, .value = CHAR_BODY, .color_code = COLOR_YELLOW } );
					changes.try_push(change_info{ .pos = new_head, .value = CHAR_HEAD, .color_code = COLOR_GREEN } );

					if(fruit_taken) {
						fruit_ = {};
						score_++;
						if(delay_ > 50) {
							delay_ -= 10;
						}
					} else {
						std::size_t pops = 1;
						if(pill_taken) {
							pill_ = {};
							pops += 2;
							score_++;
							delay_ += 10;
						}
						while(pops--) {
							if(auto tail = body_.try_pop()) {
								changes.try_push(change_info{ .pos = *tail, .value = ' ' } );
							}
						}
					}
				}
				else {
					is_alive_ = false;
					changes.try_push(change_info{ .pos = head, .value = CHAR_DEAD, .color_code = COLOR_BLUE } );
				}
			}
		}

	private:
		std::uint32_t score_ = 0;
		bool is_alive_ = true;
		bool is_paused_ = false;
		std::optional<position> fruit_;
		std::optional<position> pill_;
		std::size_t delay_ = 300;
		rnd::xorshift32 rng_;
		direction curr_dir_ = direction::up;
		body_buffer body_;
	};

	the_snake snake;

	void draw_field(std::size_t width, std::size_t height) {
		device::uart::printf("\033[2J");  // cls
		device::uart::printf("\033[37m"); // reset attributes
		for (std::size_t x = 1; x <= width; ++x) {
			device::uart::printf("\033[1;%uH#", x);
			device::uart::printf("\033[%u;%uH#", height, x);
		}

		for (std::size_t y = 2; y < height; ++y) {
			device::uart::printf("\033[%u;1H#", y);
			device::uart::printf("\033[%u;%uH#", y, width);
		}
	}

	namespace workers {

		struct kernel_yielder {
			static void yield() noexcept {
				kernel::yield();
			}
		};

		void updater(void *) {
			while(1) {
				if(!snake.paused()) {
					snake.update();
					kernel::sleep(snake.get_delay());
				}
			}
		}

		void drawer(void *) {
			while(1) {
				bool changed = false;
				while(auto c = changes.try_pop()) {
					device::uart::printf("\033[%u;%uH\033[%um%c", c->pos.y + 2, c->pos.x + 2, c->color_code, c->value);
					changed = true;
				}
				if(changed) {
					if(!snake.alive()) {
						device::uart::printf("\033[%u;%uH  GAME OVER  ",
							FIELD_HEIGHT / 2, FIELD_WIDTH / 2 - 5);
					}
					device::uart::printf("\033[%u;%uH\033[37m  Score: %u  Length: %u  Alive: %s  ",
						1, STATUS_LINE_X,
						snake.score(),
						snake.size(),
						snake.alive() ? "Yes" : "No"
					);
				}
				kernel::yield();
			}
		}

		void fruit(void *) {
			rnd::xorshift32 rng;
			while(1) {
				if(!snake.has_fruit()) {
					auto fruit_pill = rng.next() % 100;
					position pos;
					do {
						pos.x = rng.next() % SNAKE_FIELD_WIDTH;
						pos.y = rng.next() % SNAKE_FIELD_HEIGHT;
					} while(snake.collides_with_self(pos));
					if(fruit_pill < 10) {
						snake.set_pill(pos);
						changes.try_push(change_info{.pos = pos, .value = CHAR_PILL, .color_code = COLOR_GREEN });
					}
					else {
						snake.set_fruit(pos);
						changes.try_push(change_info{.pos = pos, .value = CHAR_FRUIT, .color_code = COLOR_RED });
					}
				}
				kernel::sleep(rng.next() % 3000 + 1000);
			}
		}

		void read_command(void*) {
			while (1) {
				if(auto b = device::uart::blocking_read<kernel_yielder>()) {
					switch(b) {
					case 'w': snake.set_direction(direction::up);
						break;
					case 's': snake.set_direction(direction::down);
						break;
					case 'd': snake.set_direction(direction::right);
						break;
					case 'a': snake.set_direction(direction::left);
						break;
					case 'r':
						draw_field(FIELD_WIDTH, FIELD_HEIGHT);
						snake.reset();
						break;
					case 'p':
						snake.toggle_pause();
						break;
					}
				}
				kernel::yield();
			}
		}
	}
}

namespace tests {

	int test::run() {
		using config = kernel::config;
		namespace sch_ns = sch::coop_preemptive;
		//using flags = sch_ns::config_flags;
		kernel::init<sch_ns::scheduler, config>();
		device::uart::init_rxtx(false);

		draw_field(FIELD_WIDTH, FIELD_HEIGHT);
		//draw(snake);

		kernel::add_task(&workers::updater);
		kernel::add_task(&workers::drawer);
		kernel::add_task(&workers::read_command);
		kernel::add_task(&workers::fruit);

		kernel::launch(constants::quanta_infinite);
		PANIC("Should not be here");
	};
}

#endif
