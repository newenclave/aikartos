/*
 * yield_policy.hpp
 *
 *  Created on: May 13, 2025
 *      Author: newenclave
 */

#pragma once

#include <concepts>

namespace aikartos::sync::policies {
	template<typename T>
	concept YieldPolicy = requires {
	    { T::yield() } noexcept;
	};
}
