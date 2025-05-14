/*
 * mutex_policy.hpp
 *
 *  Created on: May 5, 2025
 *      Author: newenclave
 */

#pragma once

#include <concepts>

namespace aikartos::sync::policies {

	template <typename M>
	concept MutexPolicy = requires (M m) {
		m.lock();
		m.unlock();
		{ m.try_lock() } -> std::convertible_to<bool>;
	};

}
