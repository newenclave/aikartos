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

		constexpr static std::uint32_t default_sign = 0x424B4941; // AIKB

		struct header {
			std::uint32_t signature; // 'AIKB'
			std::uint32_t modules_count;
			std::uint32_t reserved0;
			std::uint32_t reserved1;
			std::uint32_t reserved2;
			std::uint32_t reserved3;
			std::uint32_t reserved4;
			std::uint32_t reserved5;
		};

		bundle(std::uintptr_t base_addr): base_addr_(base_addr) {}

		std::size_t count() const {
			const auto hdr = reinterpret_cast<const header *>(base_addr_);
			return hdr->modules_count;
		}

		std::optional<module> get_module(std::size_t id) const {
			if(id >= count()) {
				return {};
			}
			const auto *offsets = get_modules_offset();
			return { module { base_addr_ + offsets[id] } };
		}

		static bool is_bundle_address(std::uintptr_t addr) {
			const auto hdr = reinterpret_cast<const header *>(addr);
			return hdr->signature == default_sign;
		}

	private:

		const std::uint32_t *get_modules_offset() const {
			return reinterpret_cast<const std::uint32_t *>(base_addr_ + sizeof(header));
		}

		std::uintptr_t base_addr_ = 0;
	};
}

