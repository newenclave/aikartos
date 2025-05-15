/*
 * uart.hpp
 *
 *  Created on: May 15, 2025
 *      Author: newenclave
 *  
 */

#pragma once 

#include "aikartos/const/constants.hpp"
#include "aikartos/device/device.hpp"

namespace device {
	class uart {
	public:

		static void init_tx() {
			// enable clock access GPIOA

			namespace constants = aikartos::constants;

			RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN_Msk;

			// set PA2 mode to alternate function
			GPIOA->MODER &= ~(1u << 4u);
			GPIOA->MODER |=  (1u << 5u);

			// Set alternate function to AF7 UART2_TX
			// GPIO_AFRL_AFSEL2_Pos = 8u
			GPIOA->AFR[0] |= ((1u << 8u) | (1u << 9u) | (1u << 10u));
			GPIOA->AFR[0] &= ~(1u << 11u);

			// Enable clock access to UART
			// RCC_APB1RSTR_USART2RST_Pos = 17
			RCC->APB1ENR |= RCC_APB1RSTR_USART2RST_Msk;

			// configure baud rate
			set_baud_rate(constants::system_clock_frequency, constants::default_baud_rate);

			// same as above
			// USART_CR1_RE_Pos for receiving...
			USART2->CR1 = USART_CR1_TE_Msk;
			USART2->CR1 |= USART_CR1_UE_Msk;
		}

		static void blocking_write(const char* data, std::size_t ) {
		    while (*data != '\0') {
		        while (!(USART2->SR & USART_SR_TXE));
		        USART2->DR = *data++;
		    }
		    while (!(USART2->SR & USART_SR_TC));
		}

	private:
		static void set_baud_rate(std::uint32_t periph_clock, std::uint32_t baud_rate) {
			USART2->BRR = calc_baud_rate(periph_clock, baud_rate);
		}

		static std::uint32_t calc_baud_rate(std::uint32_t periph_clock, std::uint32_t baud_rate) {
			return (periph_clock + (baud_rate >> 1)) / baud_rate;
		}
	};
}
