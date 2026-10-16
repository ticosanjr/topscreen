#include "core/evdev.hpp"
#include "test_framework.hpp"

#include <cstdint>
#include <vector>

using namespace topscreen::core;

namespace {

/* Monta um registro de input_event (24 bytes) com type/code/value. */
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

TEST(evdev_decode_press_e_release)
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

TEST(evdev_decode_ignora_tipos_nao_tecla)
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

TEST(evdev_decode_registro_parcial_ignorado)
{
	std::vector<uint8_t> data(24 + 10, 0); /* 1 registro + 10 bytes soltos */
	data[16] = 1;
	data[18] = 30;
	data[20] = 1;

	std::vector<EvdevKey> keys;
	REQUIRE(evdev_decode(data.data(), data.size(), kEvdevRecord64, &keys));
	CHECK_EQ(static_cast<int>(keys.size()), 1);
}

TEST(evdev_decode_sem_dados)
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

TEST(evdev_modificadores)
{
	CHECK(evdev_key_is_modifier(29));  /* LeftCtrl */
	CHECK(evdev_key_is_modifier(42));  /* LeftShift */
	CHECK(evdev_key_is_modifier(125)); /* LeftMeta */
	CHECK(!evdev_key_is_modifier(30)); /* A */
	CHECK(!evdev_key_is_modifier(57)); /* Space */
}
