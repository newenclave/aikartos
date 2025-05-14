/*
 * lfsr.hpp
 *
 *  Created on: May 11, 2025
 *      Author: newenclave
 */

#pragma once

#include <cstdint>

namespace aikartos::rnd {
	class lfsr {
	public:
		lfsr(std::uint32_t seed = 0xABCDEFFF)
			: state_(seed ? seed : 0xABCDEFFF)
		{}

		std::uint32_t next() {
			std::uint32_t bit = ((state_ >> 0) ^ (state_ >> 2) ^ (state_ >> 3) ^ (state_ >> 5)) & 1;
			state_ = (state_ >> 1) | (bit << 31);
			return state_;
		}

		void reset_state(std::uint32_t seed = 0xABCDEFFF) {
			state_ = seed ? seed : 0xABCDEFFF;
		}

	private:
		std::uint32_t state_ = 0xABCDEFFF;
	};
}

