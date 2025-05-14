/*
 * no_yield.hpp
 *
 *  Created on: May 13, 2025
 *      Author: newenclave
 */

#pragma once

namespace aikartos::sync::policies {
	struct no_yield {
		inline static void yield() noexcept {};
	};
}
