
#include "tests/tests.hpp"

std::uint32_t count[tests::COUNT_SIZE] = {};

int main() {
	return tests::weighted_lottery();
}
