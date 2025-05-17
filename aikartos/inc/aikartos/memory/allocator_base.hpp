/*
 * allocator_base.hpp
 *
 *  Created on: May 17, 2025
 *      Author: newenclave
 *  
 */


#pragma once 

#include <cstdlib>
#include <cstdint>

namespace aikartos::memory {
	class allocator_base {
	public:

		using printer_type = void (*)(const char*, ...);

		virtual ~allocator_base() = default;
		virtual void init(std::uintptr_t begin, std::uintptr_t end) = 0;
		virtual void *alloc(std::size_t size) = 0;
		virtual void free(void *ptr) = 0;
		virtual std::size_t total() const { return 0; }
		virtual void dump_heap(printer_type print) const {  }
		virtual void dump_info(printer_type print) const {  }
	};
}

