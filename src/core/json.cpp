#include "json.hpp"

#include <cctype>
#include <cstdlib>
#include <cstdio>

namespace topscreen::core::json {

namespace {

class Parser {
public:
	Parser(const std::string &text) : s_(text) {}

	bool run(Value *out, std::string *error)
	{
		skip_ws();
		if (!parse_value(out, error))
			return false;
		skip_ws();
		if (pos_ != s_.size()) {
			fail(error, "conteudo extra apos o valor");
			return false;
		}
		return true;
	}

private:
	void fail(std::string *error, const char *msg)
	{
		if (error) {
			char buf[128];
			std::snprintf(buf, sizeof(buf), "JSON: %s (posicao %zu)", msg, pos_);
			*error = buf;
		}
	}

	void skip_ws()
	{
		while (pos_ < s_.size()) {
			const char c = s_[pos_];
			if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
				++pos_;
			else
				break;
		}
	}

	bool peek(char *out) const
	{
		if (pos_ >= s_.size())
			return false;
		*out = s_[pos_];
		return true;
	}

	bool parse_value(Value *out, std::string *error)
	{
		char c;
		if (!peek(&c)) {
			fail(error, "valor inesperado no fim do texto");
			return false;
		}

		switch (c) {
		case '{':
			return parse_object(out, error);
		case '[':
			return parse_array(out, error);
		case '"': {
			out->type = Value::String;
			return parse_string(&out->str, error);
		}
		case 't':
		case 'f':
			return parse_bool(out, error);
		case 'n':
			return parse_null(out, error);
		default:
			return parse_number(out, error);
		}
	}

	bool parse_object(Value *out, std::string *error)
	{
		++pos_; // '{'
		out->type = Value::Object_;
		out->object = std::make_shared<Object>();
		skip_ws();

		char c;
		if (peek(&c) && c == '}') {
			++pos_;
			return true;
		}

		while (true) {
			skip_ws();
			if (!peek(&c) || c != '"') {
				fail(error, "esperava chave de objeto");
				return false;
			}
			std::string key;
			if (!parse_string(&key, error))
				return false;

			skip_ws();
			if (!peek(&c) || c != ':') {
				fail(error, "esperava ':'");
				return false;
			}
			++pos_;

			skip_ws();
			Value v;
			if (!parse_value(&v, error))
				return false;
			(*out->object)[key] = std::move(v);

			skip_ws();
			if (!peek(&c)) {
				fail(error, "objeto nao terminado");
				return false;
			}
			if (c == ',') {
				++pos_;
				continue;
			}
			if (c == '}') {
				++pos_;
				return true;
			}
			fail(error, "esperava ',' ou '}'");
			return false;
		}
	}

	bool parse_array(Value *out, std::string *error)
	{
		++pos_; // '['
		out->type = Value::Array_;
		out->array = std::make_shared<Array>();
		skip_ws();

		char c;
		if (peek(&c) && c == ']') {
			++pos_;
			return true;
		}

		while (true) {
			skip_ws();
			Value v;
			if (!parse_value(&v, error))
				return false;
			out->array->push_back(std::move(v));

			skip_ws();
			if (!peek(&c)) {
				fail(error, "array nao terminado");
				return false;
			}
			if (c == ',') {
				++pos_;
				continue;
			}
			if (c == ']') {
				++pos_;
				return true;
			}
			fail(error, "esperava ',' ou ']'");
			return false;
		}
	}

	bool parse_string(std::string *out, std::string *error)
	{
		++pos_; // '"'
		out->clear();
		while (pos_ < s_.size()) {
			const char c = s_[pos_++];
			if (c == '"')
				return true;
			if (c == '\\') {
				if (pos_ >= s_.size()) {
					fail(error, "escape incompleto");
					return false;
				}
				const char e = s_[pos_++];
				switch (e) {
				case '"':
					out->push_back('"');
					break;
				case '\\':
					out->push_back('\\');
					break;
				case '/':
					out->push_back('/');
					break;
				case 'b':
					out->push_back('\b');
					break;
				case 'f':
					out->push_back('\f');
					break;
				case 'n':
					out->push_back('\n');
					break;
				case 'r':
					out->push_back('\r');
					break;
				case 't':
					out->push_back('\t');
					break;
				case 'u': {
					/* Decodifica \uXXXX simples (BMP basic). */
					if (pos_ + 4 > s_.size()) {
						fail(error, "escape unicode incompleto");
						return false;
					}
					unsigned cp = 0;
					for (int i = 0; i < 4; ++i) {
						const char h = s_[pos_++];
						cp <<= 4;
						if (h >= '0' && h <= '9')
							cp |= static_cast<unsigned>(h - '0');
						else if (h >= 'a' && h <= 'f')
							cp |= static_cast<unsigned>(h - 'a' + 10);
						else if (h >= 'A' && h <= 'F')
							cp |= static_cast<unsigned>(h - 'A' + 10);
						else {
							fail(error, "hex invalido em \\u");
							return false;
						}
					}
					/* UTF-8 */
					if (cp < 0x80) {
						out->push_back(static_cast<char>(cp));
					} else if (cp < 0x800) {
						out->push_back(static_cast<char>(0xC0 | (cp >> 6)));
						out->push_back(static_cast<char>(0x80 | (cp & 0x3F)));
					} else {
						out->push_back(static_cast<char>(0xE0 | (cp >> 12)));
						out->push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
						out->push_back(static_cast<char>(0x80 | (cp & 0x3F)));
					}
					break;
				}
				default:
					fail(error, "escape invalido");
					return false;
				}
			} else {
				out->push_back(c);
			}
		}
		fail(error, "string nao terminada");
		return false;
	}

	bool parse_number(Value *out, std::string *error)
	{
		const size_t start = pos_;
		if (pos_ < s_.size() && (s_[pos_] == '-' || s_[pos_] == '+'))
			++pos_;
		while (pos_ < s_.size()) {
			const char c = s_[pos_];
			if ((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-')
				++pos_;
			else
				break;
		}
		if (pos_ == start) {
			fail(error, "numero invalido");
			return false;
		}
		const std::string token = s_.substr(start, pos_ - start);
		char *end = nullptr;
		const double v = std::strtod(token.c_str(), &end);
		if (end == token.c_str() || *end != '\0') {
			fail(error, "numero invalido");
			return false;
		}
		out->type = Value::Number;
		out->number = v;
		return true;
	}

	bool parse_bool(Value *out, std::string *error)
	{
		if (s_.compare(pos_, 4, "true") == 0) {
			pos_ += 4;
			out->type = Value::Bool;
			out->boolean = true;
			return true;
		}
		if (s_.compare(pos_, 5, "false") == 0) {
			pos_ += 5;
			out->type = Value::Bool;
			out->boolean = false;
			return true;
		}
		fail(error, "literal invalido");
		return false;
	}

	bool parse_null(Value *out, std::string *error)
	{
		if (s_.compare(pos_, 4, "null") == 0) {
			pos_ += 4;
			out->type = Value::Null;
			return true;
		}
		fail(error, "literal invalido");
		return false;
	}

	const std::string &s_;
	size_t pos_ = 0;
};

} // namespace

bool parse(const std::string &text, Value *out, std::string *error)
{
	Parser p(text);
	return p.run(out, error);
}

} // namespace topscreen::core::json
