/*
 * sparse_storage.hpp
 *
 *  Created on: May 14, 2025
 *      Author: newenclave
 */

#pragma once

#include <cstdint>
#include <concepts>
#include <array>

#include "aikartos/utils/light_bitset.hpp"

namespace aikartos::utils {

	template <typename DataT, std::size_t MaximumElements>
		requires (std::default_initializable<DataT> && std::movable<DataT>)
	class sparse_storage {
	public:

		constexpr static std::size_t maximum_elements = MaximumElements;
		using data_type = DataT;

		using storage_type = std::array<data_type, maximum_elements>;
		using bitset_type = utils::light_bitset<maximum_elements>;

		bool set(std::size_t pos, data_type value) {
			if (pos >= maximum_elements) {
				return false;
			}
			set_.set(pos);
			cfg_[pos] = std::move(value);
			return true;
		}

		void clear(std::size_t pos) {
			if (pos < maximum_elements) {
				set_.clear(pos);
#ifdef DEBUG
				cfg_[pos] = {};
#endif
			}
		}

		const data_type *get(std::size_t pos) const {
			return set_.test(pos) ? &cfg_[pos] : nullptr;
		}

	private:
		bitset_type set_;
		storage_type cfg_ = {};
	};

}
