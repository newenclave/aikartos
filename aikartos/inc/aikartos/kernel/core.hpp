/*
 * kernel.hpp
 *
 *  Created on: May 9, 2025
 *      Author: newenclave
 */

#pragma once

#include <atomic>
#include <cstdint>

#include "aikartos/platform/platform.hpp"
#include "aikartos/const/constants.hpp"
#include "aikartos/device/timebase.hpp"
#include "aikartos/kernel/api.hpp"
#include "aikartos/kernel/config.hpp"
#include "aikartos/kernel/impl.hpp"
#include "aikartos/sch/events.hpp"
#include "aikartos/tasks/object.hpp"
#include "aikartos/utils/object_pool.hpp"
#include "aikartos/sch/statistic.hpp"

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

		using task_block = tasks::control_block;
		using task_entry = tasks::descriptor::task_entry;
		using task_parameter = tasks::descriptor::task_parameter;
		using systick_hook_type = impl_base::systick_hook_type;
		using systick_hook_parameter_type = impl_base::systick_hook_parameter_type;

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
			impl_base::default_quanta_ = quanta;
			init_first_task();
			kernel_launch_impl();
		}

		inline static std::uint32_t get_systick_val() {
			return SysTick->VAL;
		}

		inline static std::uint32_t get_tick_count() {
			return tick_count_;
		}

		inline static std::uint32_t get_quanta() {
			return impl_base::quanta_;
		}

		inline static std::uint32_t get_default_quanta() {
			return impl_base::default_quanta_;
		}

		inline static void register_scheduler_event_handler (sch::events::handler_type cb) {
			instance_->scheduler_event_handler_ = cb;
		}

		inline static sch::events::handler_type get_scheduler_event_handler() {
			return instance_->scheduler_event_handler_;
		}

		inline static void register_systick_hook(systick_hook_type hook, systick_hook_parameter_type param) {
			instance_->systick_hook_ = hook;
			instance_->systick_hook_parameter_ = param;
		}

		inline static systick_hook_type get_systick_hook() {
			return instance_->systick_hook_;
		}

		inline static systick_hook_parameter_type get_systick_hook_parameter() {
			return instance_->systick_hook_parameter_;
		}

		inline static bool get_scheduler_statisctic(sch::statistic_base &stat) {
			return instance_->get_scheduler_statistic(stat);
		}

		inline static void add_task(task_entry task, task_parameter parameter = nullptr) {
			core::add_task(task, tasks::config{}, parameter);
		}

		static void add_task(task_entry task, const tasks::config &config, task_parameter parameter = nullptr);

		constexpr static bool has_fpu() {
#if defined(PLATFORM_USE_FPU) & PLATFORM_FPU_AVAILABLE
			return true;
#endif
			return false;
		}

		static inline bool enable_fpu_hardware() {
			if constexpr (core::has_fpu()) {
				SCB->CPACR |= ((3UL << 10*2) |	// CP10 Full Access
				               (3UL << 11*2));	// CP11 Full Access

				// allow autosave and lazy stack
				FPU->FPCCR |= (FPU_FPCCR_ASPEN_Msk | FPU_FPCCR_LSPEN_Msk);

				__DSB();
				__ISB();
				return true;
			}
			else {
				return false;
			}
		}

#if defined(PLATFORM_USE_FPU)
		static inline void set_task_fpu_default(bool value) {
			impl_base::set_task_fpu_default(value);
		}
		static inline bool get_task_fpu_default() {
			return impl_base::get_task_fpu_default();
		}
#endif

	private:

		static void init_first_task();

		friend struct handlers_friend;

		inline static volatile std::uint32_t tick_count_ = 0;
		inline static impl_base *instance_ = nullptr;
	};

}
