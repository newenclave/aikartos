/**
 * @file memory/allocator/buddy/impl/region.hpp
 * @brief Buddy allocator working over external memory region with dynamic block table.
 *
 * - Uses the buddy memory allocation algorithm with power-of-two block sizes.
 * - Works over a memory region provided at runtime via `init(begin, end)`.
 * - Maintains per-level free lists; levels are dynamically sized based on region size.
 * - Splitting is performed during allocation, down to the smallest sufficient block.
 * - Merging (coalescing) is handled entirely in `free()`, allowing recursive merging
 *   of adjacent buddies back up to the largest block.
 * - Requires minimal metadata overhead; internal block headers track usage and level.
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

	template<std::size_t MinimumLog2Order, std::size_t AlignValue>
	class region: public memory::allocator_base  {

		using buddy_base = memory::allocator::buddy::base<MinimumLog2Order, AlignValue>;

	public:

		static_assert(MinimumLog2Order >= 5, "A minimum value of 5 is required");

		constexpr static std::size_t mininum_order = buddy_base::mininum_order;
		static constexpr std::size_t maximum_order = buddy_base::maximum_order;
		constexpr static std::size_t minimum_value = buddy_base::minimum_value;
		constexpr static std::size_t align_value = buddy_base::align_value;

	private:

		using block_header = buddy_base::block_header;
		using block_header_ptr = buddy_base::block_header_ptr;

	public:

		void init(std::uintptr_t begin, std::uintptr_t end) override {
			const auto fixed_begin = buddy_base::align_up(begin);
			const auto total_size = end - fixed_begin;
			auto [fixed_size, levels] = calculate_sizes(total_size);

			begin_ptr_ = fixed_begin
					+ buddy_base::align_up(levels * sizeof(block_header_ptr));
			levels_ptr_ = reinterpret_cast<block_header_ptr*>(fixed_begin);
			levels_count_ = levels;

			block_header_ptr first_block = buddy_base::create_block(begin_ptr_,
					levels - 1);
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
		static std::tuple<std::size_t, std::size_t> calculate_sizes(
				std::size_t value) {

			std::size_t max_heap_size = buddy_base::ceil_to_log2(value);
			std::size_t order = buddy_base::find_max_log2(max_heap_size);

			while (1) {
				const std::size_t num_levels = order - mininum_order + 1;
				const std::size_t headers_size = buddy_base::align_up(
						sizeof(block_header_ptr) * num_levels);
				const std::size_t heap_size = size_t { 1 } << order;

				if ((heap_size + headers_size) <= value) {
					return {heap_size, num_levels};
				}

				order--;
			}
			return {0, 0};
		}

	private:

		std::uintptr_t begin_ptr_ = 0;
		block_header_ptr *levels_ptr_ = nullptr;
		std::size_t levels_count_ = 0;
	};

}
