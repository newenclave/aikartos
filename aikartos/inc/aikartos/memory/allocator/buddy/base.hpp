/**
 * @file memory/allocator/buddy/base.hpp
 * @brief Core implementation of the buddy memory allocation algorithm.
 *
 * - Provides the generic, reusable logic for buddy-based memory management.
 * - Implements block splitting on allocation and recursive buddy merging on free.
 * - Blocks are represented by compact headers that encode usage status and level.
 * - Supports efficient address-based buddy computation even on non-aligned memory.
 * - Designed to be used by higher-level allocators (`buddy::impl::region`, `buddy::impl::fixed`)
 *   by supplying block table pointers and memory layout parameters.
 * - Independent of memory storage model: does not own memory or allocate statically.
 * - Exposes static methods for allocation, freeing, heap traversal, and diagnostics.
 *
 * Limitations:
 * - All allocation sizes are rounded up to the nearest power-of-two block size.
 * - Internal fragmentation is unavoidable for small or uneven allocation patterns.
 * - Coalescing relies on both buddy blocks being free and of equal level.
 * - Does not track allocation metadata beyond level/used-bit in headers.
 * - Requires external management of block table storage (`levels_ptr[]`).
 *
 *  Created on: May 20, 2025
 *      Author: newenclave
 *  
 */


#pragma once 

#include <cstdint>
#include <cstdlib>
#include <bit>

#include "aikartos/memory/allocator_base.hpp"

namespace aikartos::memory::allocator::buddy {

	template <std::size_t MinimumLog2Order, std::size_t AlignValue>
	class base {

	public:

		static_assert(MinimumLog2Order >= 5, "The value should be at least 5");

		using printer_type = allocator_base::printer_type;
		constexpr static std::size_t mininum_order = MinimumLog2Order;
		static constexpr std::size_t maximum_order = sizeof(std::size_t) * CHAR_BIT - mininum_order;
		constexpr static std::size_t minimum_value = 1u << mininum_order;
		constexpr static std::size_t align_value = AlignValue;

		struct alignas(align_value) block_header {
			std::size_t level_ = 0;
			block_header* prev = nullptr;
			block_header* next = nullptr;

			std::size_t level() const {
				return level_ >> 1;
			}

			void set_level(std::size_t new_level) {
				level_ = (new_level << 1) | (is_used() ? 1 : 0);
			}

			bool is_used() const {
				return level_ & std::size_t{ 1 };
			}

			void mark_used() {
				level_ |= std::size_t{ 1 };
			}

			void mark_free() {
				level_ &= ~std::size_t{ 1 };
			}

			std::size_t size() const {
				return std::size_t{ 1 } << (level() + mininum_order);
			}

			std::size_t available() const {
				return size() - align_up(sizeof(block_header));
			}

			std::uintptr_t payload() const {
				return reinterpret_cast<std::uintptr_t>(this) + align_up(sizeof(block_header));
			}

			void* payload_ptr() const {
				return reinterpret_cast<void*>(payload());
			}

			std::uintptr_t buddy_addr(std::uintptr_t base_ptr) {
				const auto addr = reinterpret_cast<std::uintptr_t>(this);
				return ((addr - base_ptr) ^ size()) + base_ptr;
			}

			block_header(std::size_t lvl) : level_(lvl << 1) {}
		};

		using block_header_ptr = block_header*;

		static std::size_t total_block_size(std::size_t levels_count) {
			return (std::size_t{ 1 } << (levels_count - 1 + mininum_order));
		}

		static void* alloc(block_header_ptr* levels_storage, std::size_t levels_count, std::size_t value) {
			const auto full_size_value = align_up(sizeof(block_header)) + value;
			const auto level = find_minimum_level(full_size_value);

			auto current_level = level;
			while (current_level < levels_count) {
				if (auto block = levels_storage[current_level]) {

					remove_from_list(levels_storage, block, current_level);

					while (current_level > level) {
						[[maybe_unused]] block_header_ptr new_block = split_block(levels_storage, block);
						current_level--;
						block = levels_storage[current_level];
					}
					remove_from_list(levels_storage, block, current_level);
					block->mark_used();
					return block->payload_ptr();
				}
				current_level++;
			};
			return nullptr;
		}

		static void free(block_header_ptr* levels, std::size_t levels_count, std::uintptr_t begin_ptr, void* ptr) {

			if (!ptr) {
				return;
			}

			const std::uintptr_t end_addr = begin_ptr + (std::size_t{ 1 } << (levels_count - 1 + mininum_order));
			const std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(ptr);
			const std::uintptr_t last = end_addr - minimum_value;
			const std::uintptr_t block_addr = addr - align_up(sizeof(block_header));
			//const std::uintptr_t offset = block_addr - begin_ptr;

			if (addr < begin_ptr || block_addr < begin_ptr || block_addr > last) {
				return;
			}

			block_header_ptr block = reinterpret_cast<block_header_ptr>(block_addr);

			//if (block_addr & ~(block->size() - 1) != 0) {
			//	return;
			//}

			block->mark_free();

			while (auto merged = merge_block(levels, begin_ptr, block)) {
				block = merged;
			}

			//block->mark_free();

		}

