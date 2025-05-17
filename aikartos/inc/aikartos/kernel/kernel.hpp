/*
 * kernel.hpp
 *
 *  Created on: May 10, 2025
 *      Author: newenclave
 */

#pragma once

#include "aikartos/kernel/core.hpp"
#include "aikartos/kernel/api.hpp"

namespace aikartos::kernel {

	inline auto yield() -> void { return api::yield(); }
	inline auto get_tick_count() -> std::uint32_t { return core::get_tick_count(); }

	template <
			template<typename...> typename SchedulerT,
			typename ...Args
		>
	inline auto init() {
		return core::init<SchedulerT, Args...>();
	}

	template <typename ...Args>
	inline auto add_task(Args&&...args) {
		return core::add_task(std::forward<Args>(args)...);
	}

	inline auto launch(std::uint32_t quanta) {
		return core::launch(quanta);
	}

	inline void set_scheduler_event_handler(sch::events::handler_type cb) {
		core::register_scheduler_event_handler(cb);
	}

	inline auto terminate_current(bool need_yield = true) {
		api::terminate_current(need_yield);
	}

	inline void sleep(std::uint32_t millieconds) { kernel::api::sleep(millieconds); }

}
