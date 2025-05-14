/*
 * object_manager.hpp
 *
 *  Created on: Apr 29, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/kernel/panic.hpp"
#include "aikartos/utils/light_bitset.hpp"
#include <array>
#include <cstdint>

namespace aikartos::utils {
	template <typename T, std::size_t MaximumObjects, std::size_t DataAlign = 8>
	class object_pool {

	public:

		using element_type = T;
		constexpr static std::size_t data_align = DataAlign;
		constexpr static std::size_t maximum_objects = MaximumObjects;
		constexpr static std::size_t object_size = sizeof(element_type);
		using object_array = element_type[maximum_objects];

		using object_ptr = element_type *;

		template <typename ...Args>
		object_ptr alloc(Args&& ...args) {
			auto free_slot = find_free_slot();
			if (free_slot < maximum_objects) {
				allowed_objects_.set(free_slot);
				auto ptr = static_cast<void *>(&data[free_slot]);
				return new (ptr) element_type(std::forward<Args>(args)...);
			} else {
				PANIC("No free slots!");
			}
			return nullptr;
		}

		void free(element_type* ptr) {
			const auto address = reinterpret_cast<std::uintptr_t>(ptr);
			const auto begin = reinterpret_cast<std::uintptr_t>(&data[0]);
			const auto end = reinterpret_cast<std::uintptr_t>(&data[maximum_objects]);
			if ((address < end) && (address >= begin)) {
				const auto position = (address - begin);
				const auto slot = (position / object_size);
	#ifdef DEBUG
				ASSERT((position % object_size) == 0, "Bad position...");
				ASSERT(allowed_objects_.test(slot), "Object is not allocated");
	#endif
				if(allowed_objects_.test(slot)) {
					allowed_objects_.clear(slot);
					ptr->~T();
				}
			}
		}

	private:

		std::size_t find_free_slot() {
			return allowed_objects_.find_zero_bit();
		}

		utils::light_bitset<MaximumObjects> allowed_objects_;
		alignas(data_align) object_array data;
	};
}
