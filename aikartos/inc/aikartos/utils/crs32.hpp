/*
 * crs32.hpp
 *
 *  Created on: Jun 7, 2025
 *      Author: newenclave
 *  
 */


#pragma once 

#include <cstdint>

namespace aikartos::utils {
	inline std::uint32_t crc32(const std::uint8_t *data, std::size_t length)
	{
		std::uint32_t crc = 0xFFFFFFFF;
	    while (length--) {
	        crc ^= *data++;
	        for (int i = 0; i < 8; i++) {
	        	crc = (crc >> 1) ^ (0xEDB88320 * (crc & 1));
	        }
	    }
	    return crc ^ 0xFFFFFFFF;
	}

}
