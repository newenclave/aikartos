/*
 * light_bitset.hpp
 *
 *  Created on: May 12, 2025
 *      Author: newenclave
 */

#pragma once

#include <bit>
#include <cstdint>
#include <climits>

namespace aikartos::utils {
	template <std::size_t Count, typename WordT = std::uint32_t>
	class light_bitset {
	public:
		using data_type = WordT;
		constexpr static std::size_t data_bits = sizeof(data_type) * CHAR_BIT;
		constexpr static std::size_t bits_count = Count;
		constexpr static std::size_t fixed_bits_count = (bits_count + (data_bits - 1)) & ~(data_bits - 1);
		constexpr static std::size_t bucket_count = fixed_bits_count / data_bits;

		inline void set(std::size_t bit_pos) {
			if (bits_count <= bit_pos) {
				return;
			}
			const auto bucket = bit_pos / data_bits;
			const auto pos = bit_pos % data_bits;
			buckets_[bucket] |= (data_type{ 1 } << pos);
		}

		inline void clear(std::size_t bit_pos) {
			if (bits_count <= bit_pos) {
				return;
			}
			const auto bucket = bit_pos / data_bits;
			const auto pos = bit_pos % data_bits;
			buckets_[bucket] &= ~(data_type{ 1 } << pos);
		}

		inline void reset() {
			for (std::size_t b = 0; b < bucket_count; ++b) {
				buckets_[b] = 0;
			}
		}

		[[nodiscard]]
		inline bool test(std::size_t bit_pos) const {
			if (bits_count <= bit_pos) {
				return 0;
			}
			const auto bucket = bit_pos / data_bits;
			const auto pos = bit_pos % data_bits;
			return buckets_[bucket] & (data_type{ 1 } << pos);
		}

		std::size_t find_zero_bit() const {
			for (std::size_t b = 0; b < bucket_count; ++b) {
				data_type bucket = buckets_[b];

				if (bucket == static_cast<data_type>(~data_type(0))) {
					continue;
				}

				int first_zero = 0;

				if constexpr (std::is_same_v<data_type, uint32_t>) {

#if defined(__GNUC__)
					first_zero = __builtin_ffs(~bucket) - 1; // __builtin_ffs is 1-based value
#elif defined(_MSC_VER) // for testing purposes
					unsigned long pos;
					_BitScanForward(&pos, ~bucket);
					first_zero = pos;
#else
					for (first_zero = 0; first_zero < data_bits; ++first_zero) {
						if (!(bucket & (data_type{ 1 } << first_zero))) {
							break;
						}
					}
#endif
				}
				else if constexpr (std::is_same_v<data_type, uint64_t>) {
#if defined(__GNUC__)
					first_zero = __builtin_ffsll(~bucket) - 1; // __builtin_ffs is 1-based value
#elif defined(_MSC_VER) // for testing purposes
					unsigned long pos;
					_BitScanForward64(&pos, ~bucket);
					first_zero = pos;
#else
					for (first_zero = 0; first_zero < data_bits; ++first_zero) {
						if (!(bucket & (data_type{ 1 } << first_zero))) {
							break;
						}
					}
#endif
				}
				else {
					for (first_zero = 0; first_zero < data_bits; ++first_zero) {
						if (!(bucket & (data_type{ 1 } << first_zero))) {
							break;
						}
					}
				}
				std::size_t bit_pos = b * data_bits + first_zero;

				if (bit_pos < bits_count) {
					return bit_pos;
				}
			}
			return bits_count;
		}

		std::size_t find_set_bit() const {
			for (std::size_t b = 0; b < bucket_count; ++b) {
				data_type bucket = buckets_[b];

				if (bucket == 0) {
					continue;
				}

				int first_set = 0;
				if constexpr (std::is_same_v<data_type, std::uint32_t>) {
#if defined(__GNUC__)
					first_set = __builtin_ffs(bucket) - 1; // __builtin_ffs is 1-based value
#elif defined(_MSC_VER) // for testing purposes
					unsigned long pos;
					_BitScanForward(&pos, bucket);
					first_set = pos;
#else
					for (first_set = 0; first_set < data_bits; ++first_set) {
						if (bucket & (data_type{ 1 } << first_set)) {
							break;
						}
					}
#endif
				}
				else if constexpr (std::is_same_v<data_type, std::uint64_t>) {
#if defined(__GNUC__)
					first_set = __builtin_ffsll(bucket) - 1; // __builtin_ffs is 1-based value
#elif defined(_MSC_VER) // for testing purposes
					unsigned long pos;
					_BitScanForward64(&pos, bucket);
					first_set = pos;
#else
					for (first_set = 0; first_set < data_bits; ++first_set) {
						if (bucket & (data_type{ 1 } << first_set)) {
							break;
						}
					}
#endif
				}
				else {
					for (first_set = 0; first_set < data_bits; ++first_set) {
						if (bucket & (data_type{ 1 } << first_set)) {
							break;
						}
					}
				}
				std::size_t bit_pos = b * data_bits + first_set;

				if (bit_pos < bits_count) {
					return bit_pos;
				}
			}
			return bits_count;
		}


#ifdef __cpp_lib_bitops
		std::size_t popcount() const {
			std::size_t total = 0;
			for (std::size_t b = 0; b < bucket_count; ++b) {
				total += std::popcount(buckets_[b]);
			}
			return total;
		}
#else
		std::size_t popcount() const {
			std::size_t result = 0;

			for (std::size_t b = 0; b < bucket_count; ++b) {
				data_type bucket = buckets_[b];

				if constexpr (std::is_same_v<data_type, uint32_t>) {
#if defined(__GNUC__)
					result += __builtin_popcount(bucket);
#elif defined(_MSC_VER)
					result += __popcnt(bucket);
#else
					while(bucket) {
						bucket &= (bucket - 1);
						++result;
					}
#endif
				}
				else if constexpr (std::is_same_v<data_type, uint64_t>) {
#if defined(__GNUC__)
					result += __builtin_popcountll(bucket);
#elif defined(_MSC_VER)
					result += __popcnt64(bucket);
#else
					while(bucket) {
						bucket &= (bucket - 1);
						++result;
					}
#endif
				}
				else {
					while(bucket) {
						bucket &= (bucket - 1);
						++result;
					}
				}
			}

			return result;
		}
#endif

	private:
		data_type buckets_[bucket_count] = {};
	};

}
