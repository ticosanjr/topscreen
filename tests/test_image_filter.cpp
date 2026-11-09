#include "core/image_filter.hpp"
#include "test_framework.hpp"

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

using namespace topscreen::core;

namespace {

Image make_test_image(int w, int h)
{
	Image img;
	img.width = w;
	img.height = h;
	img.pixels.resize(static_cast<size_t>(w) * static_cast<size_t>(h) * 4u);
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			const size_t i = (static_cast<size_t>(y) * w + x) * 4;
			const bool left = x < w / 2;
			img.pixels[i] = left ? 255 : 0;
			img.pixels[i + 1] = left ? 0 : 255;
			img.pixels[i + 2] = 64;
			img.pixels[i + 3] = 255;
		}
	}
	return img;
}

} // namespace

TEST(image_filter_no_blur_returns_source_pixels)
{
	const Image src = make_test_image(8, 6);
	const Image out = image_prefilter(src, 8, 6, 0.0);
	CHECK_EQ(out.width, 8);
	CHECK_EQ(out.height, 6);
	CHECK(out.pixels == src.pixels);
}

TEST(image_filter_blur_preserves_dimensions_and_alpha)
{
	const Image src = make_test_image(1200, 600);
	const Image out = image_prefilter(src, 1200, 600, 8.0);
	CHECK_EQ(out.width, 600);
	CHECK_EQ(out.height, 300);
	REQUIRE(out.pixels.size() == out.width * out.height * 4u);
	CHECK_EQ(static_cast<int>(out.pixels[3]), 255);
	CHECK_EQ(static_cast<int>(out.pixels[out.pixels.size() - 1]), 255);

	/* The red/green edge must be softened by the filter. */
	const size_t center = ((out.height / 2) * out.width + out.width / 2) * 4;
	CHECK(out.pixels[center] > 0);
	CHECK(out.pixels[center + 1] > 0);
}

TEST(image_limit_max_keeps_aspect_and_caps_long_edge)
{
	const Image src = make_test_image(80, 40);
	const Image out = image_limit_max(src, 40);
	CHECK_EQ(out.width, 40);
	CHECK_EQ(out.height, 20);
	CHECK_EQ(out.pixels.size(), out.width * out.height * 4u);

	const Image unchanged = image_limit_max(src, 100);
	CHECK_EQ(unchanged.width, 80);
	CHECK_EQ(unchanged.height, 40);
}

TEST(image_filter_is_thread_safe)
{
	const Image src = make_test_image(1200, 600);
	std::atomic<int> failures{0};
	std::vector<std::thread> workers;
	for (int t = 0; t < 4; ++t) {
		workers.emplace_back([&src, &failures] {
			for (int i = 0; i < 2; ++i) {
				const Image out = image_prefilter(src, 1200, 600, 8.0);
				if (out.width != 600 || out.height != 300 ||
				    out.pixels.size() != 600u * 300u * 4u)
					++failures;
			}
		});
	}
	for (auto &worker : workers)
		worker.join();
	CHECK_EQ(failures.load(), 0);
}
