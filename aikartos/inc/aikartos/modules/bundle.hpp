/*
 * bundle.hpp
 *
 *  Created on: Jun 7, 2025
 *      Author: newenclave
 *  
 */


#pragma once 

#include <cstdint>
#include <optional>

#include "aikartos/modules/module.hpp"

namespace aikartos::modules {

	class bundle {
	public:
		struct header {
			std::uint32_t count;
			std::uint32_t reserved0;
			std::uint32_t reserved1;
			std::uint32_t reserved2;
		};

		bundle(std::uintptr_t base_addr): base_addr_(base_addr) {}

		std::size_t count() const {
			const auto hdr = reinterpret_cast<const header *>(base_addr_);
			return hdr->count;
		}

		std::optional<module> get_module(std::size_t id) const {
			if(id >= count()) {
				return {};
			}
			const auto *offsets = get_modules_offset();
			return { module { base_addr_ + offsets[id] } };
		}

	private:

		const std::uint32_t *get_modules_offset() const {
			return reinterpret_cast<const std::uint32_t *>(base_addr_ + sizeof(header));
		}

		std::uintptr_t base_addr_ = 0;
	};
}

