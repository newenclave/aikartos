/*
 * mpu.hpp
 *
 *  Created on: Sep 12, 2025
 *      Author: newenclave
 *  
 */


#pragma once 

#include <cstdint>
#include <bit>

#include "aikartos/platform/platform.hpp"
#include "aikartos/utils/align_down.hpp"
#include "aikartos/utils/align_up.hpp"

namespace aikartos::memory {
	class mpu {
	public:
		enum class access : std::uint32_t {
			// AP[2:0] RASR
			NoAccess = 0b000,
			// priv_unpriv access
			RO_No = 0b101,
			RO_RO = 0b110,
			RW_No = 0b001,
			RW_RO = 0b010,
			RW_RW = 0b011,
		};

		enum class type : std::uint32_t {
			// TEX,C,B,S
			strongly_ordered = (0u << 19) | (0u << 17) | (0u << 16) | (1u << 18), // S=1,C=0,B=0 (Device/Strongly-ordered)
			device_shared = (0u << 19) | (0u << 17) | (1u << 16) | (1u << 18), // S=1,C=0,B=1
			normal_non_cache = (1u << 19) | (0u << 17) | (0u << 16) | (1u << 18), // TEX=1,S=1,C=0,B=0 (Normal, non-cache)
			normal_WBWA = (1u << 19) | (1u << 17) | (1u << 16) | (1u << 18), // Normal, Write-Back, Write-Allocate
		};

		struct region_config {
			std::uint32_t index; // region index
			std::uintptr_t base; // base address. should be aligned by size
			std::uint32_t size;  // should be pow of 2
			access ap;
			type memtype = type::normal_non_cache;
			bool execute = true; // executable
			std::uint8_t subregion_disable = 0x00; // SRD: 1 bit for every sr
		};

		constexpr static std::uint32_t round_up_pow2_ge32(std::uint32_t x) {
			if (x <= 32u) {
				return 32u;
			}
			x--;
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			x |= x >> 8;
			x |= x >> 16;
			x++;
			return x;
		}

		constexpr static int size_to_rasr_sz(uint32_t size) {
			return (size < 32u) ? -1 : (31 - std::countl_zero(size)) - 1;
			//return (size < 32u) ? -1 : (31 - __builtin_clz(size)) - 1;
		}

		constexpr static bool is_power_of_two(std::uint32_t x) {
			return (x > 0) && ((x & (x - 1u)) == 0u);
		}

		static bool supported() {
			return (MPU->TYPE & 0xFFu) != 0u; // DREGION > 0
		}

		static std::uint32_t regions() {
			return ((MPU->TYPE >> 8) & 0xFFu); // DREGIONs
		}

		// privdefena - the kernel code has access by-default (true)
		// NMI and HardFault should follow the MPU rules (false)
		static void enable(bool privdefena = true, bool hfnmiena = false) {
			__DMB();
			MPU->CTRL = (hfnmiena ? MPU_CTRL_HFNMIENA_Msk : 0u)
					| (privdefena ? MPU_CTRL_PRIVDEFENA_Msk : 0u)
					| MPU_CTRL_ENABLE_Msk;
			__DSB();
			__ISB();
		}
		static void disable() {
			__DMB();
			MPU->CTRL = 0u;
			__DSB();
			__ISB();
		}

		static bool configure_region(const region_config &cfg) {
			if (!supported()) {
				return false;
			}

			if (cfg.index >= regions()) {
				return false;
			}

			if (!is_power_of_two(cfg.size)) {
				return false;
			}

			if (cfg.size < 32u) {
				return false;
			}

			if ((cfg.base & (cfg.size - 1u)) != 0u) {
				return false;
			}

			const int rasr_sz = size_to_rasr_sz(cfg.size);
			if (rasr_sz < 0 || rasr_sz > 31) {
				return false;
			}

			const std::uint32_t xn = cfg.execute ? 0u : (1u << 28);
			const std::uint32_t ap = (static_cast<std::uint32_t>(cfg.ap) & 0b111u) << 24;
			const std::uint32_t srd = (static_cast<std::uint32_t>(cfg.subregion_disable) & 0xFFu) << 8;
			const std::uint32_t mem = static_cast<std::uint32_t>(cfg.memtype); // TEX,S,C,B
			const std::uint32_t size_field = (static_cast<std::uint32_t>(rasr_sz) & 0x1Fu) << 1;

			__DMB();
			const std::uint32_t was_enabled = (MPU->CTRL & MPU_CTRL_ENABLE_Msk);
			if (was_enabled) {
				disable();
			}

			MPU->RNR = cfg.index & 0xFFu;

			// RBAR: [31:5] base, [4] VALID=1, [3:0] region
			MPU->RBAR = (cfg.base & 0xFFFFFFE0u) | (1u << 4) | (cfg.index & 0xFu);

			// RASR: attrs + ENABLE
			MPU->RASR = xn | ap | mem | srd | size_field | 1u;

			if (was_enabled) {
				enable((MPU->CTRL & MPU_CTRL_PRIVDEFENA_Msk),
						(MPU->CTRL & MPU_CTRL_HFNMIENA_Msk));
			}
			__DSB();
			__ISB();
			return true;
		}

	    static bool disable_region(uint32_t index) {
			if (!supported() || (index >= regions())) {
				return false;
			}
			__DMB();
			const std::uint32_t was_enabled = MPU->CTRL & MPU_CTRL_ENABLE_Msk;
			if (was_enabled) {
				disable();
			}

			MPU->RNR = index & 0xFFu;
			uint32_t rasr = MPU->RASR;
			rasr &= ~1u;	// ENABLE = 0
			MPU->RASR = rasr;

			if (was_enabled) {
				enable((MPU->CTRL & MPU_CTRL_PRIVDEFENA_Msk),
						(MPU->CTRL & MPU_CTRL_HFNMIENA_Msk));
			}
			__DSB();
			__ISB();
			return true;
		}
	};
}


