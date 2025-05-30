
#include "tests/tests.hpp"
#include "aikartos/platform/platform.hpp"
#include <cstdint>

std::uint32_t count[tests::COUNT_SIZE] = {};
//std::atomic<std::uint32_t> count[tests::COUNT_SIZE] = {};

extern "C" __attribute((weak)) void _init() {}
//extern "C" __attribute((weak)) void _fini() {}

int main() {
	aikartos::platform::init_vector_table();
	return tests::test::run();
}
