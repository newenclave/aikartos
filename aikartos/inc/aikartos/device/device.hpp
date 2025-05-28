/*
 * device.hpp
 *
 *  Created on: May 12, 2025
 *      Author: newenclave
 */

#pragma once

#if defined(PLATFORM_f411_CORE)
#	ifndef STM32F411xE
#		define STM32F411xE
#	endif
#	define PLATFORM_DEFAULT_SYSTEM_CLOCK_FREQUENCY 16'000'000ul
#	include <stm32f4xx.h>

#elif defined(PLATFORM_h753_CORE)
#	ifndef STM32H753xx
#  		define STM32H753xx
#	endif
#	define PLATFORM_DEFAULT_SYSTEM_CLOCK_FREQUENCY 64'000'000ul
#	include <stm32h7xx.h>

#endif

