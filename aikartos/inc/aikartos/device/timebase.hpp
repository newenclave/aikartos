/*
 * timebase.hpp
 *
 *  Created on: May 15, 2025
 *      Author: newenclave
 *  
 */


#pragma once 

#include "aikartos/const/constants.hpp"
#include "aikartos/device/device.hpp"

namespace aikartos::device {
	class timebase {
	public:
		static void systick_init(std::uint32_t microseconds, std::uint32_t priority = 7) {
			constexpr std::uint32_t count_in_microsecond = aikartos::constants::system_clock_frequency / 1'000'000;
			SysTick->CTRL = 0;
			SysTick->VAL  = 0;
			SysTick->LOAD = (microseconds * count_in_microsecond) - 1;

			NVIC_SetPriority(SysTick_IRQn, priority);

			SysTick->CTRL |= (SysTick_CTRL_CLKSOURCE_Msk
					| SysTick_CTRL_TICKINT_Msk
					| SysTick_CTRL_ENABLE_Msk);

		};
	};
}
