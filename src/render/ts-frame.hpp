#pragma once

#include "core/click_fx.hpp"
#include "core/cursor.hpp"
#include "core/highlighter.hpp"
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
	double zoom_blur = 0.0; // 0..1 (zoom motion blur)
	core::HighlightRender highlight;
};

/** Maximum number of clicks sent to the shader (uniform limit). */
inline constexpr int kShaderClickSlots = 4;

} // namespace topscreen
