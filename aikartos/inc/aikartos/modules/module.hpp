/*
 * module.hpp
 *
 *  Created on: Jun 7, 2025
 *      Author: newenclave
 *  
 */


#pragma once 

#include <string_view>
#include "aikartos/utils/crs32.hpp"

namespace aikartos::modules {
	class module {
	public:

		constexpr static std::uint32_t default_sign = 0x4d4b4941; // 'AIKM AIKa Module'

		enum class relocation_type : std::uint32_t {
		    R_ARM_NONE = 0,
		    R_ARM_ABS32 = 2,
			R_ARM_THM_CALL = 10,
		    R_ARM_THM_MOVW_ABS_NC = 47,
		    R_ARM_THM_MOVT_ABS = 48,
		};

		struct relocation {
			std::uint32_t offset;
			std::uint32_t type;
			std::uint32_t section_idx;
			std::uint32_t symbol_idx;
		};

		struct symbol {
			std::uint32_t value;
			std::uint32_t section_idx;
			std::uint32_t type;
			std::uint32_t reserved1;
		};

		struct section {
			std::uint32_t offset = 0;
			std::uint32_t size = 0;
		};

		struct header {
			std::uint32_t signature = 0;
			std::uint32_t version = 0;
			section binary;
			section relocs;
			section symbols;
			section bss;
			std::uint32_t crc = 0;
			std::uint32_t total_size = 0;
			std::uint32_t entry_offset = 0;
			std::uint32_t reserved0 = 0;
			std::uint32_t reserved1 = 0;
			std::uint32_t reserved2 = 0;
		};

		// Ensure that the sizes of the structures are as expected
		static_assert(sizeof(header) == 64, "header size must be 48 bytes");
		static_assert(sizeof(relocation) == 16, "relocation size must be 16 bytes");
		static_assert(sizeof(symbol) == 16, "symbol size must be 16 bytes");

		constexpr static std::size_t header_length = sizeof(header);

		module(std::uintptr_t base_addr) : base_addr_(base_addr) {}

		header* get_header() const {
			return reinterpret_cast<header*>(base_addr_);
		}

		relocation* get_relocation(std::uint32_t idx) const {
			const auto* hdr = get_header();
			if (idx >= hdr->relocs.size) {
				return nullptr;
			}
			return &reinterpret_cast<relocation*>(base_addr_ + hdr->relocs.offset)[idx];
		}

		std::size_t get_relocations_count() const {
			const auto* hdr = get_header();
			return hdr->relocs.size;
		}

		symbol* get_symbol(std::uint32_t idx = 0) const {
			const auto* hdr = get_header();
			if (idx >= hdr->symbols.size) {
				return nullptr;
			}
			return &reinterpret_cast<symbol*>(base_addr_ + hdr->symbols.offset)[idx];
		}

		std::size_t get_symbols_count() const {
			const auto* hdr = get_header();
			return hdr->symbols.size;
		}

		std::uintptr_t get_binary_base() const {
			const auto* hdr = get_header();
			return base_addr_ + hdr->binary.offset;
		}


		std::size_t get_binary_size() const {
			const auto* hdr = get_header();
			return hdr->binary.size;
		}

		bool check_crc32() const {
			auto* hdr = get_header();
			if (hdr->signature != default_sign) {
				return false; // Invalid magic number
			}
			auto old_crc = hdr->crc;
			hdr->crc = 0; // Set CRC to 0 for calculation
			auto crc = utils::crc32(reinterpret_cast<const std::uint8_t*>(base_addr_), hdr->total_size);
			hdr->crc = old_crc; // Restore original CRC
			return crc == hdr->crc;
		}

		bool load(std::uintptr_t addr) {
			auto* hdr = get_header();
			if (hdr->signature != default_sign) {
				return false; // Invalid magic number
			}
			if((hdr->version & 0xFFFF) != sizeof(header)) {
				return false; // bad header size
			}
			if (hdr->binary.size == 0) {
				return false; // No binary to load
			}
			std::memcpy(reinterpret_cast<void*>(addr), reinterpret_cast<const void*>(base_addr_ + hdr->binary.offset), hdr->binary.size);
			memset(reinterpret_cast<void*>(addr + hdr->bss.offset), 0, hdr->bss.size); // Zero out BSS section
			load_addr_ = addr; // Store the address where the binary was loaded
			apply_reallocs();
			return true;
		}

		std::size_t get_image_size() const {
			const auto hdr = reinterpret_cast<const header *>(base_addr_);
			return hdr->binary.size;
		}

		std::string_view get_description() const {
			const auto hdr = reinterpret_cast<const header *>(base_addr_);
			const auto image_offset = base_addr_ + hdr->binary.offset;
			const std::uintptr_t description_offset = base_addr_ + sizeof(header);
			return {
				reinterpret_cast<const char *>(description_offset),
				reinterpret_cast<const char *>(description_offset) + (image_offset - description_offset)
			};
		}

		bool is_loaded() const {
			return load_addr_ != 0;
		}

