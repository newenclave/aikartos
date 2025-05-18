/*
 * config.hpp
 *
 *  Created on: May 8, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/utils/flagged_storage.hpp"

#include <limits>

namespace aikartos::tasks {

	enum class priority_class: std::uint8_t {
		HIGH = 0,
		MEDIUM = 1,
		LOW = 2,
	};

	using config = utils::flagged_storage<16>;

}
