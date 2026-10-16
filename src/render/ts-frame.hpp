#pragma once

#include "core/click_fx.hpp"
#include "core/cursor.hpp"
#include "core/zoom.hpp"

namespace topscreen {

/** Estado calculado por frame que o compositor consome. */
struct FrameState {
	static constexpr int kMaxKeys = 8;

	core::CursorRender cursor;
	core::ZoomRender zoom;
	core::ClickRender clicks[core::ClickFxModel::kMaxSlots];
	int click_count = 0;
	core::KeyRender keys[kMaxKeys];
	int key_count = 0;
	double zoom_blur = 0.0; // 0..1 (motion blur do zoom)
};

/** Numero maximo de cliques enviados ao shader (limite dos uniforms). */
inline constexpr int kShaderClickSlots = 4;

} // namespace topscreen
