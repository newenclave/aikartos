/*
 * lock_guarg.hpp
 *
 *  Created on: May 29, 2025
 *      Author: newenclave
 *  
 */


#pragma once 

#include "aikartos/sync/policies/mutex_policy.hpp"

namespace aikartos::sync {

	template <sync::policies::MutexPolicy MutexT>
	class lock_guard {
	public:
		using mutex_type = MutexT;

		lock_guard(lock_guard &&) = delete;
		lock_guard(const lock_guard &) = delete;
		lock_guard& operator = (lock_guard &&) = delete;
		lock_guard& operator = (const lock_guard &) = delete;

		lock_guard(mutex_type& m): mutex_(m) {
			mutex_.lock();
		}
		~lock_guard() {
			mutex_.unlock();
		}
	private:
		MutexT &mutex_;
	};

}



