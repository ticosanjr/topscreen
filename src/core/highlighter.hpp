#pragma once

#include "types.hpp"

#include <cstdint>

namespace topscreen::core {

/* What a drag does. */
enum class HighlightMode : int {
	Box = 0,  // filled rounded box (original behaviour)
	Line = 1, // freehand line that fades out
	Zoom = 2, // no drawing: quick zoom into the selection, slow zoom back
};

struct HighlighterConfig {
	bool enabled = true;
	int mode = static_cast<int>(HighlightMode::Box);
	double hold_ms = 900.0; // time visible after releasing
	double fade_ms = 300.0; // fade duration
	uint32_t color = 0x6600E0FF; // 0xAABBGGRR (translucent yellow)
	double opacity = 0.35; // ceiling for the highlight alpha (0..1)
	double thickness = 0.004; // line mode: stroke width (content units 0..1)
	double min_span = 0.02;      // minimum drag (avoids marking clicks)

	/* Zoom mode: 0 in, 1 out, 2 punch in, 3 punch out. */
	int zoom_dir = 0;
	/* Zoom mode timings/limits. */
	double zoom_max = 2.5;
	double zoom_in_ms = 250.0;
	double zoom_hold_ms = 600.0;
	double zoom_out_ms = 1200.0;
};

/**
 * Only the box mode paints the rectangle; the line is an overlay and the zoom
 * gesture draws nothing. Keeping the rule here (instead of in the compositor)
 * makes it testable without a GPU.
 */
inline bool paints_box(const struct HighlightRender &r);

struct HighlightRender {
	bool active = false;
	int mode = 0;
	double x0 = 0.0, y0 = 0.0, x1 = 0.0, y1 = 0.0; // normalizado (x0<x1, y0<y1)
	double alpha = 0.0;
	uint32_t color = 0;
	double thickness = 0.0;
	/* Line mode: freehand points (content 0..1), valid until the next call. */
	const struct Vec2f *points = nullptr;
	int point_count = 0;
};

inline bool paints_box(const HighlightRender &r)
{
	return r.active && r.mode == 0;
}

/**
 * Drag highlighter.
 *
 * While the left button is pressed, it records the dragged region;
 * after releasing, it stays visible for `hold_ms` and fades out in `fade_ms`.
 */
class HighlighterModel {
public:
	void configure(const HighlighterConfig &cfg);

	void begin(double x, double y, double now);
	void move(double x, double y, double now);
	void end(double now);

	/** Zoom mode: consumes the pending "zoom into this rect" request. */
	bool take_zoom_request(double *x0, double *y0, double *x1, double *y1);
	void update(double now);

	HighlightRender render() const;

private:
	HighlighterConfig cfg_;
	bool dragging_ = false;
	bool has_ = false;
	double sx_ = 0.0, sy_ = 0.0;
	double ex_ = 0.0, ey_ = 0.0;
	double ended_ms_ = 0.0;
	double alpha_ = 0.0;

	/* Freehand trail (line mode). Fixed cap: no allocation while drawing. */
	static constexpr int kMaxPoints = 512;
	Vec2f trail_[kMaxPoints];
	int trail_count_ = 0;

	bool zoom_request_ = false;
	double zx0_ = 0.0, zy0_ = 0.0, zx1_ = 0.0, zy1_ = 0.0;
};

} // namespace topscreen::core
