/*
 * circular_deque.hpp
 *
 *  Created on: Aug 4, 2025
 *      Author: newenclave
 *  
 */

#pragma once 

#include <cstdint>
#include <new>
#include <tuple>

namespace aikartos::utils {

	template<typename T, std::size_t QueueSize, std::size_t Align = alignof(T)>
	class circular_deque {

		T* at(std::size_t idx) noexcept {
			return std::launder(reinterpret_cast<T*>(items_ + idx * sizeof(T)));
		}

		const T* at(std::size_t idx) const noexcept {
			return std::launder(reinterpret_cast<const T*>(items_ + idx * sizeof(T)));
		}

	public:

		using element_type = T;
		constexpr static std::size_t queue_size = QueueSize;
		constexpr static std::size_t align_as = Align;

		~circular_deque() {
			clear();
		}

		circular_deque() = default;
		circular_deque(const circular_deque&) = delete;
		circular_deque& operator=(const circular_deque&) = delete;

		bool push_back(const element_type &value) {
			return emplace_back(value);
		}

		bool push_back(element_type &&value) {
			return emplace_back(std::move(value));
		}

		template<typename ... Args>
		bool emplace_back(Args &&... args) {
			if (full()) {
				return false;
			}
			new (at(tail_)) element_type(std::forward<Args>(args)...);
			tail_ = (tail_ + 1) % queue_size;
			return true;
		}

		bool push_front(const element_type &value) {
			return emplace_front(value);
		}

		bool push_front(element_type &&value) {
			return emplace_front(std::move(value));
		}

		template<typename ... Args>
		bool emplace_front(Args &&... args) {
			if (full()) {
				return false;
			}
			head_ = (head_ + queue_size - 1) % queue_size;
			new (at(head_)) element_type(std::forward<Args>(args)...);
			return true;
		}

		std::optional<element_type> pop_front() {
			if (empty()) {
				return std::nullopt;
			}
			element_type value = std::move(*at(head_));
			destroy(at(head_));
			head_ = (head_ + 1) % queue_size;
			return { std::move(value) };
		}

		std::optional<element_type> pop_back() {
			if (empty()) {
				return std::nullopt;
			}
			tail_ = (tail_ + queue_size - 1) % queue_size;
			element_type value = std::move(*at(tail_));
			destroy(at(tail_));
			return { std::move(value) };
		}

		element_type& front() {
			return *at(head_);
		}

		const element_type& front() const {
			return *at(head_);
		}

		element_type& back() {
			return *at((tail_ + queue_size - 1) % queue_size);
		}

		const element_type& back() const {
			return *at((tail_ + queue_size - 1) % queue_size);
		}

		element_type& operator[](std::size_t id) {
			return *at((head_ + id) % queue_size);
		}

		const element_type& operator[](std::size_t id) const {
			return *at((head_ + id) % queue_size);
		}

		std::size_t size() const {
			return (tail_ + queue_size - head_) % queue_size;
		}

		bool empty() const {
			return head_ == tail_;
		}

		bool full() const {
			return ((tail_ + 1) % queue_size) == head_;
		}

		void clear() {
			while (!empty()) {
				std::ignore = pop_front();
			}
		}

	private:

		static void destroy(element_type *ptr) {
			ptr->~element_type();
		}

		using buffer_type = std::byte[sizeof(element_type) * queue_size];

		alignas(align_as) buffer_type items_ = { };
		std::size_t head_ = 0;
		std::size_t tail_ = 0;
	};
}
