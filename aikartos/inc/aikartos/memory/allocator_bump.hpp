/*
 * allocator_bump.hpp
 *
 *  Created on: May 17, 2025
 *      Author: newenclave
 *  
 */


#pragma once 

#include "allocator_base.hpp"

namespace aikartos::memory {

	template <std::size_t Align = alignof(std::max_align_t)>
	class allocator_bump: public allocator_base {
	public:

		constexpr static std::size_t align_value = Align;

		void init(std::uintptr_t begin, std::uintptr_t end) override {
			end_ptr = end;
			begin_ptr = (begin + align_value - 1) & ~(align_value - 1);
			current_ptr = begin_ptr;
		}

		void* alloc(std::size_t size) override {
			uintptr_t current_addr = current_ptr;
			const uintptr_t aligned_addr = (current_addr + align_value - 1) & ~(align_value - 1);

			if ((aligned_addr + size) > end_ptr) {
				last_error = 1;
				return nullptr;
			}

			current_ptr = aligned_addr + size;
			return reinterpret_cast<void*>(aligned_addr);
		}

		void free(void*) override {}
		std::size_t total() override { return end_ptr - begin_ptr; }

	private:
		int last_error = 0;
		std::uintptr_t begin_ptr;
		std::uintptr_t end_ptr;
		std::uintptr_t current_ptr;
	};

}
