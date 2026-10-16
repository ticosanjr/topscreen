#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace topscreen::core::json {

struct Value;

using Object = std::map<std::string, Value>;
using Array = std::vector<Value>;

/** Valor JSON minimo (sem dependencias externas). */
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

	/** Busca uma chave se este valor for um objeto. */
	const Value *find(const std::string &key) const
	{
		if (type != Object_ || !object)
			return nullptr;
		auto it = object->find(key);
		return it == object->end() ? nullptr : &it->second;
	}

	/** Acesso a string ("" se nao for string). */
	std::string as_string() const { return type == String ? str : std::string(); }
	/** Acesso a numero (0 se nao for numero). */
	double as_number() const { return type == Number ? number : 0.0; }
	/** Acesso a booleano (false se nao for bool). */
	bool as_bool() const { return type == Bool ? boolean : false; }
};

/** Faz o parse de um texto JSON. Retorna false e preenche `error` em caso de erro. */
bool parse(const std::string &text, Value *out, std::string *error);

} // namespace topscreen::core::json
