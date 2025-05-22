/*
 * scheduler.hpp
 *
 *  Created on: May 18, 2025
 *      Author: newenclave
 *  
 */


#pragma once 

#include "aikartos/utils/sparse_storage.hpp"

namespace aikartos::sch {

	class statistic_base {
	public:
		~statistic_base() noexcept = default;
		virtual std::size_t size() const = 0;
		virtual void add_field(std::size_t pos, std::size_t field, std::uintptr_t value) = 0;
		virtual std::uintptr_t get_field(std::size_t pos, std::size_t field) const = 0;
	};

	template <std::size_t MaximumElements, std::size_t MaximumFields = 16>
	class statistic: public statistic_base {
	public:
		constexpr static std::size_t maximum_elements = MaximumElements;
		constexpr static std::size_t maximum_fields = MaximumFields;
		using statistic_element_type = utils::sparse_storage<std::uintptr_t, MaximumFields>;
		using stogare_type = std::array<statistic_element_type, maximum_elements>;
		std::size_t size() const override {
			return maximum_elements;
		}
		void add_field(std::size_t pos, std::size_t field, std::uintptr_t value) override {
			if(pos < maximum_elements && field < maximum_fields) {
				storage_[pos].set(field, value);
			}
		}
		std::uintptr_t get_field(std::size_t pos, std::size_t field) const override {
			if(pos < maximum_elements && field < maximum_fields) {
				if(auto *value = storage_[pos].get(field)) {
					return *value;
				}
			}
			return {};
		}
	private:
		stogare_type storage_;

	};

	template <typename SchT>
	concept HasGetStatistic =  requires(SchT s, sch::statistic_base &stat_ref) {
		{ s.get_statistic(stat_ref) };
	};

#if 0
	template <typename T>
	class has_get_statistic_method {
	private:

	    template <typename U>
	    static auto test(int) -> decltype(std::declval<U>().get_statistic(std::declval<sch::statistic_base &>()), std::true_type());

	    template <typename>
	    static std::false_type test(...);

	public:
	    static constexpr bool value = decltype(test<T>(0))::value;
	};
#endif
}
