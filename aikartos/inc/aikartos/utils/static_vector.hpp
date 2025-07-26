/*
 * static_vector.hpp
 *
 *  Created on: Jun 29, 2025
 *      Author: newenclave
 *
 */

#pragma once 

#include <cstdint>
#include <cstddef>

namespace aikatros::utils {

	template<typename T, std::size_t MaximumElements = 10, std::size_t AlignValue = 8>
	class static_vector {

		using buffer_type = std::byte[sizeof(T) * MaximumElements];

	public:

		static_assert(alignof(T) <= AlignValue, "AlignValue must be >= alignof(T)");

		using value_type = T;
		constexpr static std::size_t maximum_elements = MaximumElements;
		constexpr static std::size_t align_value = AlignValue;

		constexpr static_vector() noexcept = default;
		constexpr static_vector(const static_vector&) noexcept = delete;
		static_vector& operator=(const static_vector&) = delete;

		using iterator = value_type*;
		using const_iterator = const value_type*;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		value_type& at(std::size_t idx) noexcept {
			return *std::launder(reinterpret_cast<T*>(data_ + idx * sizeof(T)));
		}

		const value_type& at(std::size_t idx) const noexcept {
			return *std::launder(reinterpret_cast<const T*>(data_ + idx * sizeof(T)));
		}

#pragma region iterators
		iterator begin() noexcept {
			return &at(0);
		}
		const_iterator begin() const noexcept {
			return &at(0);
		}
		iterator end() noexcept {
			return &at(0) + size_;
		}
		const_iterator end() const noexcept {
			return &at(0) + size_;
		}
		const_iterator cbegin() const noexcept {
			return &at(0);
		}
		const_iterator cend() const noexcept {
			return &at(0) + size_;
		}
		reverse_iterator rbegin() noexcept {
			return reverse_iterator(end());
		}
		const_reverse_iterator rbegin() const noexcept {
			return const_reverse_iterator(end());
		}
		reverse_iterator rend() noexcept {
			return reverse_iterator(begin());
		}
		const_reverse_iterator rend() const noexcept {
			return const_reverse_iterator(begin());
		}
#pragma endregion iterators

		~static_vector() {
			clear();
		}

		bool empty() const noexcept {
			return size_ == 0;
		}

		bool full() const noexcept {
			return size_ == maximum_elements;
		}

		void reserve(std::size_t) noexcept {
		}

		std::size_t size() const noexcept {
			return size_;
		}

		constexpr std::size_t max_size() const noexcept {
			return maximum_elements;
		}

		value_type* data() noexcept {
			return &at(0);
		}

		value_type const* data() const noexcept {
			return &at(0);
		}

		value_type& back() noexcept {
			return at(size_ - 1);
		}

		const value_type& back() const noexcept {
			return at(size_ - 1);
		}

		value_type& front() noexcept {
			return at(0);
		}

		const value_type& front() const noexcept {
			return at(0);
		}

		value_type& operator[](std::size_t idx) noexcept {
			return at(idx);
		}

		const value_type& operator[](std::size_t idx) const noexcept {
			return at(idx);
		}

		bool push_back(const value_type &value) noexcept {
			if (size_ < maximum_elements) {
				new (&at(size_)) value_type(value);
				++size_;
				return true;
			}
			return false;
		}

		template<typename ... Args>
		bool emplace_back(Args &&...args) {
			if (size_ < maximum_elements) {
				new (&at(size_)) value_type(std::forward<Args>(args)...);
				++size_;
				return true;
			}
			return false;
		}

		bool insert(const_iterator it, const value_type &value) noexcept {
			const std::size_t idx = index_by_iterator(it);
			return insert(idx, value);
		}

		bool insert(std::size_t idx, const value_type &value) noexcept {
			if (expand_at(idx)) {
				new (&at(idx)) value_type(value);
				return true;
			}
			return false;
		}

		template<typename ... Args>
		bool emplace(std::size_t idx, Args &&...args) {
			if (expand_at(idx)) {
				new (&at(idx)) value_type(std::forward<Args>(args)...);
				return true;
			}
			return false;
		}

		template<typename ... Args>
		bool emplace(const_iterator it, Args &&...args) {
			const std::size_t idx = index_by_iterator(it);
			return emplace(idx, std::forward<Args>(args)...);
		}

		bool erase(const_iterator it) noexcept {
			const std::size_t idx = index_by_iterator(it);
			return erase(idx);
		}

		bool erase(std::size_t idx) noexcept {
			if (idx < size_) {
				destroy(&at(idx));
				for (std::size_t i = idx; i < size_ - 1; ++i) {
					new (&at(i)) value_type(std::move(at(i + 1)));
					destroy(&at(i + 1));
				}
				--size_;
				return true;
			}
			return false;
		}

		std::size_t erase(const_iterator first, const_iterator last) noexcept {
			const std::size_t start_idx = index_by_iterator(first);
			const std::size_t end_idx = index_by_iterator(last);

			if ((start_idx < size_) && (end_idx <= size_)
					&& (start_idx < end_idx)) {
				const std::size_t count = end_idx - start_idx;
				for (std::size_t i = start_idx; i < size_ - count; ++i) {
					destroy(&at(i));
					new (&at(i)) value_type(std::move(at(i + count)));
				}
				for (std::size_t i = size_ - count; i < size_; ++i) {
					destroy(&at(i));
				}
				size_ -= count;
				return count;
			}
			return 0;
		}

		void clear() noexcept {
			for (std::size_t i = 0; i < size_; ++i) {
				destroy(&at(i));
			}
			size_ = 0;
		}

		bool pop_back() noexcept {
			if (size_ > 0) {
				destroy(&at(size_ - 1));
				--size_;
				return true;
			}
			return false;
		}

		template<typename ItrT>
		void assign(ItrT b, ItrT e) {
			clear();
			std::size_t count = 0;
			while ((count != max_size()) && (b != e)) {
				new (&at(count++)) value_type(*(b++));
			}
			if (count != max_size()) {
				size_ = count;
			}
		}

		template<typename ItrT>
		void assign_move(ItrT b, ItrT e) {
			clear();
			std::size_t count = 0;
			while ((count != max_size()) && (b != e)) {
				new (&at(count++)) value_type(std::move(*(b++)));
			}
			if (count != max_size()) {
				size_ = count;
			}
		}

		void reduce(std::size_t count) noexcept {
			if (count > size_) {
				count = size_;
			}
			for (std::size_t i = size_ - count; i < size_; ++i) {
				destroy(&at(i));
			}
			size_ -= count;
		}

	private:

		std::size_t index_by_iterator(const_iterator it) const noexcept {
			if ((it < data()) || it >= (data() + maximum_elements)) {
				return maximum_elements; // Invalid index
			}
			return static_cast<std::size_t>(it - data());
		}

		bool expand_at(std::size_t idx) noexcept {
			if ((size_ < maximum_elements) && (idx <= size_)) {
				for (std::size_t i = size_; i > idx; --i) {
					new (&at(i)) value_type(std::move(at(i - 1)));
					destroy(&at(i - 1));
				}
				++size_;
				return true;
			}
			return false;
		}

		// ptr must never be nullptr here
		// because all the pointers are from the local array.
		static void destroy(value_type *ptr) noexcept {
			ptr->~value_type();
		}

		alignas(align_value) buffer_type data_ { };
		std::size_t size_ = 0;
	};
}

