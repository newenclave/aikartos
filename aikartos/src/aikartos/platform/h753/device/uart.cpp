/*
 * uart.cpp
 *
 *  Created on: May 24, 2025
 *      Author: newenclave
 *  
 */


#include "aikartos/device/device.hpp"
#include "aikartos/device/uart.hpp"
#include "aikartos/kernel/kernel.hpp"

namespace aikartos::device {

#ifdef PLATFORM_h753_CORE

	struct handler_friend {
		static void uart_irq_handler() {
			if (uart::rx_ready()) {
				char c = USART3->RDR;
				uart::rx_queue.try_push(c);
			}
		}
	};

	extern "C" void USART3_IRQHandler() {
		handler_friend::uart_irq_handler();
	}

	void uart::init_rxtx(bool use_irq) {

		constexpr int pin8_shift = (8 - 8) * 4;
		constexpr int pin9_shift = (9 - 8) * 4;

		RCC->AHB4ENR |= RCC_AHB4ENR_GPIODEN;       // PD8/PD9
		RCC->APB1LENR |= RCC_APB1LENR_USART3EN;    // USART3

		// PD8 (TX), PD9 (RX)
		GPIOD->MODER &= ~((3 << (2 * 8)) | (3 << (2 * 9)));
		GPIOD->MODER |=  ((2 << (2 * 8)) | (2 << (2 * 9))); // AF mode


		GPIOD->AFR[1] &= ~(0xF << pin8_shift | 0xF << pin9_shift);
		GPIOD->AFR[1] |=  (0x7 << pin8_shift | 0x7 << pin9_shift); // AF7 (USART3)

		set_baud_rate(constants::uart_clock_frequency, constants::default_baud_rate);
		USART3->CR1 = USART_CR1_TE | USART_CR1_RE;
		uart::irq_used = use_irq;
		if (use_irq) {
			USART3->CR1 |= USART_CR1_RXNEIE;
		}
		USART3->CR1 |= USART_CR1_UE;
		if (use_irq) {
			NVIC_SetPriority(USART3_IRQn, 15);
			NVIC_EnableIRQ(USART3_IRQn);
		}
	}

	void uart::init_tx() {
		constexpr int pin8_shift = (8 - 8) * 4;

		RCC->AHB4ENR |= RCC_AHB4ENR_GPIODEN;
		RCC->APB1LENR |= RCC_APB1LENR_USART3EN;

		// TX: PD8
		GPIOD->MODER &= ~(3 << (2 * 8));
		GPIOD->MODER |=  (2 << (2 * 8));

		GPIOD->AFR[1] &= ~(0xF << pin8_shift);
		GPIOD->AFR[1] |=  (0x7 << pin8_shift); // AF7

		set_baud_rate(constants::uart_clock_frequency, constants::default_baud_rate);

		USART3->CR1 = USART_CR1_TE;
		USART3->CR1 |= USART_CR1_UE;
	}

	void uart::init_rx(bool use_irq) {
		constexpr int pin9_shift = (9 - 8) * 4;

		RCC->AHB4ENR |= RCC_AHB4ENR_GPIODEN;
		RCC->APB1LENR |= RCC_APB1LENR_USART3EN;

		// RX: PD9
		GPIOD->MODER &= ~(3 << (2 * 9));
		GPIOD->MODER |=  (2 << (2 * 9));

		GPIOD->AFR[1] &= ~(0xF << pin9_shift);
		GPIOD->AFR[1] |=  (0x7 << pin9_shift); // AF7

		set_baud_rate(constants::uart_clock_frequency, constants::default_baud_rate);

		USART3->CR1 = USART_CR1_RE;
		if (use_irq) {
			USART3->CR1 |= USART_CR1_RXNEIE;
			uart::irq_used = true;
		}
		USART3->CR1 |= USART_CR1_UE;

		if (use_irq) {
			NVIC_SetPriority(USART3_IRQn, 15);
			NVIC_EnableIRQ(USART3_IRQn);
		}
	}

	bool uart::try_write(std::uint8_t b) {
		if(tx_ready()) {
			 USART3->TDR = b;
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
				return {USART3->RDR};
			}
			return {};
		}
	}

	bool uart::tx_ready() {
		return (USART3->ISR & USART_ISR_TXE_TXFNF);
	}

	inline bool uart::rx_ready() {
		if (USART3->ISR & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE | USART_ISR_PE)) {
			USART3->ICR = 0xFFFFFFFF;
		}
		return (USART3->ISR & USART_ISR_RXNE_RXFNE);
	}

	void uart::set_baud_rate(std::uint32_t periph_clock, std::uint32_t baud_rate) {
		USART3->BRR = calc_baud_rate(periph_clock, baud_rate);
	}

#endif
}
