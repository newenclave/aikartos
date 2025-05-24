/**
 * @file tests/the_snake.cpp
 * @brief Snake game demo for a custom RTOS running on STM32 (UART + ANSI terminal).
 *
 * This is a demonstration of building a simple interactive game on top of a cooperative RTOS.
 * It uses UART as input/output and displays everything via ANSI escape codes in a VT100-compatible terminal
 * (e.g., TeraTerm or minicom). The architecture is modular, with each game component running as an independent task.
 *
 * Features:
 * - Real-time input handling via UART (WASD + R to restart).
 * - Efficient partial screen rendering using a change queue.
 * - Snake body stored in a custom circular queue (no dynamic memory allocation).
 * - ANSI color output, fruit generation, scoring, game over state.
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
	constexpr std::uint32_t COLOR_LIGHT = 60;

	const auto printer = device::uart::printf;

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

	struct console {
		static void set_position(std::size_t x, std::size_t y) {
			printer("\033[%u;%uH", y + 1, x + 1);
		}
		static void set_color(std::uint32_t code) {
			printer("\033[%um", code);
		}
		static void clear() {
			printer("\033[2J");
		}
	};

	class the_snake {
	public:

		the_snake() {
			spawn();
		}

		void set_direction(direction dir) {
			if(!paused() && !is_opposite(dir)) {
				curr_dir_ = dir;
			}
		}

		auto get_direction() const {
			return curr_dir_;
		}

		std::size_t length() const {
			return body_.size();
		}

		std::size_t get_delay() const {
			return delay_;
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

		bool paused() const {
			return is_paused_;
		}

		void toggle_pause() {
			is_paused_ = !is_paused_;
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
			const auto len = length();
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
					changes_.try_push(change_info{ .pos = head, .value = CHAR_BODY, .color_code = COLOR_YELLOW } );
					changes_.try_push(change_info{ .pos = new_head, .value = CHAR_HEAD, .color_code = COLOR_GREEN } );

					if(fruit_taken(new_head)) {
						score_++;
						if(delay_ > 50) {
							delay_ -= 10;
						}
					} else {
						std::size_t pops = 1;
						if(pill_taken(new_head)) {
							pops += 2;
							score_++;
							delay_ += 10;
						}
						pop_tail(pops);
					}
				}
				else {
					is_alive_ = false;
					changes_.try_push(change_info{ .pos = head, .value = CHAR_DEAD, .color_code = COLOR_BLUE } );
				}
			}
		}

		changes_buffer &get_changes() {
			return changes_;
		}

		bool collides_with_self(position pos) const {
			for (std::size_t i = 0; i < length(); ++i) {
				if (get_body_element(i) == pos) {
					return true;
				}
			}
			return false;
		}

	private:

		void spawn() {
			auto start_x = rng_.next() % SNAKE_FIELD_WIDTH;
			auto start_y = rng_.next() % SNAKE_FIELD_HEIGHT;
			auto start_dir = rng_.next() % 4;
			body_.try_push({start_x, start_y});
			changes_.try_push(change_info{.pos = {start_x, start_y}, .value = CHAR_HEAD, .color_code = COLOR_GREEN });
			curr_dir_ = static_cast<direction>(start_dir);
		}

		bool is_opposite(direction new_dir) const {
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

		position get_body_element(std::size_t id) const {
			if(auto pos = body_.try_get(id) ) {
				return *pos;
			}
			return {};
		}

		void pop_tail(std::size_t count) {
			while(count--) {
				if(length() == 1) {
					break;
				}
				if(auto tail = body_.try_pop()) {
					changes_.try_push(change_info{ .pos = *tail, .value = ' ' } );
				}
			}
		}

		bool pill_taken(position new_head) {
			return obj_taken(pill_, new_head);
		}

		bool fruit_taken(position new_head) {
			return obj_taken(fruit_, new_head);
		}

		bool obj_taken(std::optional<position> &obj, position new_head) {
			if(obj && new_head == *obj) {
				obj = {};
				return true;
			}
			return false;
		}


		std::uint32_t score_ = 0;
		bool is_alive_ = true;
		bool is_paused_ = false;
		std::optional<position> fruit_;
		std::optional<position> pill_;
		std::size_t delay_ = 300;
		rnd::xorshift32 rng_;
		direction curr_dir_ = direction::up;
		body_buffer body_;
		changes_buffer changes_;

	};

	the_snake snake;

	void draw_field(std::size_t width, std::size_t height) {
		console::clear();
		console::set_color(COLOR_WHITE + COLOR_LIGHT);
		for (std::size_t x = 0; x < width; ++x) {
			console::set_position(x, 0);
			printer("#");
			console::set_position(x, height - 1);
			printer("#");
		}

		for (std::size_t y = 0; y < height; ++y) {
			console::set_position(0, y);
			printer("#");
			console::set_position(width - 1, y);
			printer("#");
		}
	}

	namespace workers {

		struct kernel_yielder {
			static void yield() noexcept {
				kernel::yield();
			}
		};

		void update_game_state(void *) {
			while(1) {
				if(!snake.paused()) {
					snake.update();
					kernel::sleep(snake.get_delay());
				}
			}
		}

		void draw_game(void *) {
			auto &changes = snake.get_changes();
			while(1) {
				bool changed = false;
				while(auto c = changes.try_pop()) {
					console::set_position(c->pos.x + 1, c->pos.y + 1);
					console::set_color(c->color_code);
					printer("%c", c->value);
					changed = true;
				}
				if(changed) {
					if(!snake.alive()) {
						console::set_position(FIELD_WIDTH / 2 - 5, FIELD_HEIGHT / 2);
						console::set_color(COLOR_BLUE);
						printer("GAME OVER");
					}

					console::set_position(STATUS_LINE_X, 0);
					console::set_color(COLOR_WHITE + COLOR_LIGHT);
					device::uart::printf("  Score: %u  Length: %u  Alive: %s  ",
						snake.score(),
						snake.length(),
						snake.alive() ? "Yes" : "No"
					);
				}
				kernel::yield();
			}
		}

		void generate_fruit(void *) {
			rnd::xorshift32 rng;
			auto &changes = snake.get_changes();
			while(1) {
				if(!snake.has_fruit()) {
					auto fruit_pill = rng.next() % 100;
					position pos;
					do {
						pos.x = rng.next() % SNAKE_FIELD_WIDTH;
						pos.y = rng.next() % SNAKE_FIELD_HEIGHT;
					} while(snake.collides_with_self(pos));
					if(fruit_pill < 20) {
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

		kernel::add_task(&workers::update_game_state);
		kernel::add_task(&workers::draw_game);
		kernel::add_task(&workers::read_command);
		kernel::add_task(&workers::generate_fruit);

		kernel::launch(constants::quanta_infinite);
		PANIC("Should not be here");
	};
}

#endif