		std::uintptr_t get_entry_address() const {
			return load_addr_;
		}

		template<typename CallT>
		CallT get_entry_point() const {
			auto* hdr = get_header();
			if (hdr->signature != default_sign) {
				return nullptr; // Invalid magic number
			}
			const std::uintptr_t entry = (load_addr_ + (hdr->entry_offset & ~0x1)) | 1; // thumb interworking
			return reinterpret_cast<CallT>(entry);
		}

		static bool is_module_address(std::uintptr_t addr) {
			const auto hdr = reinterpret_cast<const header *>(addr);
			return hdr->signature == default_sign;
		}

	private:

		static void apply_thm_call_reloc(std::uintptr_t instr_ptr, std::uint32_t target_addr) {

			const std::uint32_t pc = instr_ptr + 4;

			// must be halfword-aligned
			std::int32_t rel = (static_cast<int32_t>(target_addr) - static_cast<int32_t>(pc)) & ~std::uint32_t{1};

			// Encode 25-bit signed offset into fields
			const std::uint32_t S = (rel >> 24) & 0x1;
			const std::uint32_t J1 = (rel >> 23) & 0x1;
			const std::uint32_t J2 = (rel >> 22) & 0x1;
			const std::uint32_t imm10 = (rel >> 12) & 0x3FF;
			const std::uint32_t imm11 = (rel >> 1) & 0x7FF;

			// J1/J2 encoding:
			const std::uint32_t I1 = ~(J1 ^ S) & 1;
			const std::uint32_t I2 = ~(J2 ^ S) & 1;

			// Patch
			std::uint16_t *hw = reinterpret_cast<std::uint16_t*>(instr_ptr);
			hw[0] = static_cast<const std::uint16_t>(0xF000 | (S << 10) | imm10); // upper halfword
			hw[1] = static_cast<const std::uint16_t>(0xF800 | (I1 << 13) | (I2 << 11) | imm11); // lower halfword
		}

		static void apply_thumb_mov_relocation(std::uint16_t *reloc_ptr, std::uint32_t val, relocation_type type) {
			if (type == relocation_type::R_ARM_THM_MOVT_ABS) {
				val >>= 16; // for MOVT relocation, we need the upper 16 bits
			}

			// split the value into parts according to the ARM Thumb relocation format
			const std::uint32_t imm4 = (val >> 12) & 0xF;  // 4 bits
			const std::uint32_t i = (val >> 11) & 0x1;	   // 1 bit
			const std::uint32_t imm3 = (val >> 8) & 0x7;   // 3 bits
			const std::uint32_t imm8 = val & 0xFF;		   // 8 bits

			const std::uint16_t low = (reloc_ptr[0] & ~0x040F) | (imm4 << 0) | (i << 10);
			const std::uint16_t high = (reloc_ptr[1] & ~0x70FF) | (imm3 << 12) | imm8;

			// write the modified values back to the relocation pointer
			reloc_ptr[0] = low;
			reloc_ptr[1] = high;
		}

		void apply_reallocs() {
			auto* hdr = get_header();
			std::uintptr_t relocated_res = 0;
			for (std::size_t i = 0; i < hdr->relocs.size; ++i) {
				auto *reloc = get_relocation(i);
				auto *symbol = get_symbol(reloc->symbol_idx);
				if (!symbol) {
					PANIC("Invalid symbol. Check the binary structure");
				}

				std::uintptr_t reloc_addr = load_addr_ + reloc->offset;
				const std::uint32_t symbol_value = load_addr_ + symbol->value;

				switch (static_cast<relocation_type>(reloc->type))
				{
				case relocation_type::R_ARM_ABS32:
					symbol->type == 3 // STT_SECTION
						? *reinterpret_cast<std::uint32_t*>(reloc_addr) += symbol_value
						: *reinterpret_cast<std::uint32_t*>(reloc_addr) = symbol_value;
					relocated_res = static_cast<std::uintptr_t>(*reinterpret_cast<std::uint32_t*>(reloc_addr));
					break;
				case relocation_type::R_ARM_THM_CALL:
					apply_thm_call_reloc(reloc_addr, symbol_value);
					break;
				case relocation_type::R_ARM_THM_MOVW_ABS_NC:
					apply_thumb_mov_relocation(
						reinterpret_cast<std::uint16_t *>(reloc_addr),
						symbol_value,
						relocation_type::R_ARM_THM_MOVW_ABS_NC);
					break;
				case relocation_type::R_ARM_THM_MOVT_ABS:
					apply_thumb_mov_relocation(
						reinterpret_cast<std::uint16_t*>(reloc_addr),
						symbol_value,
						relocation_type::R_ARM_THM_MOVT_ABS);
					break;
				case relocation_type::R_ARM_NONE:
					break;
				default:
					PANIC("Unsupported (yet) relocation type.");
					break;
				}
			}
		}

		std::uintptr_t base_addr_ = 0;
		std::uintptr_t load_addr_ = 0;
	};
}
