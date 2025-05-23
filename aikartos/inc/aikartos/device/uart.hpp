/*
 * uart.hpp
 *
 *  Created on: May 15, 2025
 *      Author: newenclave
 *  
 */

#pragma once 

#include <stdarg.h>
#include <stdio.h>
#include <optional>

#include "aikartos/const/constants.hpp"
#include "aikartos/device/device.hpp"
#include "aikartos/sync/policies/yield_policy.hpp"
#include "aikartos/sync/policies/no_yield.hpp"
#include "aikartos/sync/policies/no_mutex.hpp"
#include "aikartos/sync/circular_queue.hpp"

namespace aikartos::device {
	class uart {
	public:

		template <std::size_t BufSize = 128>
		static void printf(const char* format, ...) {
			char buf[BufSize];
			va_list args;
			va_start(args, format);
			int len = vsnprintf(buf, sizeof(buf), format, args);
			va_end(args);

			if (len > 0) {
				if (len > (int)sizeof(buf)) {
					len = sizeof(buf);
				}
				blocking_write(buf, len);
			}
		}

		static void init_tx();
		static void init_rx(bool use_irq = false);
		static void init_rxtx(bool use_rx_irq = false);

		template <aikartos::sync::policies::YieldPolicy YieldT = aikartos::sync::policies::no_yield>
		static void blocking_write(const char* data, std::size_t len) {
			for (std::size_t i = 0; i < len; ++i) {
				while (!try_write(data[i])) {
					YieldT::yield();
				}
			}
			while (!tx_ready());
		}

		template <aikartos::sync::policies::YieldPolicy YieldT = aikartos::sync::policies::no_yield>
		static char blocking_read() {
			while (true) {
				if(auto b = try_read()) {
					return *b;
				}
				YieldT::yield();
			}
		}

		static bool try_write(std::uint8_t b);

		[[nodiscard]]
		static std::optional<std::uint8_t> try_read();
		static bool tx_ready();
		static bool rx_ready();
		static bool using_irq() { return irq_used; }

		inline constexpr static std::uint32_t calc_baud_rate(std::uint32_t periph_clock, std::uint32_t baud_rate) {
			return (periph_clock + (baud_rate >> 1)) / baud_rate;
		}

	private:

		friend struct handler_friend;

		static void set_baud_rate(std::uint32_t periph_clock, std::uint32_t baud_rate);

		static inline sync::circular_queue<char, 64, sync::policies::no_mutex> rx_queue;
		static inline bool irq_used = false;
	};
}
