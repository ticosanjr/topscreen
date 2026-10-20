#include "json.hpp"

#include <cctype>
#include <cstdlib>
#include <cstdio>

namespace topscreen::core::json {

namespace {

struct DepthGuard {
	int &depth;
	explicit DepthGuard(int &d) : depth(d) { ++depth; }
	~DepthGuard() { --depth; }
};

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
			fail(error, "extra content after the value");
			return false;
		}
		return true;
	}

private:
	void fail(std::string *error, const char *msg)
	{
		if (error) {
			char buf[128];
			std::snprintf(buf, sizeof(buf), "JSON: %s (position %zu)", msg, pos_);
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
			fail(error, "unexpected value at end of text");
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
		if (depth_ >= kMaxDepth) {
			fail(error, "nesting too deep");
			return false;
		}
		DepthGuard guard(depth_);
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
				fail(error, "expected object key");
				return false;
			}
			std::string key;
			if (!parse_string(&key, error))
				return false;

			skip_ws();
			if (!peek(&c) || c != ':') {
				fail(error, "expected ':'");
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
				fail(error, "unterminated object");
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
			fail(error, "expected ',' or '}'");
			return false;
		}
	}

	bool parse_array(Value *out, std::string *error)
	{
		if (depth_ >= kMaxDepth) {
			fail(error, "nesting too deep");
			return false;
		}
		DepthGuard guard(depth_);
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
				fail(error, "unterminated array");
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
			fail(error, "expected ',' or ']'");
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
					fail(error, "incomplete escape");
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
					/* Decodes simple \uXXXX (basic BMP). */
					if (pos_ + 4 > s_.size()) {
						fail(error, "incomplete unicode escape");
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
							fail(error, "invalid hex in \\u");
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
					fail(error, "invalid escape");
					return false;
				}
			} else {
				out->push_back(c);
			}
		}
		fail(error, "unterminated string");
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
			fail(error, "invalid number");
			return false;
		}
		const std::string token = s_.substr(start, pos_ - start);
		char *end = nullptr;
		const double v = std::strtod(token.c_str(), &end);
		if (end == token.c_str() || *end != '\0') {
			fail(error, "invalid number");
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
		fail(error, "invalid literal");
		return false;
	}

	bool parse_null(Value *out, std::string *error)
	{
		if (s_.compare(pos_, 4, "null") == 0) {
			pos_ += 4;
			out->type = Value::Null;
			return true;
		}
		fail(error, "invalid literal");
		return false;
	}

	const std::string &s_;
	size_t pos_ = 0;
	int depth_ = 0;
};

} // namespace

bool parse(const std::string &text, Value *out, std::string *error)
{
	if (!out) {
		if (error)
			*error = "JSON: null output";
		return false;
	}
	if (text.size() > kMaxBytes) {
		if (error)
			*error = "JSON: input too large";
		return false;
	}
	Parser p(text);
	return p.run(out, error);
}

} // namespace topscreen::core::json
