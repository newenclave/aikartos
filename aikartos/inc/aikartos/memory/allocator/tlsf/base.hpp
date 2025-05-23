/**
 * @file memory/allocator/tlsf/base.hpp
 * @brief Core implementation of the Two-Level Segregated Fit (TLSF) memory allocator.
 *
 * - Provides the generic, reusable logic for TLSF-style memory management.
 * - Implements two-level size class bucketing with fine-grained subclass separation.
 * - Supports block splitting on allocation and bi-directional merging on free.
 * - Uses compact block headers with physical linkage for fast merge operations.
 * - Designed to be used by higher-level allocators (`tlsf::impl::region`, `tlsf::impl::fixed`)
 *   which provide memory and index storage.
 * - Independent of memory source: does not manage memory buffers directly.
 * - Works with user-defined state objects that satisfy `StateConcept` and `IndicesAccessorConcept`.
 * - Exposes static methods for allocation, deallocation, merging, splitting, and diagnostics.
 *
 * Limitations:
 * - All allocation sizes are rounded up to the next aligned bucket size.
 * - Coalescing occurs only when physically adjacent blocks are free.
 * - Requires external storage for bucket index table and heap memory region.
 *
 *  Created on: May 22, 2025
 *      Author: newenclave
 *  
 */

#pragma once 

#include "aikartos/utils/align_up.hpp"

namespace aikartos::memory::allocator::tlsf {

	template <typename T>
	concept IndicesAccessorConcept = requires(T a, std::size_t i, std::size_t j, void* ptr) {
		{ a.lookup_bucket(i, j) } -> std::convertible_to<std::uintptr_t>;
		{ a.assign_bucket(i, j, ptr) };
		{ a.main_class_count() } -> std::convertible_to<std::size_t>;
	};

	template <typename T>
	concept StateConcept = requires(T a, std::uintptr_t addr, std::size_t level) {
		{ a.get_begin_addr() } -> std::convertible_to<std::uintptr_t>;
		{ a.get_end_addr() } -> std::convertible_to<std::uintptr_t>;
	};

	template<typename T>
	concept HasCommitRegionHook = requires(T a, std::uintptr_t b, std::uintptr_t e, std::size_t l) {
		{ a.commit_region(b, e, l) };
	};

	template <std::size_t SubclassBits = 2, std::size_t MinClassLog2 = 5, std::size_t Align = 8>
	class base {

		struct alignas(Align) block_header {

			block_header* prev_phys = nullptr;
			block_header* prev = nullptr;
			block_header* next = nullptr;
			size_t allocated_size_ = 0;

			block_header(std::size_t s) noexcept {
				resize(s);
			}

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

			constexpr static std::size_t aligned_size() {
				return align_up(sizeof(block_header));
			}

			std::size_t available() const {
				return size() - aligned_size();
			}

			std::uintptr_t payload_addr() const {
				return reinterpret_cast<std::uintptr_t>(this) + aligned_size();
			}

			void* payload_ptr() const {
				return reinterpret_cast<void*>(payload_addr());
			}

			std::uintptr_t addr() const {
				return reinterpret_cast<std::uintptr_t>(this);
			}

			static std::uintptr_t header_addr_from_payload(void* ptr) {
				return reinterpret_cast<std::uintptr_t>(ptr) - aligned_size();
			}

			static block_header* header_ptr_from_payload(void* ptr) {
				return reinterpret_cast<block_header*>(header_addr_from_payload(ptr));
			}

			std::uintptr_t next_physical_addr() const {
				return addr() + size();
			}

			std::uintptr_t prev_physical_addr() const {
				return reinterpret_cast<std::uintptr_t>(prev_phys);
			}
		};

		using block_header_ptr = block_header*;

	public:

		constexpr static std::size_t alignment = Align;
		constexpr static std::size_t subclass_count_log2 = SubclassBits;
		constexpr static std::size_t subclass_count = std::size_t{ 1 } << subclass_count_log2;

		constexpr static std::size_t min_class_log2 = MinClassLog2;
		constexpr static std::size_t min_block_size = std::size_t{ 1 } << min_class_log2;

		constexpr static std::size_t min_payload_size = alignment;
		constexpr static std::size_t min_usable_size = sizeof(block_header) + min_payload_size;

		using printer_type = allocator_base::printer_type;

