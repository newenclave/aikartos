/*
 * memory.cpp
 *
 *  Created on: May 17, 2025
 *      Author: newenclave
 *  
 */

#include <cstdlib>
#include <cstring>

#include "aikartos/device/device.hpp"
#include "aikartos/memory/core.hpp"
#include "aikartos/sync/irq_critical_section.hpp"

extern "C" {
	extern uint8_t _end;
	extern uint8_t _estack;
	extern uint8_t _Min_Stack_Size;
}

namespace aikartos::memory {

	struct memory_core_friend {

		inline static auto alloc(std::size_t size) {
			DEBUG_ASSERT(core::instance_ != nullptr, "Allocator not initialized");
			aikartos::sync::irq_critical_section irq_disable;
			return core::instance_->alloc(size);
		}

		inline static auto calloc(std::size_t size) {
			DEBUG_ASSERT(core::instance_ != nullptr, "Allocator not initialized");
			aikartos::sync::irq_critical_section irq_disable;
			auto *ptr = core::instance_->alloc(size);
			if(ptr) {
				std::memset(ptr, 0, size);
			}
			return ptr;
		}

		inline static auto realloc(void *ptr, std::size_t size) {
			DEBUG_ASSERT(core::instance_ != nullptr, "Allocator not initialized");
			aikartos::sync::irq_critical_section irq_disable;
			return core::instance_->realloc(ptr, size);
		}

		inline static auto free(void *ptr) {
			DEBUG_ASSERT(core::instance_ != nullptr, "Allocator not initialized");
			aikartos::sync::irq_critical_section irq_disable;
			return core::instance_->free(ptr);
		}
	};

	void core::init_allocator() {
		auto min_stack = reinterpret_cast<std::uintptr_t>(&_Min_Stack_Size);
		auto begin_ptr = reinterpret_cast<std::uintptr_t>(&_end);
		auto end_ptr = reinterpret_cast<std::uintptr_t>(&_estack) - min_stack;

		instance_->init(begin_ptr, end_ptr);
	}
}


extern "C" {

	void* malloc(std::size_t size) {
		return aikartos::memory::memory_core_friend::alloc(size);
	}

	void* calloc(std::size_t num, std::size_t size) {
		return aikartos::memory::memory_core_friend::calloc(num * size);
	}

	void* realloc(void *ptr, std::size_t size) {
		return aikartos::memory::memory_core_friend::realloc(ptr, size);
	}

	void free(void* ptr) {
		aikartos::memory::memory_core_friend::free(ptr);
	}
}


