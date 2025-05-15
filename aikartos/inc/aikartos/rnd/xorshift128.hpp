/*
 * xorshift128.hpp
 *
 *  Created on: May 13, 2025
 *      Author: newenclave
 */

#pragma once

#include <cstdint>

namespace aikartos::rnd {
	class xorshift128 {
	public:
		xorshift128(uint32_t seed = 0xABCDEFFF)
		{
			reset_state(seed);
		}

		std::uint32_t next() {
			/* Algorithm "xor128" from p. 5 of Marsaglia, "Xorshift RNGs" */
			std::uint32_t t  = state_[3];
			std::uint32_t s  = state_[0];  /* Perform a contrived 32-bit shift. */
			state_[3] = state_[2];
			state_[2] = state_[1];
			state_[1] = s;

			t ^= t << 11;
			t ^= t >> 8;
			return state_[0] = t ^ s ^ (s >> 19);
		}

		void reset_state(std::uint32_t seed = 0xABCDEFFF) {
			state_[0] = state_[1] = state_[2] = state_[3] = seed ? seed : 0xABCDEFFF;
		}

	private:
		std::uint32_t state_[4];
	};
}
