#include "test_framework.hpp"

#include <exception>

int main()
{
	int failed = 0;
	const auto &reg = tst::registry();

	for (const auto &c : reg) {
		const int before = tst::failures();
		std::printf("[ RUN  ] %s\n", c.name.c_str());
		try {
			c.fn();
		} catch (const std::exception &e) {
			tst::fail(__FILE__, __LINE__, std::string("excecao: ") + e.what());
		} catch (...) {
			tst::fail(__FILE__, __LINE__, "excecao desconhecida");
		}
		if (tst::failures() > before) {
			++failed;
			std::printf("[ FAIL ] %s\n", c.name.c_str());
		} else {
			std::printf("[  OK  ] %s\n", c.name.c_str());
		}
	}

	std::printf("\nTotal: %zu testes | %d verificacoes | %d falhas\n", reg.size(), tst::checks(),
		    tst::failures());
	return failed == 0 ? 0 : 1;
}
