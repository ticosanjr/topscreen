#pragma once

#include "types.hpp"

#include <cstdint>

namespace topscreen::core {

/* What a drag does. */
enum class HighlightMode : int {
	Box = 0,  // filled rounded box (original behaviour)
	Line = 1, // freehand line that fades out
	Zoom = 2, // no drawing: hold to zoom, release to glide back
	BoxZoom = 3, // draws the box AND zooms while held
	LineZoom = 4, // draws the freehand line AND zooms while held
};

struct HighlighterConfig {
	bool enabled = true;
	/** Extra, independent of the drag modes: a hotkey toggles a zoom in/out. */
	bool hotkey_enabled = true;
	int mode = static_cast<int>(HighlightMode::Box);
	double hold_ms = 900.0; // time visible after releasing
	double fade_ms = 300.0; // fade duration
	uint32_t color = 0x6600E0FF; // 0xAABBGGRR (translucent yellow)
	double opacity = 0.35; // ceiling for the highlight alpha (0..1)
	double thickness = 0.004; // line mode: stroke width (content units 0..1)
	double min_span = 0.02;      // minimum drag (avoids marking clicks)

	/* Zoom mode timings/limits. */
	double zoom_max = 2.5;
	double zoom_delay_ms = 1200.0; // hold this long before the zoom kicks in
	double zoom_in_ms = 250.0;
	double zoom_hold_ms = 600.0;
	double zoom_out_ms = 1200.0;
};

/**
 * Only the box modes paint the rectangle; the line is drawn as shader
 * segments and the pure zoom gesture draws nothing. Keeping the rules here
 * (instead of in the compositor) makes them testable without a GPU.
 */
inline bool paints_box(const struct HighlightRender &r);
inline bool draws_line(const struct HighlightRender &r);

/** Modes that record and draw a freehand stroke. */
inline bool mode_draws_line(int mode)
{
	return mode == static_cast<int>(HighlightMode::Line) ||
	       mode == static_cast<int>(HighlightMode::LineZoom);
}

/** Modes that run the hold-to-zoom gesture (the filter animates the zoom). */
inline bool mode_wants_zoom(int mode)
{
	return mode == static_cast<int>(HighlightMode::Zoom) ||
	       mode == static_cast<int>(HighlightMode::BoxZoom) ||
	       mode == static_cast<int>(HighlightMode::LineZoom);
}

/** Modes that leave a visible mark (a box or a line). The pure zoom mode
 * draws nothing at all: it only moves the view. */
inline bool mode_draws_mark(int mode)
{
	return mode == static_cast<int>(HighlightMode::Box) ||
	       mode == static_cast<int>(HighlightMode::Line) ||
	       mode == static_cast<int>(HighlightMode::BoxZoom) ||
	       mode == static_cast<int>(HighlightMode::LineZoom);
}

struct HighlightRender {
	bool active = false;
	int mode = 0;
	double x0 = 0.0, y0 = 0.0, x1 = 0.0, y1 = 0.0; // normalized (x0<x1, y0<y1)
	double alpha = 0.0;
	uint32_t color = 0;
	double thickness = 0.0;
	/* Line modes: freehand points (content 0..1), valid until the next call. */
	const struct Vec2f *points = nullptr;
	int point_count = 0;
};

/* Only the box modes paint the rectangle: the line is drawn by the main
 * shader and the pure zoom gesture draws nothing. Regression for "Line still
 * shows the filled box". */
inline bool paints_box(const HighlightRender &r)
{
	return r.active && (r.mode == static_cast<int>(HighlightMode::Box) ||
			    r.mode == static_cast<int>(HighlightMode::BoxZoom));
}

/* True when the rendered mark includes the freehand line. */
inline bool draws_line(const HighlightRender &r)
{
	return r.active && mode_draws_line(r.mode);
}

/**
 * Decimates a freehand trail into at most `max_out` points, keeping the first
 * and the last and spreading the rest evenly (the shader declares a fixed
 * number of line slots). Returns how many points were written to `out`.
 */
int subsample_line(const struct Vec2f *points, int count, struct Vec2f *out, int max_out);


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
	/* Selection the zoom gesture uses (stroke bounding box for line modes,
	 * the dragged rectangle otherwise). */
	void selection_rect(double *x0, double *y0, double *x1, double *y1) const;

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
	bool zoom_fired_ = false;
	double press_ms_ = 0.0;
	double zx0_ = 0.0, zy0_ = 0.0, zx1_ = 0.0, zy1_ = 0.0;
};

} // namespace topscreen::core
