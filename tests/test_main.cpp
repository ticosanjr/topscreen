#include "test_framework.hpp"

#include <exception>

int main()
{
	int failed = 0;
	const auto &reg = tst::registry();

	for (const auto &c : reg) {
		const int before = tst::failures();
		std::printf("[ RUN  ] %s\n", c.name.c_str());
		std::fflush(stdout); // flushed: a crash still shows which test died
		try {
			c.fn();
		} catch (const std::exception &e) {
			tst::fail(__FILE__, __LINE__, std::string("exception: ") + e.what());
		} catch (...) {
			tst::fail(__FILE__, __LINE__, "unknown exception");
		}
		if (tst::failures() > before) {
			++failed;
			std::printf("[ FAIL ] %s\n", c.name.c_str());
		} else {
			std::printf("[  OK  ] %s\n", c.name.c_str());
		}
		std::fflush(stdout);
	}

	std::printf("\nTotal: %zu tests | %d checks | %d failures\n", reg.size(), tst::checks(),
		    tst::failures());
	return failed == 0 ? 0 : 1;
}
