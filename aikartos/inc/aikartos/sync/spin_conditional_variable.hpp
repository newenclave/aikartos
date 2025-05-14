/*
 * spin_conditional_variable.hpp
 *
 *  Created on: May 11, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/sync/policies/mutex_policy.hpp"
#include "aikartos/sync/policies/yield_policy.hpp"
#include <atomic>

namespace aikartos::sync {

	template <sync::policies::YieldPolicy YieldPolicyT = sync::policies::no_yield>
	class spin_conditional_variable {

		struct waiter {
			std::atomic<bool> notified = false;
			waiter* next = nullptr;
		};

	public:

		using yield_policy = YieldPolicyT;

		template <sync::policies::MutexPolicy MutexT>
		void wait(MutexT& lock) {
			waiter w;
			do {
				w.next = waiters_.load(std::memory_order_acquire);
			} while (!waiters_.compare_exchange_weak(w.next, &w, std::memory_order_release, std::memory_order_acquire));
			lock.unlock();
			while (!w.notified.load(std::memory_order_acquire)) {
				yield_policy::yield();
			}
			lock.lock();
		}

		void notify_one() {
			waiter *w = nullptr;
			do {
				w = waiters_.load(std::memory_order_acquire);
				if (!w) {
					return;
				}
			} while (!waiters_.compare_exchange_weak(w, w->next, std::memory_order_release, std::memory_order_acquire));
			w->notified.store(true, std::memory_order_release);
		}

		void notify_all() {
			waiter *w = nullptr;
			do {
				w = waiters_.load(std::memory_order_acquire);
				if (!w) {
					return;
				}
			} while (!waiters_.compare_exchange_weak(w, nullptr, std::memory_order_release, std::memory_order_acquire));
			while (w) {
				w->notified.store(true, std::memory_order_release);
				w = w->next;
			}
		}

	private:
		std::atomic<waiter*> waiters_ = nullptr;
	};
}
