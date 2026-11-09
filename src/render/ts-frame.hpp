#pragma once

#include "core/click_fx.hpp"
#include "core/cursor.hpp"
#include "core/highlighter.hpp"
#include "core/magnifier.hpp"
#include "core/spotlight.hpp"
#include "core/zoom.hpp"

namespace topscreen {

/** Per-frame state that the compositor consumes. */
struct FrameState {
	static constexpr int kMaxKeys = 8;

	core::CursorRender cursor;
	core::ZoomRender zoom;
	core::ClickRender clicks[core::ClickFxModel::kMaxSlots];
	int click_count = 0;
	core::KeyRender keys[kMaxKeys];
	int key_count = 0;
	core::HighlightRender highlight;
	core::SpotlightRender spotlight;
	core::MagnifierRender magnifier;
};

/** Maximum number of clicks sent to the shader (uniform limit). */
inline constexpr int kShaderClickSlots = 4;

/** Maximum number of freehand line points sent to the shader (uniform limit). */
inline constexpr int kShaderLineSlots = 32;

} // namespace topscreen