		static void draw_heap(std::size_t levels_count, std::uintptr_t begin_ptr, printer_type printer) {
			printer("Buddy heap layout:\r\n");

			std::uintptr_t curr = begin_ptr;
			const std::uintptr_t end = begin_ptr + total_block_size(levels_count);

			printer("[");
			while (curr < end) {
				auto* block = reinterpret_cast<block_header_ptr>(curr);
				std::size_t size = block->size();
				bool used = block->is_used();

				std::size_t units = size / minimum_value;
				for (std::size_t i = 0; i < units; ++i) {
					printer((used ? "#" : "."));
				}
				curr += size;
				if (curr < end) {
					printer("|");
				}
			}
			printer("]\r\n");
		}

		static void print_blocks(std::size_t levels_count, std::uintptr_t begin_ptr, printer_type printer) {
			printer("Buddy heap blocks:\r\n");

			std::uintptr_t curr = begin_ptr;
			std::uintptr_t end = begin_ptr + (std::size_t{ 1 } << (levels_count - 1 + mininum_order));

			int i = 0;
			while (curr < end) {
				auto* block = reinterpret_cast<block_header_ptr>(curr);

				printer("[%d] used: %d, level: %u, size: %u, addr: %p\n",
					i++,
					block->is_used(),
					block->level(),
					block->size(),
					static_cast<void*>(block));
				curr += block->size();
			}
		}

		static block_header_ptr create_block(std::uintptr_t where, std::uintptr_t lvl) {
			return new (reinterpret_cast<block_header_ptr>(where)) block_header(lvl);
		}

		constexpr static std::size_t find_max_log2(std::size_t value) {
			if (value == 0) {
				return 0;
			}
#ifdef __cpp_lib_bitops
			return std::bit_width(value) - 1;
#else
			std::size_t level = 0;
			while (value >>= 1) {
				++level;
			}
			return level;
#endif
		}

		constexpr static std::uintptr_t align_up(std::uintptr_t value) {
			return align_up(value, align_value);
		}

		constexpr static std::uintptr_t align_up(std::uintptr_t value, std::size_t align) {
			return (value + (align - 1)) & ~(align - 1);
		}

		constexpr static std::size_t ceil_to_log2(std::size_t value) {
#ifdef __cpp_lib_bitops
			return std::bit_ceil(value);
#else
			const std::size_t nearest_log2 = find_nearest_log2(value);
			return (nearest_log2 < value) ? (nearest_log2 << 1) : nearest_log2;
#endif
		}

	private:

		static block_header_ptr split_block(block_header_ptr* levels, block_header_ptr block) {
			if (can_split(block) && !block->is_used()) {

				const std::uintptr_t addr = reinterpret_cast<uintptr_t>(block);
				const auto old_level = block->level();
				const auto new_level = old_level - 1;
				const auto new_size = block->size() >> 1;
				const std::uintptr_t middle_addr = addr + new_size;

				block->set_level(new_level);
				auto new_block = create_block(middle_addr, new_level);

				remove_from_list(levels, block, old_level);
				insert_to_list(levels, block, new_level);
				insert_to_list(levels, new_block, new_level);

				return block;
			}
			return nullptr;
		}

		static block_header_ptr merge_block(block_header_ptr* levels, std::uintptr_t begin_ptr, block_header_ptr block) {
			if (can_merge(begin_ptr, block) && !block->is_used()) {

				const std::uintptr_t addr = reinterpret_cast<uintptr_t>(block);
				const auto size = (block->size());
				const auto old_level = block->level();
				const auto result_level = old_level + 1;
				const auto buddy_addr = ((addr - begin_ptr) ^ size) + begin_ptr;

				auto buddy_block = reinterpret_cast<block_header_ptr>(buddy_addr);
				auto result_block = reinterpret_cast<block_header_ptr>(std::min(buddy_addr, addr));

				if (!buddy_block->is_used() && (buddy_block->level() == block->level())) {
					result_block->set_level(result_level);

					block->mark_free();
					buddy_block->mark_free();
					remove_from_list(levels, block, old_level);
					remove_from_list(levels, buddy_block, old_level);

					insert_to_list(levels, result_block, result_level);
					return result_block;
				}
			}
			return nullptr;
		}

		static void insert_to_list(block_header_ptr *levels, block_header_ptr block, std::size_t level) {
			block->prev = nullptr;
			block->next = levels[level];

			if (levels[level]) {
				levels[level]->prev = block;
			}

			levels[level] = block;
		}

		static void remove_from_list(block_header_ptr* levels, block_header_ptr block, std::size_t level) {
			auto prev = block->prev;
			auto next = block->next;
			if (next) {
				next->prev = prev;
			}
			if (prev) {
				prev->next = next;
			}
			else { // the first element
				levels[level] = next;
			}
			block->prev = nullptr;
			block->next = nullptr;
		}

		static bool can_split(block_header_ptr block) {
			return block->level() > 0;
		}

		static bool can_merge(std::size_t levels_count, const block_header_ptr block) {
			return block->level() < (levels_count - 1);
		}


#ifndef __cpp_lib_bitops
		constexpr static std::size_t find_nearest_log2(std::size_t value) {
			return std::size_t{ 1 } << find_max_log2(value);
		}
#endif

		constexpr static std::size_t find_minimum_level(std::size_t size) {
			std::size_t block_size = ceil_to_log2(size);
			return (block_size < minimum_value) ? 0 : find_max_log2(block_size) - mininum_order;
		}

	};

}
