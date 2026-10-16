#pragma once

// Micro-framework de testes (sem dependencias externas).

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace tst {

struct Case {
	std::string name;
	std::function<void()> fn;
};

inline std::vector<Case> &registry()
{
	static std::vector<Case> r;
	return r;
}

inline int &checks()
{
	static int c = 0;
	return c;
}

inline int &failures()
{
	static int f = 0;
	return f;
}

inline void fail(const char *file, int line, const std::string &msg)
{
	++failures();
	std::fprintf(stderr, "    %s:%d: %s\n", file, line, msg.c_str());
}

struct Registrar {
	Registrar(const char *name, std::function<void()> fn)
	{
		registry().push_back({name, std::move(fn)});
	}
};

} // namespace tst

#define TEST(name)                                                    \
	static void name();                                           \
	static ::tst::Registrar reg_##name(#name, name);              \
	static void name()

#define CHECK(cond)                                                                                \
	do {                                                                                       \
		++::tst::checks();                                                                 \
		if (!(cond))                                                                       \
			::tst::fail(__FILE__, __LINE__, "CHECK falhou: " #cond);                    \
	} while (0)

#define CHECK_NEAR(a, b, eps)                                                                      \
	do {                                                                                       \
		++::tst::checks();                                                                 \
		const double _a = static_cast<double>(a);                                           \
		const double _b = static_cast<double>(b);                                           \
		if (std::fabs(_a - _b) > (eps)) {                                                   \
			char _m[256];                                                               \
			std::snprintf(_m, sizeof(_m), "CHECK_NEAR: " #a "=%g vs " #b "=%g", _a,     \
				      _b);                                                          \
			::tst::fail(__FILE__, __LINE__, _m);                                        \
		}                                                                                  \
	} while (0)

#define CHECK_EQ(a, b)                                                                             \
	do {                                                                                       \
		++::tst::checks();                                                                 \
		if (!((a) == (b)))                                                                  \
			::tst::fail(__FILE__, __LINE__, "CHECK_EQ: " #a " vs " #b);                 \
	} while (0)

#define REQUIRE(cond)                                                                              \
	do {                                                                                       \
		++::tst::checks();                                                                 \
		if (!(cond)) {                                                                      \
			::tst::fail(__FILE__, __LINE__, "REQUIRE falhou: " #cond);                  \
			return;                                                                     \
		}                                                                                  \
	} while (0)
