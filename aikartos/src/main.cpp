
#include "tests/tests.hpp"
#include "aikartos/platform/platform.hpp"


std::uint32_t count[tests::COUNT_SIZE] = {};
//std::atomic<std::uint32_t> count[tests::COUNT_SIZE] = {};

int main() {
	aikartos::platform::init_vector_table();
	return tests::test::run();
}
