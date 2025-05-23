/*
 * uart.cpp
 *
 *  Created on: May 23, 2025
 *      Author: newenclave
 *  
 */


#include "aikartos/device/device.hpp"
#include "aikartos/device/uart.hpp"
#include "aikartos/kernel/kernel.hpp"

namespace aikartos::device {

#if 1 //def PLATFORM_f411_CORE

	struct handler_friend {
		static void uart_irq_handler() {
			if (USART2->SR & USART_SR_RXNE) {
				char c = USART2->DR;
				uart::rx_queue.try_push(c);
			}
		}
	};

	extern "C" void USART2_IRQHandler() {
		handler_friend::uart_irq_handler();
	}

	void uart::init_tx() {

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

	void uart::init_rx(bool use_irq) {
		namespace constants = aikartos::constants;

		// enable clock access GPIOA
		namespace constants = aikartos::constants;
		RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN_Msk;

		// set PA3 mode to alternate function
		GPIOA->MODER &= ~(1u << 6u);
		GPIOA->MODER |=  (1u << 7u);

		// Set alternate function to AF7 UART2_RX
		GPIOA->AFR[0] &= ~(0xF << 12);  // PA3 — bits [15:12]
		GPIOA->AFR[0] |=  (0x7 << 12);

		RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

		// Use RX only
		USART2->CR1 = 0;

		set_baud_rate(constants::system_clock_frequency, constants::default_baud_rate);

		USART2->CR1 |= USART_CR1_RE_Msk | (use_irq ? USART_CR1_RXNEIE_Msk : 0);
		USART2->CR1 |= USART_CR1_UE_Msk;
		if(use_irq) {
			uart::irq_used = true;
			NVIC_EnableIRQ(USART2_IRQn);
		}
	}

	void uart::init_rxtx(bool use_rx_irq) {
		namespace constants = aikartos::constants;

		RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN_Msk;

		// TX = PA2
		GPIOA->MODER &= ~(1u << 4u);
		GPIOA->MODER |=  (1u << 5u);
		GPIOA->AFR[0] &= ~(0xF << 8);
		GPIOA->AFR[0] |=  (0x7 << 8);

		// RX = PA3
		GPIOA->MODER &= ~(1u << 6u);
		GPIOA->MODER |=  (1u << 7u);
		GPIOA->AFR[0] &= ~(0xF << 12);
		GPIOA->AFR[0] |=  (0x7 << 12);

		RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

		USART2->CR1 = 0;

		uart::set_baud_rate(constants::system_clock_frequency, constants::default_baud_rate);

		USART2->CR1 |= USART_CR1_RE_Msk | USART_CR1_TE_Msk | (use_rx_irq ? USART_CR1_RXNEIE_Msk : 0);
		USART2->CR1 |= USART_CR1_UE_Msk;
		if(use_rx_irq) {
			uart::irq_used = true;
			NVIC_EnableIRQ(USART2_IRQn);
		}
	}

	bool uart::try_write(std::uint8_t b) {
		if(tx_ready()) {
			 USART2->DR = b;
			 return true;
		}
		return false;
	}

	std::optional<std::uint8_t> uart::try_read() {
		if(uart::irq_used) {
			return uart::rx_queue.try_pop();
		}
		else {
			if(rx_ready()) {
				return {USART2->DR};
			}
			return {};
		}
	}

	inline bool uart::tx_ready() {
		return (USART2->SR & USART_SR_TXE);
	}

	inline bool uart::rx_ready() {
		return (USART2->SR & USART_SR_RXNE);
	}

	void uart::set_baud_rate(std::uint32_t periph_clock, std::uint32_t baud_rate) {
		USART2->BRR = calc_baud_rate(periph_clock, baud_rate);
	}

#endif

}
