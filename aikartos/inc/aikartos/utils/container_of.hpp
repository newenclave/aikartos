/*
 * utils.hpp
 *
 *  Created on: May 11, 2025
 *      Author: newenclave
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace aikartos::utils {
	template <typename ObjectT, typename MemberT>
	constexpr ObjectT* container_of(MemberT* member_ptr, MemberT ObjectT::*member) {
		return reinterpret_cast<ObjectT*>(
			reinterpret_cast<std::uintptr_t>(member_ptr) -
			reinterpret_cast<std::uintptr_t>(&(reinterpret_cast<ObjectT*>(0)->*member))
		);
	}
}
