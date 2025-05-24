
#include "tests/tests.hpp"


std::atomic<std::uint32_t> count[tests::COUNT_SIZE] = {};

int main() {
	return tests::test::run();
}