		template <typename StateT>
		static void init(StateT& state)
			requires StateConcept<StateT> && IndicesAccessorConcept<StateT>
		{
			const std::uintptr_t begin_addr = state.get_begin_addr();
			const std::uintptr_t end_addr = state.get_end_addr();

#ifdef DEBUG
			auto *uptr = reinterpret_cast<std::uint32_t *>(begin_addr);
			while(uptr < reinterpret_cast<std::uint32_t *>(end_addr)) {
				*uptr++ = 0;
			}
			//*--uptr = 0xDEADBEEF;
#endif
			const auto fixed_base = align_up(begin_addr);
			auto total_size = end_addr - fixed_base;
			auto [level, sub_level] = split_size(total_size);

			if constexpr (HasCommitRegionHook<StateT>) {
				state.commit_region(fixed_base, end_addr, level + 1);
				total_size = state.get_end_addr() - state.get_begin_addr();
			}

			for (std::size_t m = 0; m < state.main_class_count(); ++m) {
				for (std::size_t s = 0; s < subclass_count; ++s) {
					state.assign_bucket(m, s, static_cast<void*>(nullptr));
				}
			}
			auto* first_block = create_block(fixed_base, total_size);
			state.assign_bucket(state.main_class_count() - 1, sub_level, first_block);
		}

		template <typename StateT>
		static void* allocate(StateT& state, std::size_t requested_size)
			requires(IndicesAccessorConcept<StateT>&& StateConcept<StateT>)
		{
			if (auto* block = find_proper_block(state, requested_size)) {
				block = split_block(state, block, requested_size);
				block->mark_used();
				return block->payload_ptr();
			}
			return nullptr;
		}

		template <typename StateT>
		static void free(StateT& state, void* ptr)
			requires(IndicesAccessorConcept<StateT> && StateConcept<StateT>)
		{
			if (!ptr) {
				return;
			}

			auto* header = block_header::header_ptr_from_payload(ptr);
			std::uintptr_t payload_addr = reinterpret_cast<std::uintptr_t>(ptr);
			const auto begin_addr = state.get_begin_addr();
			const auto end_addr = state.get_end_addr();

			if ((payload_addr < begin_addr) || (payload_addr >= end_addr)) {
				return;
			}

			std::uintptr_t header_addr = reinterpret_cast<std::uintptr_t>(header);
			if ((header_addr < begin_addr) || (header_addr >= end_addr)) {
				return;
			}

			if (!header->is_used()) {
				return;
			}

			header->mark_free();
			header = merge(state, header);

			insert_to_list(state, header);
		}

		constexpr static std::size_t get_level_count(std::size_t size) {
			const auto [m, s] = split_size(size);
			return m + 1;
		}

		template <StateConcept StateT>
		static void dump_heap(const StateT& state, printer_type printer)
		{
			printer("TLSF heap layout:\r\n[");

			std::uintptr_t curr = state.get_begin_addr();
			const std::uintptr_t end = state.get_end_addr();

			while (curr < end) {
				auto* block = reinterpret_cast<block_header_ptr>(curr);
				std::size_t size = block->size();
				bool used = block->is_used();

				std::size_t units = size / (min_block_size * 4);
				units = units ? units : 1;
				for (std::size_t i = 0; i < units; ++i) {
					printer(used ? "#" : ".");
				}

				curr += size;
				if (curr < end) {
					printer("|");
				}
			}

			printer("]\r\n");
		}

		constexpr static auto align_up(auto value) {
			return utils::align_up(value, alignment);
		}

	private:

		constexpr static std::tuple<std::size_t, std::size_t> split_size(std::size_t size) {
			const auto main_class = find_max_log2(size);
			const auto base = (std::size_t{ 1 } << main_class);

			if (main_class < min_class_log2) {
				return { 0, 0 };
			}

			if (base == size) {
				return { main_class - min_class_log2, 0};
			}

			const auto diff = size - base;
			const auto factor = base / subclass_count;
			const auto sub_class = diff / factor;
			return { main_class - min_class_log2, sub_class };
		}

		template <StateConcept StateT>
		static block_header_ptr get_next_physical(const StateT &state, block_header_ptr block) {
			const std::uintptr_t next_addr = block->next_physical_addr();
			const auto end_addr = (state.get_end_addr() - block_header::aligned_size());
			if (next_addr < end_addr) {
				return reinterpret_cast<block_header_ptr>(next_addr);
			}
			return nullptr;
		}

		template <StateConcept StateT>
		static block_header_ptr get_prev_physical(StateT &, block_header_ptr block) {
			return reinterpret_cast<block_header_ptr>(block->prev_physical_addr());
		}

