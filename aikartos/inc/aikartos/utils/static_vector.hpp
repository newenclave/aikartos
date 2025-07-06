/*
 * static_vector.hpp
 *
 *  Created on: Jun 29, 2025
 *      Author: newenclave
 *  
 */


#pragma once 

namespace aikatros::utils {

	template<typename T, std::size_t MaximumElements = 10, std::size_t AlignValue = 8>
	class static_vector {
	public:
		using value_type = T;
		constexpr static std::size_t maximum_elements = MaximumElements;
		constexpr static std::size_t align_value = AlignValue;

		constexpr static_vector() noexcept = default;
		constexpr static_vector(const static_vector&) noexcept = default;
		static_vector& operator=(const static_vector&) = delete;

		using iterator = value_type *;
		using const_iterator = const value_type *;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	#pragma region iterators
		iterator begin() noexcept {
			return data_.data();
		}
		const_iterator begin() const noexcept {
			return data_.data();
		}
		iterator end() noexcept {
			return data_.data() + size_;
		}
		const_iterator end() const noexcept {
			return data_.data() + size_;
		}
		const_iterator cbegin() const noexcept {
			return data_.data();
		}
		const_iterator cend() const noexcept {
			return data_.data() + size_;
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

		bool empty() const noexcept {
			return size_ == 0;
		}

		bool full() const noexcept {
			return size_ == maximum_elements;
		}

		std::size_t size() const noexcept {
			return size_;
		}

		std::size_t max_size() const noexcept {
			return maximum_elements;
		}

		value_type* data() noexcept {
			return data_.data();
		}

		value_type const* data() const noexcept {
			return data_.data();
		}

		value_type& operator[](std::size_t idx) noexcept {
			return data_[idx];
		}

		const value_type& operator[](std::size_t idx) const noexcept {
			return data_[idx];
		}

		bool push_back(const value_type &value) noexcept {
			if (size_ < maximum_elements) {
				data_[size_] = value;
				++size_;
				return true;
			}
			return false;
		}

		template<typename ... Args>
		bool emplace_back(Args &&...args) {
			if (size_ < maximum_elements) {
				new (&data_[size_]) value_type(std::forward<Args>(args)...);
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
				data_[idx] = value;
				return true;
			}
			return false;
		}

		template<typename ... Args>
		bool emplace(std::size_t idx, Args &&...args) {
			if (expand_at(idx)) {
				new (&data_[idx]) value_type(std::forward<Args>(args)...);
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
				destroy(&data_[idx]);
				for (std::size_t i = idx; i < size_ - 1; ++i) {
					new (&data_[i]) value_type(std::move(data_[i + 1]));
					destroy(&data_[i + 1]);
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
					destroy(&data_[i]);
					new (&data_[i]) value_type(std::move(data_[i + count]));
				}
				for (std::size_t i = size_ - count; i < size_; ++i) {
					destroy(&data_[i]);
				}
				size_ -= count;
				return count;
			}
			return 0;
		}

		void clear() noexcept {
			for (std::size_t i = 0; i < size_; ++i) {
				destroy(&data_[i]);
			}
			size_ = 0;
		}

		bool pop_back() noexcept {
			if (size_ > 0) {
				destroy(&data_[size_ - 1]);
				--size_;
				return true;
			}
			return false;
		}

	private:

		std::size_t index_by_iterator(const_iterator it) const noexcept {
			if ((it < data_.data()) || it >= (data_.data() + maximum_elements)) {
				return maximum_elements; // Invalid index
			}
			return static_cast<std::size_t>(it - data_.data());
		}

		bool expand_at(std::size_t idx) noexcept {
			if ((size_ < maximum_elements) && (idx <= size_)) {
				for (std::size_t i = size_; i > idx; --i) {
					new (&data_[i]) value_type(std::move(data_[i - 1]));
					destroy(&data_[i - 1]);
				}
				++size_;
				return true;
			}
			return false;
		}

		static void destroy(value_type *ptr) noexcept {
			ptr->~value_type();
		}

		alignas(align_value) std::array<value_type, maximum_elements> data_ { };
		std::size_t size_ = 0;
	};

}



