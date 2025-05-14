/*
 * irq_critical_section.hpp
 *
 *  Created on: May 11, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/device/device.hpp"

namespace aikartos::sync {
	struct irq_critical_section {
		irq_critical_section() {
			lock();
		}
		~irq_critical_section() {
			unlock();
		}
		void lock() {
			__disable_irq();
		}
		void unlock() {
			__enable_irq();
		}
		bool try_lock() {
			lock();
			return true;
		}
	};
}
