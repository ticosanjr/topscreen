#include "core/json.hpp"
#include "test_framework.hpp"

using namespace topscreen::core::json;

TEST(json_simple_object)
{
	Value v;
	std::string err;
	REQUIRE(parse("{\"a\": 1, \"b\": \"x\", \"c\": true, \"d\": null}", &v, &err));
	REQUIRE(v.is_object());

	const Value *a = v.find("a");
	REQUIRE(a != nullptr);
	CHECK(a->is_number());
	CHECK_NEAR(a->as_number(), 1.0, 1e-9);

	const Value *b = v.find("b");
	REQUIRE(b != nullptr);
	CHECK(b->is_string());
	CHECK_EQ(b->as_string(), std::string("x"));

	const Value *c = v.find("c");
	REQUIRE(c != nullptr);
	CHECK(c->is_bool());
	CHECK(c->as_bool());

	const Value *d = v.find("d");
	REQUIRE(d != nullptr);
	CHECK(d->is_null());
}

TEST(json_nested_and_array)
{
	Value v;
	std::string err;
	REQUIRE(parse("{\"bg\": {\"padding\": 64}, \"list\": [1, 2, 3]}", &v, &err));

	const Value *bg = v.find("bg");
	REQUIRE(bg != nullptr && bg->is_object());
	const Value *pad = bg->find("padding");
	REQUIRE(pad != nullptr);
	CHECK_NEAR(pad->as_number(), 64.0, 1e-9);

	const Value *list = v.find("list");
	REQUIRE(list != nullptr && list->is_array());
	CHECK_EQ(static_cast<int>(list->array->size()), 3);
	CHECK_NEAR((*list->array)[2].as_number(), 3.0, 1e-9);
}

TEST(json_negative_and_fractional_numbers)
{
	Value v;
	std::string err;
	REQUIRE(parse("[-1.5, 2e3, 0.25]", &v, &err));
	REQUIRE(v.is_array());
	CHECK_NEAR((*v.array)[0].as_number(), -1.5, 1e-9);
	CHECK_NEAR((*v.array)[1].as_number(), 2000.0, 1e-6);
	CHECK_NEAR((*v.array)[2].as_number(), 0.25, 1e-9);
}

TEST(json_errors)
{
	Value v;
	std::string err;
	CHECK(!parse("{\"a\": }", &v, &err));
	CHECK(!parse("[1, 2", &v, &err));
	CHECK(!parse("{\"a\":1} junk", &v, &err));
	CHECK(!parse("", &v, &err));
	CHECK(!err.empty());
}

TEST(json_escape_unicode)
{
	Value v;
	std::string err;
	REQUIRE(parse("{\"s\": \"\\u00e9\"}", &v, &err));
	const Value *s = v.find("s");
	REQUIRE(s != nullptr && s->is_string());
	CHECK_EQ(s->as_string(), std::string("\xc3\xa9")); // U+00E9 in UTF-8
}
