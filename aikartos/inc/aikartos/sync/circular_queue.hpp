/*
 * circular_queue.hpp
 *
 *  Created on: May 2, 2025
 *      Author: newenclave
 */

#pragma once
#include "aikartos/sync/policies/mutex_policy.hpp"
#include "aikartos/sync/spin_lock.hpp"
#include <array>
#include <mutex>
#include <cstdint>
#include <atomic>
#include <optional>


namespace aikartos::sync {
	template <typename T, std::size_t QueueSize, sync::policies::MutexPolicy MutexType = sync::spin_lock<>>
		requires std::default_initializable<T>
	class circular_queue {
	public:
		using mutex_type = MutexType;
		using element_type = T;
		constexpr static std::size_t queue_size = QueueSize + 1;

		bool try_push(element_type value) {
			std::lock_guard<mutex_type> l(lock_);
			const auto next_head = (head_ + 1) % queue_size;
			if (next_head == tail_) {
				return false;
			}
			items_[head_] = std::move(value);
			head_ = next_head;
			return true;
		}

		std::optional<element_type> try_get(std::size_t id) const {
			if(id <= size()) {
				return {items_[(tail_ + id) % queue_size] };
			}
			return {};
		}

		std::optional<element_type> try_pop() {
			std::lock_guard<mutex_type> l(lock_);
			if (head_ == tail_) {
				return {};
			}
			element_type value = std::move(items_[tail_]);
			tail_ = (tail_ + 1) % queue_size;
			return { value };
		}

		bool active() {
			return active_.load(std::memory_order_acquire);
		}

		void stop() {
			active_.store(false, std::memory_order_release);
		}

		std::size_t size() const {
			return (head_ + queue_size - tail_) % queue_size;
		}

		void clear() {
			std::lock_guard<mutex_type> l(lock_);
			head_ = 0;
			tail_ = 0;
		}

	private:
		std::array<element_type, queue_size> items_;
		std::size_t head_ = 0;
		std::size_t tail_ = 0;
		mutex_type lock_;
		std::atomic<bool> active_ = true;
	};
}
