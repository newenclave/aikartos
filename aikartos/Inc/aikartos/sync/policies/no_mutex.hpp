/*
 * no_mutex.hpp
 *
 *  Created on: May 4, 2025
 *      Author: newenclave
 */

#pragma once

namespace aikartos::sync::policies {
	struct no_mutex {
		void lock() {};
		void unlock() {};
		bool try_lock() { return true; };
	};
}
