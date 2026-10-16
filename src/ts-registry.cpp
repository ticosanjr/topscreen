#include "ts-registry.hpp"

#include <algorithm>
#include <mutex>
#include <vector>

namespace topscreen {

namespace {

std::mutex &mutex()
{
	static std::mutex m;
	return m;
}

std::vector<FilterControl *> &items()
{
	static std::vector<FilterControl *> v;
	return v;
}

} // namespace

void registry_add(FilterControl *control)
{
	if (!control)
		return;
	std::lock_guard<std::mutex> lock(mutex());
	auto &v = items();
	if (std::find(v.begin(), v.end(), control) == v.end())
		v.push_back(control);
}

void registry_remove(FilterControl *control)
{
	std::lock_guard<std::mutex> lock(mutex());
	auto &v = items();
	v.erase(std::remove(v.begin(), v.end(), control), v.end());
}

FilterControl *registry_first()
{
	std::lock_guard<std::mutex> lock(mutex());
	auto &v = items();
	return v.empty() ? nullptr : v.front();
}

} // namespace topscreen
