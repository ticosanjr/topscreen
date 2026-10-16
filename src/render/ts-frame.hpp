#pragma once

#include "core/click_fx.hpp"
#include "core/cursor.hpp"
#include "core/zoom.hpp"

namespace topscreen {

/** Estado calculado por frame que o compositor consome. */
struct FrameState {
	core::CursorRender cursor;
	core::ZoomRender zoom;
	core::ClickRender clicks[core::ClickFxModel::kMaxSlots];
	int click_count = 0;
};

/** Numero maximo de cliques enviados ao shader (limite dos uniforms). */
inline constexpr int kShaderClickSlots = 4;

} // namespace topscreen
