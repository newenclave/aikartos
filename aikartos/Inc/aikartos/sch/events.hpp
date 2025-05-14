/*
 * events.hpp
 *
 *  Created on: May 11, 2025
 *      Author: newenclave
 */

#pragma once

namespace aikartos::sch {

	using scheduler_specific_event = std::uint32_t;

	enum class decision: std::uint32_t {
		CONTINUE,
		RETRY,
	};

	namespace events {
		constexpr scheduler_specific_event OK = 0;
		using handler_type = decision(*)(scheduler_specific_event);
	}

}
