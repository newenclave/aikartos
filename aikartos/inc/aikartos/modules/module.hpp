/*
 * module.hpp
 *
 *  Created on: Jun 7, 2025
 *      Author: newenclave
 *  
 */


#pragma once 

#include "aikartos/utils/crs32.hpp"

namespace aikartos::modules {
	class module {
	public:

		constexpr static std::uint32_t default_sign = 0x4D4B4941; // AIKM

		struct header {
			std::uint32_t signature;	// 'AIKM' signature
			std::uint32_t image_size;	// only bin payload
			std::uint32_t entry_offset;	// payload offset
			std::uint32_t crc32;		// crc32 header + description + all body
			std::uint32_t reserved0;
			std::uint32_t reserved1;
			std::uint32_t reloc_count;
			std::uint32_t total_size;	// full size = header + description + payload + relocation_table
		};

		struct description {
			const char *str = nullptr;
			std::size_t length = 0;
		};

		module(std::uintptr_t base_addr): base_addr_(base_addr) {}
		bool check_crc32() const {
			const auto hdr = reinterpret_cast<const header *>(base_addr_);
			const std::size_t calc_size = (hdr->total_size - hdr->entry_offset);
			const std::uintptr_t calc_offset = base_addr_ + hdr->entry_offset;
			const auto crc32 = aikartos::utils::crc32(reinterpret_cast<const std::uint8_t *>(calc_offset), calc_size);
			return crc32 == hdr->crc32;
		}

		std::size_t get_image_size() const {
			const auto hdr = reinterpret_cast<const header *>(base_addr_);
			return hdr->image_size;
		}

		description get_description() const {
			const auto hdr = reinterpret_cast<const header *>(base_addr_);
			const auto image_offset = base_addr_ + hdr->entry_offset;
			const std::uintptr_t description_offset = base_addr_ + sizeof(header);
			return description {
				.str = reinterpret_cast<const char *>(description_offset),
				.length = image_offset - description_offset
			};
		}

		bool load(std::uintptr_t addr) {
			const auto hdr = reinterpret_cast<const header *>(base_addr_);
			const auto image_addr = base_addr_ + hdr->entry_offset;
			memcpy(reinterpret_cast<void *>(addr), reinterpret_cast<void *>(image_addr), hdr->image_size);
			load_addr_ = addr;
			apply_relocations(addr);
			return true;
		}

		bool is_loaded() const {
			return load_addr_ != 0;
		}

		std::uintptr_t get_entry_address() const {
			return load_addr_;
		}

		template<typename CallT>
		CallT get_entry_point() const {
			return reinterpret_cast<CallT>(load_addr_ | 1); // thumb interworking
		}

		static bool is_module_address(std::uintptr_t addr) {
			const auto hdr = reinterpret_cast<const header *>(addr);
			return hdr->signature == default_sign;
		}

	private:

		void apply_relocations(std::uintptr_t addr) const {
			const auto hdr = reinterpret_cast<const header *>(base_addr_);
			std::uint32_t *relocation_table = reinterpret_cast<std::uint32_t *>(base_addr_ + hdr->entry_offset + hdr->image_size);
			for(std::size_t i=0; i<hdr->reloc_count; ++i) {
			    const std::uint32_t offset = relocation_table[i];
			    std::uint32_t* reloc_address = reinterpret_cast<std::uint32_t*>(addr + offset);
			    const bool valid = (*reloc_address % 4 == 0)
			    		&& (*reloc_address < hdr->image_size)
						&& (*reloc_address != 0);
			    if(valid) {
				    *reloc_address += addr;
			    }
			}
		}

		std::uintptr_t base_addr_ = 0;
		std::uintptr_t load_addr_ = 0;
	};
}
