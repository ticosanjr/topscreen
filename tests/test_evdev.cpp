#include "core/evdev.hpp"
#include "test_framework.hpp"

#include <cstdint>
#include <vector>

using namespace topscreen::core;

namespace {

/* Builds an input_event record (24 bytes) with type/code/value. */
std::vector<uint8_t> record(uint16_t type, uint16_t code, int32_t value)
{
	std::vector<uint8_t> r(24, 0);
	r[16] = static_cast<uint8_t>(type & 0xFF);
	r[17] = static_cast<uint8_t>((type >> 8) & 0xFF);
	r[18] = static_cast<uint8_t>(code & 0xFF);
	r[19] = static_cast<uint8_t>((code >> 8) & 0xFF);
	r[20] = static_cast<uint8_t>(value & 0xFF);
	r[21] = static_cast<uint8_t>((value >> 8) & 0xFF);
	r[22] = static_cast<uint8_t>((value >> 16) & 0xFF);
	r[23] = static_cast<uint8_t>((value >> 24) & 0xFF);
	return r;
}

std::vector<uint8_t> concat(std::initializer_list<std::vector<uint8_t>> parts)
{
	std::vector<uint8_t> out;
	for (const auto &p : parts)
		out.insert(out.end(), p.begin(), p.end());
	return out;
}

} // namespace

TEST(evdev_decode_press_and_release)
{
	const auto data = concat({record(1, 30, 1),  /* KEY_A press */
				  record(1, 30, 0)}); /* KEY_A release */

	std::vector<EvdevKey> keys;
	REQUIRE(evdev_decode(data.data(), data.size(), kEvdevRecord64, &keys));
	REQUIRE(keys.size() == 2);
	CHECK_EQ(keys[0].code, 30);
	CHECK(keys[0].pressed);
	CHECK_EQ(keys[1].code, 30);
	CHECK(!keys[1].pressed);
}

TEST(evdev_decode_ignores_non_key_types)
{
	const auto data = concat({
		record(0, 0, 0),    /* EV_SYN */
		record(4, 4, 1),    /* EV_MSC */
		record(1, 42, 1),   /* KEY_LEFTSHIFT */
	});

	std::vector<EvdevKey> keys;
	REQUIRE(evdev_decode(data.data(), data.size(), kEvdevRecord64, &keys));
	REQUIRE(keys.size() == 1);
	CHECK_EQ(keys[0].code, 42);
	CHECK(keys[0].pressed);
}

TEST(evdev_decode_partial_record_ignored)
{
	std::vector<uint8_t> data(24 + 10, 0); /* 1 record + 10 loose bytes */
	data[16] = 1;
	data[18] = 30;
	data[20] = 1;

	std::vector<EvdevKey> keys;
	REQUIRE(evdev_decode(data.data(), data.size(), kEvdevRecord64, &keys));
	CHECK_EQ(static_cast<int>(keys.size()), 1);
}

TEST(evdev_decode_no_data)
{
	std::vector<EvdevKey> keys;
	CHECK(!evdev_decode(nullptr, 0, kEvdevRecord64, &keys));
	std::vector<uint8_t> vazio;
	CHECK(!evdev_decode(vazio.data(), 0, kEvdevRecord64, &keys));
}

TEST(evdev_labels)
{
	CHECK_EQ(evdev_key_label(30), std::string("A"));
	CHECK_EQ(evdev_key_label(11), std::string("0"));
	CHECK_EQ(evdev_key_label(57), std::string("Space"));
	CHECK_EQ(evdev_key_label(28), std::string("Enter"));
	CHECK_EQ(evdev_key_label(29), std::string("Ctrl"));
	CHECK_EQ(evdev_key_label(999), std::string(""));
}

TEST(evdev_shifted_punctuation)
{
	CHECK_EQ(evdev_key_label_with_shift(2, true), std::string("!"));  /* 1 */
	CHECK_EQ(evdev_key_label_with_shift(3, true), std::string("@"));  /* 2 */
	CHECK_EQ(evdev_key_label_with_shift(4, true), std::string("#"));  /* 3 */
	CHECK_EQ(evdev_key_label_with_shift(5, true), std::string("$"));  /* 4 */
	CHECK_EQ(evdev_key_label_with_shift(6, true), std::string("%"));  /* 5 */
	CHECK_EQ(evdev_key_label_with_shift(7, true), std::string("^"));  /* 6 */
	CHECK_EQ(evdev_key_label_with_shift(8, true), std::string("&"));  /* 7 */
	CHECK_EQ(evdev_key_label_with_shift(9, true), std::string("*"));  /* 8 */
	CHECK_EQ(evdev_key_label_with_shift(10, true), std::string("(")); /* 9 */
	CHECK_EQ(evdev_key_label_with_shift(11, true), std::string(")")); /* 0 */
	CHECK_EQ(evdev_key_label_with_shift(12, false), std::string("-"));
	CHECK_EQ(evdev_key_label_with_shift(12, true), std::string("_"));
	CHECK_EQ(evdev_key_label_with_shift(13, true), std::string("+"));
	CHECK_EQ(evdev_key_label_with_shift(26, true), std::string("{"));
	CHECK_EQ(evdev_key_label_with_shift(39, true), std::string(":"));
	CHECK_EQ(evdev_key_label_with_shift(53, true), std::string("?"));
	CHECK_EQ(evdev_key_label_with_shift(30, false), std::string("A"));
	CHECK_EQ(evdev_key_label_with_shift(57, false), std::string("Space"));
}

TEST(evdev_modifiers)
{
	CHECK(evdev_key_is_modifier(29));  /* LeftCtrl */
	CHECK(evdev_key_is_modifier(42));  /* LeftShift */
	CHECK(evdev_key_is_modifier(125)); /* LeftMeta */
	CHECK(!evdev_key_is_modifier(30)); /* A */
	CHECK(!evdev_key_is_modifier(57)); /* Space */
}
