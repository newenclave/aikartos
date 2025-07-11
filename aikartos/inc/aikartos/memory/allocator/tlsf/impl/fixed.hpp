/**
 * @file memory/allocator/tlsf/fixed.hpp
 * @brief Fixed-capacity TLSF allocator with statically allocated bucket index table.
 *
 * - Stores all allocator metadata, including index tables, inside the allocator object.
 * - Suitable for statically sized, pre-allocated memory regions.
 * - Guarantees that no external memory is used for metadata at runtime.
 * - Uses the core logic from `allocator_tlsf_base` to perform allocation and free operations.
 * - Template parameters define maximum managed memory size and bucket configuration.
 *
 * Characteristics:
 * - Ideal for systems with predictable memory needs and limited resources (e.g., embedded).
 * - Fast access to index table via `std::array`, no dynamic memory overhead.
s *
 * Limitations:
 * - Maximum memory size and class count are fixed at compile-time.
 * - Slightly higher binary size due to embedded tables.
 *
 *  Created on: May 22, 2025
 *      Author: newenclave
 *  
 */

#pragma once

#include <cstdint>
#include <cstdlib>
#include <array>

#include "aikartos/memory/allocator/tlsf/base.hpp"

namespace aikartos::memory::allocator::tlsf::impl {

	template <std::size_t MaximumMemory, std::size_t SubclassBits, std::size_t MinClassLog2, std::size_t Align>
	class fixed: public allocator_base {
		using tlsf_base = tlsf::base<SubclassBits, MinClassLog2, Align>;
	public:

		static_assert(MaximumMemory >= tlsf_base::min_usable_size, "Too small value");

		constexpr static std::size_t maximum_memory = MaximumMemory;
		constexpr static std::size_t maximum_class_count = tlsf_base::get_level_count(maximum_memory);

		using subindex_table_type = std::array<std::uintptr_t, tlsf_base::subclass_count>;
		using main_table_type = std::array<subindex_table_type, maximum_class_count>;

	private:

		struct state {

			state() noexcept = default;

			inline std::uintptr_t lookup_bucket(std::size_t main_class, std::size_t sub_class) const {
				return indices_table[main_class][sub_class];
			}

			template<typename T>
			inline void assign_bucket(std::size_t main_class, std::size_t sub_class, T value)
				requires(std::is_pointer_v<T>)
			{
				indices_table[main_class][sub_class] = reinterpret_cast<std::uintptr_t>(value);
			}

			inline std::size_t main_class_count() const {
				return main_class_count_;
			}

			std::uintptr_t get_begin_addr() const {
				return begin_addr_;
			}

			std::uintptr_t get_end_addr() const {
				return end_addr_;
			}

			void commit_region(std::uintptr_t begin_addr, std::uintptr_t end_addr, std::size_t levels) {
				const auto total_size = (end_addr - begin_addr);
				begin_addr_ = begin_addr;
				end_addr_ = (total_size <= maximum_memory) ? end_addr : begin_addr_ + maximum_memory;
				main_class_count_ = levels < maximum_class_count ? levels : maximum_class_count;
			}

			main_table_type indices_table;
			std::size_t main_class_count_ = maximum_class_count;
			std::uintptr_t begin_addr_ = 0;
			std::uintptr_t end_addr_ = 0;
		};

	public:

		virtual std::size_t total() const override {
			return state_.end_addr_ - state_.begin_addr_;
		}

		void* alloc(std::size_t size) override {
			return tlsf_base::allocate(state_, size);
		}

		void *realloc(void *ptr, std::size_t size) override {
			return tlsf_base::reallocate(state_, ptr, size);
		}

		void free(void *ptr) override {
			return tlsf_base::free(state_, ptr);
		}

		void init(std::uintptr_t begin_addr, std::uintptr_t end_addr) override {
			state_.begin_addr_ = tlsf_base::align_up(begin_addr);
			state_.end_addr_ = end_addr;
			tlsf_base::init(state_);
		}

		void dump_heap(tlsf_base::printer_type printer) const override {
			return tlsf_base::dump_heap(state_, printer);
		}

	private:
		state state_;
	};

}

