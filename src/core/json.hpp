#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace topscreen::core::json {

struct Value;

using Object = std::map<std::string, Value>;
using Array = std::vector<Value>;

/** Minimal JSON value (no external dependencies). */
struct Value {
	enum Type { Null, Bool, Number, String, Object_, Array_ };

	Type type = Null;
	bool boolean = false;
	double number = 0.0;
	std::string str;
	std::shared_ptr<Object> object;
	std::shared_ptr<Array> array;

	bool is_null() const { return type == Null; }
	bool is_bool() const { return type == Bool; }
	bool is_number() const { return type == Number; }
	bool is_string() const { return type == String; }
	bool is_object() const { return type == Object_; }
	bool is_array() const { return type == Array_; }

	/** Looks up a key if this value is an object. */
	const Value *find(const std::string &key) const
	{
		if (type != Object_ || !object)
			return nullptr;
		auto it = object->find(key);
		return it == object->end() ? nullptr : &it->second;
	}

	/** String access ("" if not a string). */
	std::string as_string() const { return type == String ? str : std::string(); }
	/** Number access (0 if not a number). */
	double as_number() const { return type == Number ? number : 0.0; }
	/** Boolean access (false if not a bool). */
	bool as_bool() const { return type == Bool ? boolean : false; }
};

/** Parses a JSON text. Returns false and fills `error` on failure. */
bool parse(const std::string &text, Value *out, std::string *error);

} // namespace topscreen::core::json
