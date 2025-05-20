/**
 * @file allocator/buddy/impl/fixed.hpp
 * @brief Fixed-capacity buddy allocator for embedded use with static free list table.
 *
 * - Implements a classic buddy allocator with statically sized free list array.
 * - Designed for constrained environments where maximum memory size is known at compile-time.
 * - Operates on an external memory region provided via `init(begin, end)`.
 * - Free list structure is allocated internally as a fixed-size array, avoiding dynamic memory.
 * - Supports power-of-two sized allocations with internal splitting and merging.
 * - Efficient for predictable workloads with limited fragmentation tolerance.
 *
 *  Created on: May 20, 2025
 *      Author: newenclave
 *  
 */

#pragma once 

#include <cstdint>
#include <cstdlib>
#include <bit>

#include "aikartos/memory/allocator/buddy/base.hpp"

namespace aikartos::memory::allocator::buddy::impl {

	template<std::size_t MemorySize, std::size_t MinimumLog2Order, std::size_t AlignValue>
	class fixed: public memory::allocator_base {

		using buddy_base = memory::allocator::buddy::base<MinimumLog2Order, AlignValue>;

	public:

		static_assert(MinimumLog2Order >= 5, "A minimum value of 5 is required");
		static_assert(MemorySize >= buddy_base::minimum_value, "The amount of memory requested is too small");

		constexpr static std::size_t mininum_order = buddy_base::mininum_order;
		constexpr static std::size_t memory_reuested = MemorySize;
		constexpr static std::size_t maximum_value = std::size_t { 1 } << buddy_base::find_max_log2(memory_reuested);
		constexpr static std::size_t maximum_level = buddy_base::find_max_log2(memory_reuested) - mininum_order + 1;
		constexpr static std::size_t minimum_value = buddy_base::minimum_value;
		constexpr static std::size_t align_value = buddy_base::align_value;


	private:

		using block_header = buddy_base::block_header;
		using block_header_ptr = buddy_base::block_header_ptr;

	public:

		void init(std::uintptr_t begin, std::uintptr_t end) override {

			const auto total_size = end - begin;
			levels_count_ = maximum_level;

			if (total_size < maximum_value) {
				levels_count_ = buddy_base::find_max_log2(total_size)
						- mininum_order + 1;
			}

			begin_ptr_ = begin;

			block_header *first_block = buddy_base::create_block(begin_ptr_,
					levels_count_ - 1);
			levels_ptr_[levels_count_ - 1] = first_block;

			for (auto lvl = levels_count_ - 1; lvl > 0; --lvl) {
				levels_ptr_[lvl - 1] = nullptr;
			}
		}

		void* alloc(std::size_t value) override {
			return buddy_base::alloc(levels_ptr_, levels_count_, value);
		}

		void free(void *ptr) override {
			return buddy_base::free(levels_ptr_, levels_count_, begin_ptr_, ptr);
		}

		void dump_heap(buddy_base::printer_type printer) const override {
			buddy_base::draw_heap(levels_count_, begin_ptr_, printer);
		}

		void dump_info(buddy_base::printer_type printer) const override {
			buddy_base::print_blocks(levels_count_, begin_ptr_, printer);
		}

		std::size_t total() const override {
			return buddy_base::total_block_size(levels_count_);
		}

	private:

		std::uintptr_t begin_ptr_ = 0;
		std::size_t levels_count_ = 0;
		block_header *levels_ptr_[maximum_level];
	};
}
