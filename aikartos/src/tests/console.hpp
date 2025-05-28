/*
 * console.hpp
 *
 *  Created on: May 28, 2025
 *      Author: newenclave
 *  
 */


#pragma once 


namespace tests {
	template<typename PrintT>
	struct console {
		PrintT printer;
		void set_position(std::size_t x, std::size_t y) {
			printer("\033[%u;%uH", y + 1, x + 1);
		}
		void set_color(std::uint32_t code) {
			printer("\033[%um", code);
		}
		void clear() {
			printer("\033[2J");
		}
		void reset_color() {
			printer("\033[0m");
		}
		void hide_cursor() {
			printer("\033[?25l");
		}
		void show_cursor() {
			printer("\033[?25h");
		}
	};
}
