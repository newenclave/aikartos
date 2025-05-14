/*
 * semaphore.hpp
 *
 *  Created on: Apr 28, 2025
 *      Author: newenclave
 */

#pragma once
#include <atomic>

namespace aikartos::sync {
	class semaphore {
	public:
		semaphore(int count = 1) : count_(count) {}

		void acquire() {
			while (count_.load(std::memory_order_acquire) <= 0) {
			}
			count_.fetch_sub(1, std::memory_order_acquire);
		}

		void release() {
			count_.fetch_add(1, std::memory_order_release);
		}

	private:
		std::atomic<int> count_;
	};
}
