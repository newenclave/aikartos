/*
 * core.hpp
 *
 *  Created on: May 17, 2025
 *      Author: newenclave
 *  
 */


#pragma once 

#include <cstdlib>
#include <cstdint>

#include "aikartos/memory/allocator_base.hpp"
#include "aikartos/kernel/panic.hpp"

namespace aikartos::memory {
	class core {
	public:
		template <typename AllocT>
		static void init() {
			static AllocT static_instance;
			if(instance_ == nullptr) {
				instance_ = &static_instance;
				init_allocator();
			}
			else {
				PANIC("Allocator already initialized");
			}
		}

		static std::size_t total_memory() {
			DEBUG_ASSERT(instance_ != nullptr, "Allocator not initialized");
			return instance_->total();
		}

		static memory::allocator_base *get_allocator() {
			return instance_;
		}

	private:
		static void init_allocator();
		friend struct memory_core_friend;
		inline static memory::allocator_base *instance_ = nullptr;
	};
}

