/*
 * kernel.hpp
 *
 *  Created on: May 9, 2025
 *      Author: newenclave
 */

#pragma once

#include <atomic>
#include <cstdint>

#include "aikartos/const/constants.hpp"
#include "aikartos/device/timebase.hpp"
#include "aikartos/kernel/api.hpp"
#include "aikartos/kernel/config.hpp"
#include "aikartos/kernel/impl.hpp"
#include "aikartos/sch/events.hpp"
#include "aikartos/tasks/object.hpp"
#include "aikartos/utils/object_pool.hpp"

extern "C" void kernel_launch_impl();

namespace aikartos::kernel {

#if 0
	template <typename, typename = void>
	struct has_accessor : std::false_type {};

	template <typename T>
	struct has_accessor<T, std::void_t<typename T::accessor>> : std::true_type {};

	template <typename T>
	constexpr bool has_accessor_v = has_accessor<T>::value;
#endif
	class core {
	public:

		using task_block = tasks::control_block<>;
		using task_entry = tasks::descriptor::task_entry;
		using task_parameter = tasks::descriptor::task_parameter;

		template <
				template<typename, typename> typename SchedulerT,
				typename ConfigT
			>
		static auto init() {
			static impl<SchedulerT, ConfigT> instance;
			DEBUG_ASSERT(instance_ == nullptr, "kernel already initialized");
			instance_ = &instance;
		}

		static void launch(std::uint32_t quanta) {

			// SysTick higher priority
			device::timebase::systick_init(1'000, 8);

			//PendSV lower priority
			NVIC_SetPriority(PendSV_IRQn, 15);

			impl_base::quanta_ = quanta;
			kernel_launch_impl();
		}

		static std::uint32_t get_systick_val() {
			return SysTick->VAL;
		}

		static std::uint32_t get_tick_count() {
			return tick_count_;
		}

		static std::uint32_t get_quanta() {
			return impl_base::quanta_;
		}

		static void set_scheduler_event_handler (sch::events::handler_type cb) {
			instance_->set_scheduler_event_handler_ = cb;
		}

		static void add_task(task_entry task, task_parameter parameter = nullptr) {
			core::add_task(task, tasks::config{}, parameter);
		}
		static void add_task(task_entry task, const tasks::config &config, task_parameter parameter = nullptr);

	private:

		friend struct handlers_friend;
		inline static volatile std::uint32_t tick_count_ = 0;
		inline static impl_base *instance_ = nullptr;
	};

}
