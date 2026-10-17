#include <obs-module.h>

#include "ts-filter.hpp"

#ifdef TOPSCREEN_HAVE_QT
#include "ui/ts-dock.hpp"
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("topscreen", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "TopScreen - live screen capture effects for OBS";
}

bool obs_module_load(void)
{
	obs_register_source(&ts_filter_source_info);

#ifdef TOPSCREEN_HAVE_QT
	topscreen::ts_dock_register();
#endif

	blog(LOG_INFO, "[TopScreen] loaded v" TOPSCREEN_VERSION);
	return true;
}

void obs_module_unload(void)
{
#ifdef TOPSCREEN_HAVE_QT
	topscreen::ts_dock_unregister();
#endif
}
