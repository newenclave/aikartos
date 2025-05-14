/*
 * static_type_info.hpp
 *
 *  Created on: May 14, 2025
 *      Author: newenclave
 */

#pragma once

#include <cstdint>

namespace aikartos::utils {
	template<typename T>
	struct static_type_info {
		static constexpr std::uintptr_t id() {
			return reinterpret_cast<std::uintptr_t>(&id);
		}
	};
}