		template <typename StateT>
		static block_header_ptr find_proper_block(StateT &state, std::size_t requested_size)
			requires (IndicesAccessorConcept<StateT>&& StateConcept<StateT>)
		{

			const std::size_t fixed_size = align_up(requested_size + block_header::aligned_size());
			auto [main_idx, sub_idx] = split_size(fixed_size);

			if(main_idx >= state.main_class_count()) {
				return nullptr;
			}

			const auto find_in = [&state, fixed_size](std::size_t main_idx, std::size_t sub_idx) {
				auto* block = reinterpret_cast<block_header_ptr>(state.lookup_bucket(main_idx, sub_idx));
				if (block && block->size() >= fixed_size) {
					return block;
				}
				return static_cast<block_header_ptr>(nullptr);
			};


			for (std::size_t s = sub_idx; s < subclass_count; ++s) {
				if (auto* block = find_in(main_idx, s)) {
					return block;
				}
			}

			for (std::size_t m = (main_idx + 1); m < state.main_class_count(); ++m) {
				for (std::size_t s = 0; s < subclass_count; ++s) {
					if (auto* block = find_in(m, s)) {
						return block;
					}
				}
			}
			return nullptr;
		}

		template <typename StateT>
		static block_header_ptr split_block(StateT& state, block_header_ptr block, std::size_t requested_size)
			requires(IndicesAccessorConcept<StateT> && StateConcept<StateT>)
		{
			constexpr auto header_size = block_header::aligned_size();
			const auto fixed_size = align_up(header_size + requested_size);
			const auto remaining = block->size() - fixed_size;

			if (remaining < header_size + min_payload_size) {
				return block;
			}

			remove_from_list(state, block);

			auto* next_block = get_next_physical(state, block);

			const auto current_addr = block->addr();
			const auto next_block_addr = current_addr + fixed_size;

			block_header_ptr new_block = create_block(next_block_addr, remaining);

			new_block->prev_phys = block;

			if (next_block) {
				next_block->prev_phys = new_block;
			}

			insert_to_list(state, new_block);

			block->resize(fixed_size);
			block->mark_used();

			return block;
		}

		template <typename StateT>
		static block_header_ptr merge_forward(StateT& state, block_header_ptr block)
			requires(IndicesAccessorConcept<StateT>&& StateConcept<StateT>)
		{
			auto* next_block = get_next_physical(state, block);

			while (next_block && !next_block->is_used()) {
				remove_from_list(state, next_block);
				block->resize(block->size() + next_block->size());
				next_block = get_next_physical(state, block);
			}
			if (next_block) {
				next_block->prev_phys = block;
			}
			return block;
		}

		template <typename StateT>
		static block_header_ptr merge_backward(StateT& state, block_header_ptr block)
			requires(IndicesAccessorConcept<StateT>&& StateConcept<StateT>)
		{
			auto* prev_block = get_prev_physical(state, block);
			auto* next_block = get_next_physical(state, block);

			while (prev_block && !prev_block->is_used()) {
				remove_from_list(state, prev_block);
				prev_block->resize(prev_block->size() + block->size());
				block = prev_block;
				prev_block = get_prev_physical(state, block);
			}

			if (next_block) {
				next_block->prev_phys = block;
			}
			return block;
		}

		template <typename StateT>
		static block_header_ptr merge(StateT& state, block_header_ptr block)
			requires(IndicesAccessorConcept<StateT>&& StateConcept<StateT>)
		{
			return merge_forward(state, merge_backward(state, block));
		}


		template <IndicesAccessorConcept AccessorT>
		static void insert_to_list(AccessorT &accessor, block_header_ptr block) {
			const auto [main_idx, sub_idx] = split_size(block->size());

			block->prev = nullptr;
			block->next = reinterpret_cast<block_header_ptr>(accessor.lookup_bucket(main_idx, sub_idx));

			if (block->next) {
				block->next->prev = block;
			}
			accessor.assign_bucket(main_idx, sub_idx, block);
		}

		template <IndicesAccessorConcept AccessorT>
		static void remove_from_list(AccessorT& accessor, block_header_ptr block) {
			const auto [main_idx, sub_idx] = split_size(block->size());

			auto prev = block->prev;
			auto next = block->next;
			if (next) {
				next->prev = prev;
			}
			if (prev) {
				prev->next = next;
			}
			else { // the block is the first in the list
				accessor.assign_bucket(main_idx, sub_idx, next);
			}
			block->prev = nullptr;
			block->next = nullptr;
		}

		static block_header_ptr create_block(std::uintptr_t addr, std::size_t size) {
			return new (reinterpret_cast<block_header_ptr>(addr)) block_header(size);
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
	};

}
