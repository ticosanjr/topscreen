#include "ts-parent-cursor.hpp"

#include "capture/ts-capture-source.hpp"

#include <cstring>
#include <map>
#include <mutex>

namespace topscreen::parent_cursor {

namespace {

/* Native cursor ownership is shared by every TopScreen filter attached to the
 * same capture. Each filter contributes one "hide" want while effects and its
 * own cursor is enabled; the capture is restored only when the last want goes
 * away. This avoids one filter restoring the cursor while another still draws
 * its own. */
struct CursorEntry {
	int users = 0;
	int hidden_wants = 0;
	bool has[2] = {false, false};
	bool was[2] = {false, false};
	bool logged_missing = false;
};

std::mutex cursor_mutex;
std::map<obs_source_t *, CursorEntry> cursor_entries;

const char *const kCursorKeys[] = {"capture_cursor", "show_cursor"};

void capture_cursor_state(obs_source_t *parent, CursorEntry *entry)
{
	obs_properties_t *props = obs_source_properties(parent);
	if (!props)
		return;
	for (int i = 0; i < 2; ++i)
		entry->has[i] = obs_properties_get(props, kCursorKeys[i]) != nullptr;
	obs_properties_destroy(props);

	obs_data_t *s = obs_source_get_settings(parent);
	if (s) {
		for (int i = 0; i < 2; ++i) {
			if (entry->has[i])
				entry->was[i] = obs_data_get_bool(s, kCursorKeys[i]);
		}
		obs_data_release(s);
	}

	if (!entry->has[0] && !entry->has[1] && !entry->logged_missing) {
		entry->logged_missing = true;
		blog(LOG_INFO, "[TopScreen] parent source does not expose a native cursor option (%s)",
		     obs_source_get_name(parent) ? obs_source_get_name(parent) : "?");
	}
}

void apply_cursor_state(obs_source_t *parent, CursorEntry *entry)
{
	const bool hide = entry->hidden_wants > 0;

	/* TopScreen Capture keeps the override in memory: changing its persisted
	 * obs_data would save "cursor off" into the scene while the filter lives. */
	const char *id = obs_source_get_id(parent);
	if (id && std::strcmp(id, "topscreen_capture") == 0) {
		set_native_cursor(parent, !hide);
		return;
	}

	obs_data_t *s = obs_source_get_settings(parent);
	if (!s)
		return;
	bool changed = false;
	for (int i = 0; i < 2; ++i) {
		if (!entry->has[i])
			continue;
		const bool want = hide ? false : entry->was[i];
		const bool current = obs_data_get_bool(s, kCursorKeys[i]);
		if (current == want)
			continue;
		obs_data_set_bool(s, kCursorKeys[i], want);
		changed = true;
		blog(LOG_INFO, "[TopScreen] capture native cursor %s (%s, was %s)",
		     want ? "restored" : "disabled", kCursorKeys[i], current ? "on" : "off");
	}
	if (changed)
		obs_source_update(parent, s);
	obs_data_release(s);
}

void acquire_impl(obs_source_t *parent)
{
	std::lock_guard<std::mutex> lock(cursor_mutex);
	CursorEntry &entry = cursor_entries[parent];
	if (entry.users++ == 0)
		capture_cursor_state(parent, &entry);
}

void set_wanted_impl(obs_source_t *parent, bool hide)
{
	std::lock_guard<std::mutex> lock(cursor_mutex);
	auto it = cursor_entries.find(parent);
	if (it == cursor_entries.end())
		return;
	CursorEntry &entry = it->second;
	if (hide)
		++entry.hidden_wants;
	else if (entry.hidden_wants > 0)
		--entry.hidden_wants;
	apply_cursor_state(parent, &entry);
}

void release_impl(obs_source_t *parent, bool was_hiding)
{
	std::lock_guard<std::mutex> lock(cursor_mutex);
	auto it = cursor_entries.find(parent);
	if (it == cursor_entries.end())
		return;
	CursorEntry &entry = it->second;
	if (was_hiding && entry.hidden_wants > 0)
		--entry.hidden_wants;
	if (entry.users > 0)
		--entry.users;
	if (entry.users == 0) {
		entry.hidden_wants = 0;
		apply_cursor_state(parent, &entry);
		cursor_entries.erase(it);
	}
}

} // namespace

void acquire(obs_source_t *parent)
{
	acquire_impl(parent);
}

void set_wanted(obs_source_t *parent, bool hide)
{
	set_wanted_impl(parent, hide);
}

void release(obs_source_t *parent, bool was_hiding)
{
	release_impl(parent, was_hiding);
}

} // namespace topscreen::parent_cursor
