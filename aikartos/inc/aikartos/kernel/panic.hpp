/*
 * panic.hpp
 *
 *  Created on: May 1, 2025
 *      Author: newenclave
 */

#pragma once
#include <cstdint>

extern "C" void hard_fault_handler(uint32_t* stack_frame);
[[noreturn]] void panic(const char* reason, const char* file, int line);

#define PANIC(msg) panic(msg, __FILE__, __LINE__)

#define ASSERT(cond, msg) if(!(cond)) { panic(msg, __FILE__, __LINE__); }

#ifdef _DEBUG
#define DEBUG_ASSERT(cond, msg) if(!(cond)) { panic(msg, __FILE__, __LINE__); }
#else
#define DEBUG_ASSERT(cond, msg)
#endif
