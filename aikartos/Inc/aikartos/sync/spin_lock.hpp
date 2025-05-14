/*
 * spin_lock.hpp
 *
 *  Created on: Apr 27, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/sync/policies/no_yield.hpp"
#include "aikartos/sync/policies/yield_policy.hpp"
#include <atomic>

namespace aikartos::sync {

	template <policies::YieldPolicy YieldPolicyT = sync::policies::no_yield>
	class spin_lock {
	public:

		using yield_policy = YieldPolicyT ;

		void lock() {
			while (flag_.test_and_set(std::memory_order_acquire)) {
				yield_policy::yield();
			}
		}

		void unlock() {
			flag_.clear(std::memory_order_release);
		}

		bool try_lock() {
			return !flag_.test_and_set(std::memory_order_acquire);
		}
	private:
		std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
	};
}
