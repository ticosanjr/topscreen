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

FilterControl *&selected_control()
{
	static FilterControl *control = nullptr;
	return control;
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
	if (!selected_control())
		selected_control() = control;
}

void registry_remove(FilterControl *control)
{
	std::lock_guard<std::mutex> lock(mutex());
	auto &v = items();
	v.erase(std::remove(v.begin(), v.end(), control), v.end());
	if (selected_control() == control)
		selected_control() = nullptr;
}

std::vector<FilterEntry> registry_list()
{
	std::lock_guard<std::mutex> lock(mutex());
	std::vector<FilterEntry> out;
	auto &v = items();
	out.reserve(v.size());
	for (FilterControl *control : v)
		out.push_back({control, control ? control->source_name() : std::string()});
	return out;
}

bool registry_select(FilterControl *control)
{
	if (!control)
		return false;
	std::lock_guard<std::mutex> lock(mutex());
	auto &v = items();
	if (std::find(v.begin(), v.end(), control) == v.end())
		return false;
	selected_control() = control;
	return true;
}

FilterControl *registry_first()
{
	std::lock_guard<std::mutex> lock(mutex());
	auto &v = items();
	FilterControl *selected = selected_control();
	if (selected && std::find(v.begin(), v.end(), selected) != v.end())
		return selected;
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
	FilterControl *selected = selected_control();
	if (selected) {
		for (HotkeyContext *target : v) {
			if (target && target->filter_control() == selected)
				return target;
		}
	}
	return v.empty() ? nullptr : v.front();
}

} // namespace topscreen
