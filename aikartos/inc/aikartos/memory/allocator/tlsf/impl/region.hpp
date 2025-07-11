/**
 * @file memory/allocator/tlsf/region.hpp
 * @brief Dynamic TLSF allocator that places bucket index table inside managed memory.
 *
 * - Dynamically reserves part of the managed memory region for internal metadata.
 * - Automatically computes the optimal number of bucket classes based on region size.
 * - Suitable for general-purpose heaps with flexible layout and runtime configuration.
 * - Uses the core TLSF logic to perform allocation, freeing, splitting, and coalescing.
 * - Does not require external state or static memory regions.
 *
 * Characteristics:
 * - Designed for maximum flexibility and memory utilization.
 * - Metadata and heap are colocated in a single memory block.
 * - Safe bounds-checking during setup and allocation.
 *
 * Limitations:
 * - Slightly reduced usable heap size due to inline index table.
 * - Requires write access to entire managed region, including metadata area.
 *
 *  Created on: May 22, 2025
 *      Author: newenclave
 *  
 */

#pragma once 


#include <cstdint>
#include <cstdlib>
#include "aikartos/memory/allocator/tlsf/base.hpp"

namespace aikartos::memory::allocator::tlsf::impl {

	template <std::size_t SubclassBits, std::size_t MinClassLog2, std::size_t Align>
	class region: public allocator_base {
		using tlsf_base = tlsf::base<SubclassBits, MinClassLog2, Align>;

		struct state {

			inline std::uintptr_t lookup_bucket(std::size_t main_class, std::size_t sub_class) const {
				return indices_ptr_[main_class * tlsf_base::subclass_count + sub_class];
			}

			template<typename T>
			inline void assign_bucket(std::size_t main_class, std::size_t sub_class, T value) const
				requires(std::is_pointer_v<T>)
			{
				indices_ptr_[main_class * tlsf_base::subclass_count + sub_class] = reinterpret_cast<std::uintptr_t>(value);
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
				begin_addr_ = begin_addr;
				end_addr_ = end_addr;
				main_class_count_ = levels;
			}

			std::uintptr_t* indices_ptr_ = nullptr;
			std::size_t main_class_count_ = 0;
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

		void free(void* ptr) override {
			return tlsf_base::free(state_, ptr);
		}

		void init(std::uintptr_t begin_addr, std::uintptr_t end_addr) override {

			const std::size_t total_length = end_addr - begin_addr;
			std::uintptr_t base = utils::align_up(begin_addr, alignof(std::uintptr_t));

			std::size_t level_count = tlsf_base::get_level_count(total_length);
			std::size_t indices_bytes = 0;

			while (level_count > 0) {
				indices_bytes = level_count * tlsf_base::subclass_count * sizeof(std::uintptr_t);
				const std::size_t usable_memory = total_length - indices_bytes;
				std::size_t test_level_count = tlsf_base::get_level_count(usable_memory);
				if (test_level_count >= level_count) {
					break;
				}
				--level_count;
			}

			state_.indices_ptr_ = reinterpret_cast<std::uintptr_t*>(base);
			state_.begin_addr_ = tlsf_base::align_up(base + indices_bytes);
			state_.end_addr_ = end_addr;
			state_.main_class_count_ = level_count;
			tlsf_base::init(state_);
		}

		void dump_heap(tlsf_base::printer_type printer) const override {
			return tlsf_base::dump_heap(state_, printer);
		}

	private:
		state state_;
	};
}
