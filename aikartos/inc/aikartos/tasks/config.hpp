/*
 * config.hpp
 *
 *  Created on: May 8, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/utils/sparse_storage.hpp"
#include "aikartos/utils/static_type_info.hpp"

#ifdef DEBUG
# define CONFIG_DEBUG
#endif

#include <limits>

namespace aikartos::tasks {

	enum class priority_class: std::uint8_t {
		HIGH = 0,
		MEDIUM = 1,
		LOW = 2,
	};

	class config {
			template <auto Flag>
			consteval static std::size_t first_bit() {
				using T = decltype(Flag);
				constexpr T value = Flag;
				//using UT = std::make_unsigned_t<std::underlying_type_t<T>>;
				using UT = std::make_unsigned_t<T>;
				UT x = static_cast<UT>(value);

				if (x == 0) {
					return std::numeric_limits<std::size_t>::max();
				}

				std::size_t count = 0;
				while ((x & 1) == 0) {
					x >>= 1;
					++count;
				}
				return count;
			}
		public:

		constexpr static std::size_t maximum_elements = 16;
		using data_type = std::uintptr_t;
#ifdef CONFIG_DEBUG
		struct element_container_type {
			std::uintptr_t tag;
			std::uintptr_t value;
		};
#else
		using element_container_type = data_type;
#endif

		using storage_type = utils::sparse_storage<element_container_type, maximum_elements>;
		template <auto Flag>
		auto set(std::uintptr_t value) {
			using T = decltype(Flag);
			constexpr std::size_t position = first_bit<Flag>();
			constexpr auto flag = static_cast<std::make_unsigned_t<T>>(Flag);
			static_assert(0 == (flag & (flag - 1)), "The flag has more than one bit set");
			static_assert(position < maximum_elements, "Bad flag");
#ifdef CONFIG_DEBUG
			store_.set(position, { .tag = utils::static_type_info<T>::id(), .value = value });
#else
			store_.set(posision, value);
#endif
			return *this;
		}

		template <auto Flag>
		auto get() const {
			using T = decltype(Flag);
			constexpr std::size_t position = first_bit<Flag>();
			constexpr auto flag = static_cast<std::make_unsigned_t<T>>(Flag);
			static_assert(0 == (flag & (flag - 1)), "The flag has more than one bit set");
			static_assert(position < maximum_elements, "Bad flag");
#ifdef CONFIG_DEBUG
			auto *element = store_.get(position);
			if (element) {
				if (element->tag != utils::static_type_info<T>::id()) {
					// process here somehow
					return element ? &element->value : nullptr;
				} else {
					return element ? &element->value : nullptr;
				}
			}
			return static_cast<decltype(&element->value)>(nullptr);
#else
			return store_.get(position);
#endif
		}

		template <auto Flag, typename T>
		constexpr auto update_value(T& value) const {
			if (auto* stored = get<Flag>()) {
				value = static_cast<T>(*stored);
			}
			return *this;
		}

	private:
		storage_type store_;
	};

	// todo: the config for all schedulers. need to think ...
	class old_config {
	public:
		priority_class priority = priority_class::HIGH;

		std::uint32_t relative_deadline = 0;

		std::uint8_t tickets = 1;

		std::uint8_t threshold = 0;
		std::uint8_t delta = 0;
		bool agressive = false;

		std::uint8_t win_threshold = 0;
		std::uint8_t win_delta = 0;
		bool win_agressive = false;
	};
}
