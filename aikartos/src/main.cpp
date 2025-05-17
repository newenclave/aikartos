
#include "tests/tests.hpp"
#include <stdlib.h>

std::uint32_t count[tests::COUNT_SIZE] = {};

int main() {
	return tests::test::run();
}
