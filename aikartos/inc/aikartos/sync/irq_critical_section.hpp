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
			__disable_irq();
		}
		~irq_critical_section() {
			__enable_irq();
		}
	};
}
