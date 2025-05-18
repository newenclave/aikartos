/*
 *
 * @file allocator_free_list.hpp
 * @brief Single-linked free list allocator with forward coalescing.
 *
 * - Memory is managed as a singly linked list of variable-size blocks.
 * - `alloc()` searches for a suitable free block and splits it if possible.
 * - `free()` marks blocks as free and optionally merges them with next neighbors.
 * - Merge on allocation ensures contiguous memory reuse and reduces fragmentation.
 *
 *  Created on: May 17, 2025
 *      Author: newenclave
 *  
 */

#pragma once 

#include <cstddef>
#include "allocator_base.hpp"

namespace aikartos::memory {

	template <std::size_t Align = alignof(std::max_align_t)>
	class allocator_free_list: public memory::allocator_base {
	public:

		static_assert(Align % 2 == 0, "A power of two is what Align should be");

		constexpr static std::size_t align_value = Align;
		constexpr static std::size_t min_payload = align_value;

	private:
		struct alignas(align_value) block_header {

			std::size_t allocated_size_ = 0;
			block_header* next = nullptr;

			std::size_t size() const {
				return allocated_size_ & ~std::size_t{ 1 };
			}

			void resize(std::size_t value) {
				allocated_size_ = (value | (allocated_size_ & std::size_t{ 1 }));
			}

			bool is_used() const {
				return allocated_size_ & std::size_t{ 1 };
			}

			void mark_used() {
				allocated_size_ |= std::size_t{ 1 };
			}

			void mark_free() {
				allocated_size_ &= ~std::size_t{ 1 };
			}
		};

		constexpr static std::size_t header_size = sizeof(block_header);
	public:

		void init(std::uintptr_t begin, std::uintptr_t end) override {
			end_ptr = end;
			begin_ptr = (begin + align_value - 1) & ~(align_value - 1);
			auto* first_block = get_first_block();
			new (first_block) block_header;
			first_block->resize(end_ptr - begin_ptr);
		}

		void* alloc(std::size_t size) override {
			auto next = get_first_block();
			const auto fixed_size = fix_value_by_align(size);
			const auto full_block_size = fixed_size + header_size;

			while (true) {
				if (!next->is_used()) {
					merge_forward(next);
				}
				if (!next->is_used() && (next->size() >= full_block_size)) {
					break;
				}
				if (!next->next) {
					last_error = 1;
					return nullptr;
				}
				next = next->next;
			}

			// an unused or the very last block
			// the last block;
			const auto allocate_ptr = reinterpret_cast<std::uintptr_t>(next) + header_size;

			// here is:
			// header_size - parent block,
			// full_block_size new chunk (block_header + size)
			// min_payload is just so that no small blocks are left.
			if (next->size() > (header_size + full_block_size + min_payload)) { // possible to insert a block

				auto next_block = create_block(allocate_ptr + fixed_size);

				next_block->resize(next->size() - full_block_size);
				next_block->next = next->next;

				next->next = next_block;
				next->resize(full_block_size);
			}

			last_error = 0;
			next->mark_used();
			return reinterpret_cast<void*>(allocate_ptr);
		}

		void free(void *ptr) override {
			// how to validate the ptr?
			if (ptr == nullptr) {
				return;
			}
			const auto ptr_val = reinterpret_cast<std::uintptr_t>(ptr);
			if ((ptr_val < begin_ptr) || (end_ptr < ptr_val)) {
				return;
			}
			auto* header = get_block_by_ptr(ptr);
			if (header->is_used()) {
				header->mark_free();
				merge_forward(header);
			}
		}

		std::size_t total() const override { return end_ptr - begin_ptr; }

		std::size_t available() const {
			std::size_t total = 0;
			auto* curr = get_first_block();
			while (curr) {
				if (!curr->is_used()) {
					total += curr->size() - header_size;
				}
				curr = curr->next;
			}
			return total;
		}

		void dump_heap(printer_type printer) const override {
			auto* curr = get_first_block();
			printer("Heap layout:\r\n");

			const auto scale_factor = total() / 64;

			while (curr) {
				std::size_t size = curr->size();
				bool used = curr->is_used();

				printer("[");
				std::size_t units = size / scale_factor;
				if(units == 0) {
					units = 1;
				}
				for (std::size_t i = 0; i < units; ++i) {
					printer(used ? "#" : ".");
				}
				printer("] ");

				curr = curr->next;
			}
			printer("\r\n");
		}

		void dump_info(printer_type printer) const override {
			auto* curr = get_first_block();
			int i = 0;
			while (curr) {
				printer("[%d] %s, size: %u, addr: %p\r\n",
				       i++,
				       static_cast<int>(curr->is_used()) ? "used" : "free",
				       curr->size(),
				       reinterpret_cast<void*>(curr));
				curr = curr->next;
			}
			dump_heap(printer);
		}

	private:

		void merge_forward(block_header* header) {
			while (header->next && !header->next->is_used()) {
				header->resize(header->size() + header->next->size());
				header->next = header->next->next;
			}
		}

		block_header* first_header() const {
			return reinterpret_cast<block_header*>(begin_ptr);
		}

		static std::uintptr_t fix_value_by_align(std::uintptr_t value) {
			return (value + (align_value - 1)) & ~(align_value - 1);
		}

		block_header* get_first_block() const {
			return reinterpret_cast<block_header*>(begin_ptr);
		}

		block_header* get_block_by_ptr(void *ptr) const {
			return reinterpret_cast<block_header *>(reinterpret_cast<std::uintptr_t>(ptr) - header_size);
		}

		block_header* create_block(std::uintptr_t ptr) const {
			return new (reinterpret_cast<block_header*>(ptr)) block_header;
		}

		int last_error = 0;
		std::uintptr_t begin_ptr;
		std::uintptr_t end_ptr;
	};

}
