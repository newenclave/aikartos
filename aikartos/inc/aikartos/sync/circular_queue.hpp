/*
 * circular_queue.hpp
 *
 *  Created on: May 2, 2025
 *      Author: newenclave
 */

#pragma once

#include <array>
#include <cstdint>
#include <atomic>
#include <optional>

#include "aikartos/sync/policies/mutex_policy.hpp"
#include "aikartos/sync/spin_lock.hpp"
#include "aikartos/sync/lock_guarg.hpp"
#include "aikartos/utils/circular_deque.hpp"

namespace aikartos::sync {
	template <typename T, std::size_t QueueSize, sync::policies::MutexPolicy MutexType = sync::spin_lock<>>
	class circular_queue {
	public:
		using mutex_type = MutexType;
		using element_type = T;
		constexpr static std::size_t queue_size = QueueSize + 1;
		using contailer_type = utils::circular_deque<element_type, queue_size + 1>;

		bool try_push(element_type value) {
			sync::lock_guard<mutex_type> l(lock_);
			return queue_.emplace_back(std::move(value));
		}

		std::optional<element_type> try_get(std::size_t id) const {
			sync::lock_guard<mutex_type> l(lock_);
			if(id < size()) {
				return { queue_[id] };
			}
			return {};
		}

		std::optional<element_type> try_pop() {
			sync::lock_guard<mutex_type> l(lock_);
			return queue_.pop_front();
		}

		bool active() {
			return active_.load(std::memory_order_acquire);
		}

		void stop() {
			active_.store(false, std::memory_order_release);
		}

		std::size_t size() const {
			sync::lock_guard<mutex_type> l(lock_);
			return queue_.size();
		}

		bool empty() const {
			sync::lock_guard<mutex_type> l(lock_);
			return queue_.empty();
		}

		bool full() const {
			sync::lock_guard<mutex_type> l(lock_);
			return queue_.full();
		}

		void clear() {
			sync::lock_guard<mutex_type> l(lock_);
			queue_.clear();
		}

	private:
		contailer_type queue_;
		mutable mutex_type lock_;
		std::atomic<bool> active_ = true;
	};
}
