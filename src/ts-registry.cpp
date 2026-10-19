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

namespace {

std::vector<HotkeyContext *> &hotkey_targets()
{
	static std::vector<HotkeyContext *> v;
	return v;
}

} // namespace

void hotkey_target_add(HotkeyContext *target)
{
	if (!target)
		return;
	std::lock_guard<std::mutex> lock(mutex());
	auto &v = hotkey_targets();
	if (std::find(v.begin(), v.end(), target) == v.end())
		v.push_back(target);
}

void hotkey_target_remove(HotkeyContext *target)
{
	std::lock_guard<std::mutex> lock(mutex());
	auto &v = hotkey_targets();
	v.erase(std::remove(v.begin(), v.end(), target), v.end());
}

HotkeyContext *hotkey_target_first()
{
	std::lock_guard<std::mutex> lock(mutex());
	auto &v = hotkey_targets();
	return v.empty() ? nullptr : v.front();
}

} // namespace topscreen
