/*
 * stable_priority_queue.hpp
 *
 *  Created on: May 5, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/sync/policies/mutex_policy.hpp"
#include "aikartos/sync/spin_lock.hpp"
#include <optional>
#include <cstdint>
#include <array>
#include <algorithm>
#include <concepts>
#include <functional>
#include <mutex>


namespace aikartos::sync {
	template <typename T,
		std::size_t QueueSize = 8,
		typename LessT = std::less<T>, sync::policies::MutexPolicy MutexType = sync::spin_lock<>>
		requires (std::default_initializable<T> && std::copyable<T>)
	class stable_priority_queue {
	public:
		using element_type = T;
		using less_type = LessT;
		using mutex_type = MutexType;
		constexpr static std::size_t queue_size = QueueSize;

		bool try_push(element_type value) {
			std::lock_guard<mutex_type> l(lock_);
			if (count_ == queue_size) {
				return false;
			}
			items_[count_++] = queue_element{ .value = std::move(value), .enqueue_order = index_++ };
			std::push_heap(items_.begin(), std::next(items_.begin(), count_), less_wrapper{});
			return true;
		}

		std::optional<element_type> try_pop() {
			std::lock_guard<mutex_type> l(lock_);
			if (count_ == 0) {
				return {};
			}
			std::pop_heap(items_.begin(), std::next(items_.begin(), count_), less_wrapper{});
			auto [value, index] = std::move(items_[--count_]);
			if (0 == count_) {
				index_ = 0;
			}
			return { value };
		}

	private:

		struct queue_element {
			element_type value = {};
			std::size_t enqueue_order = 0;
		};

		struct less_wrapper {
			bool operator ()(const queue_element& lhs, const queue_element& rhs) {
				const static less_type less{};
				if (less(lhs.value, rhs.value)) {
					return true;
				}
				if (less(rhs.value, lhs.value)) {
					return false;
				}
				return rhs.enqueue_order < lhs.enqueue_order;
			}
		};

		std::array<queue_element, queue_size> items_;
		std::size_t count_ = 0;
		std::size_t index_ = 0;
		mutex_type lock_;
	};
}
