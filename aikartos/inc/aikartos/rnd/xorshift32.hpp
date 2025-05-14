/*
 * xorshift32.hpp
 *
 *  Created on: May 12, 2025
 *      Author: newenclave
 */

#pragma once

#include <cstdint>

namespace aikartos::rnd {
	class xorshift32 {
	public:
		xorshift32(std::uint32_t seed = 0xABCDEFFF)
			: state_(seed ? seed : 0xABCDEFFF) {}

		std::uint32_t next() {
			std::uint32_t x = state_;
			x ^= x << 13;
			x ^= x >> 17;
			x ^= x << 5;
			state_ = x;
			return x;
		}

		void reset_state(std::uint32_t seed = 0xABCDEFFF) {
			state_ = seed ? seed : 0xABCDEFFF;
		}

	private:
		std::uint32_t state_;
	};
}
