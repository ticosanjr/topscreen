#pragma once

namespace topscreen {

/** Registers/unregisters the TopScreen dock in OBS (no-op without Qt). */
void ts_dock_register();
void ts_dock_unregister();

} // namespace topscreen
