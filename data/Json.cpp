#include "data/Json.h"

#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace argmap {
namespace data {
namespace json {
namespace {

std::string escapeString(const std::string& value) {
    std::string out = "\"";
    for (unsigned char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    std::ostringstream os;
                    os << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                       << static_cast<int>(c);
                    out += os.str();
                } else {
                    out.push_back(static_cast<char>(c));
                }
        }
    }
    out += '"';
    return out;
}

void appendUtf8(std::string& out, unsigned int codepoint) {
    if (codepoint <= 0x7F) {
        out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
}

class Parser {
public:
    Parser(const std::string& text, std::string* error) : text_(text), error_(error) {}

    bool parse(Value& out) {
        whitespace();
        if (!value(out)) return false;
        whitespace();
        if (pos_ != text_.size()) {
            return fail("unexpected trailing characters");
        }
        return true;
    }

private:
    const std::string& text_;
    std::string* error_;
    std::size_t pos_ = 0;

    bool fail(const std::string& message) {
        if (error_) {
            *error_ = "JSON error at byte " + std::to_string(pos_) + ": " + message;
        }
        return false;
    }

    void whitespace() {
        while (pos_ < text_.size() &&
               std::isspace(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }

    bool literal(const char* word) {
        const std::size_t length = std::char_traits<char>::length(word);
        if (text_.compare(pos_, length, word) != 0) {
            return false;
        }
        pos_ += length;
        return true;
    }

    bool value(Value& out) {
        whitespace();
        if (pos_ >= text_.size()) return fail("unexpected end of input");
        const char c = text_[pos_];
        if (c == '{') return object(out);
        if (c == '[') return array(out);
        if (c == '"') {
            std::string s;
            if (!string(s)) return false;
            out = Value::string(s);
            return true;
        }
        if (c == 't') {
            if (!literal("true")) return fail("invalid literal");
            out = Value::boolean(true);
            return true;
        }
        if (c == 'f') {
            if (!literal("false")) return fail("invalid literal");
            out = Value::boolean(false);
            return true;
        }
        if (c == 'n') {
            if (!literal("null")) return fail("invalid literal");
            out = Value();
            return true;
        }
        return number(out);
    }

    bool hex4(unsigned int& out) {
        if (pos_ + 4 > text_.size()) return false;
        out = 0;
        for (int i = 0; i < 4; ++i) {
            const unsigned char c = static_cast<unsigned char>(text_[pos_++]);
            unsigned int digit = 0;
            if (c >= '0' && c <= '9') digit = c - '0';
            else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
            else return false;
            out = out * 16 + digit;
        }
        return true;
    }

    bool string(std::string& out) {
        if (text_[pos_] != '"') return fail("expected string");
        ++pos_;
        out.clear();
        while (pos_ < text_.size()) {
            const unsigned char c = static_cast<unsigned char>(text_[pos_++]);
            if (c == '"') return true;
            if (c == '\\') {
                if (pos_ >= text_.size()) return fail("unterminated escape");
                const char e = text_[pos_++];
                switch (e) {
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case 'u': {
                        unsigned int codepoint = 0;
                        if (!hex4(codepoint)) return fail("invalid unicode escape");
                        if (codepoint >= 0xD800 && codepoint <= 0xDBFF &&
                            pos_ + 6 <= text_.size() && text_[pos_] == '\\' &&
                            text_[pos_ + 1] == 'u') {
                            pos_ += 2;
                            unsigned int low = 0;
                            if (!hex4(low) || low < 0xDC00 || low > 0xDFFF) {
                                return fail("invalid surrogate pair");
                            }
                            codepoint = 0x10000 + ((codepoint - 0xD800) << 10) +
                                        (low - 0xDC00);
                        }
                        appendUtf8(out, codepoint);
                        break;
                    }
                    default: return fail("unknown escape sequence");
                }
            } else if (c < 0x20) {
                return fail("control character inside string");
            } else {
                out.push_back(static_cast<char>(c));
            }
        }
        return fail("unterminated string");
    }

    bool number(Value& out) {
        const std::size_t start = pos_;
        if (pos_ < text_.size() && (text_[pos_] == '-' || text_[pos_] == '+')) ++pos_;
        bool digits = false;
        while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
            digits = true;
        }
        if (pos_ < text_.size() && text_[pos_] == '.') {
            ++pos_;
            while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
                ++pos_;
                digits = true;
            }
        }
        if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < text_.size() && (text_[pos_] == '-' || text_[pos_] == '+')) ++pos_;
            while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
                ++pos_;
            }
        }
        if (!digits) return fail("expected number");
        out = Value::number(std::strtod(text_.substr(start, pos_ - start).c_str(), nullptr));
        return true;
    }

    bool array(Value& out) {
        ++pos_;
        out = Value::array();
        whitespace();
        if (pos_ < text_.size() && text_[pos_] == ']') {
            ++pos_;
            return true;
        }
        while (true) {
            Value item;
            if (!value(item)) return false;
            out.push_back(item);
            whitespace();
            if (pos_ >= text_.size()) return fail("unterminated array");
            if (text_[pos_] == ']') {
                ++pos_;
                return true;
            }
            if (text_[pos_] != ',') return fail("expected ',' or ']'");
            ++pos_;
        }
    }

    bool object(Value& out) {
        ++pos_;
        out = Value::object();
        whitespace();
        if (pos_ < text_.size() && text_[pos_] == '}') {
            ++pos_;
            return true;
        }
        while (true) {
            whitespace();
            std::string key;
            if (pos_ >= text_.size() || text_[pos_] != '"') return fail("expected object key");
            if (!string(key)) return false;
            whitespace();
            if (pos_ >= text_.size() || text_[pos_] != ':') return fail("expected ':'");
            ++pos_;
            Value item;
            if (!value(item)) return false;
            out[key] = item;
            whitespace();
            if (pos_ >= text_.size()) return fail("unterminated object");
            if (text_[pos_] == '}') {
                ++pos_;
                return true;
            }
            if (text_[pos_] != ',') return fail("expected ',' or '}'");
            ++pos_;
        }
    }
};

} // namespace

Value Value::boolean(bool value) {
    Value v;
    v.type_ = Type::Bool;
    v.boolValue_ = value;
    return v;
}

Value Value::number(double value) {
    Value v;
    v.type_ = Type::Number;
    v.numberValue_ = value;
    return v;
}

Value Value::string(const std::string& value) {
    Value v;
    v.type_ = Type::String;
    v.stringValue_ = value;
    return v;
}

Value Value::array() {
    Value v;
    v.type_ = Type::Array;
    return v;
}

Value Value::object() {
    Value v;
    v.type_ = Type::Object;
    return v;
}

bool Value::asBool(bool fallback) const {
    return type_ == Type::Bool ? boolValue_ : fallback;
}

double Value::asNumber(double fallback) const {
    return type_ == Type::Number ? numberValue_ : fallback;
}

int Value::asInt(int fallback) const {
    return type_ == Type::Number ? static_cast<int>(numberValue_) : fallback;
}

std::string Value::asString(const std::string& fallback) const {
    return type_ == Type::String ? stringValue_ : fallback;
}

const std::vector<Value>& Value::asArray() const {
    static const std::vector<Value> empty;
    return type_ == Type::Array ? arrayValue_ : empty;
}

const std::map<std::string, Value>& Value::asObject() const {
    static const std::map<std::string, Value> empty;
    return type_ == Type::Object ? objectValue_ : empty;
}

Value& Value::operator[](const std::string& key) {
    type_ = Type::Object;
    return objectValue_[key];
}

const Value& Value::operator[](const std::string& key) const {
    static const Value missing;
    if (type_ != Type::Object) return missing;
    const auto it = objectValue_.find(key);
    return it == objectValue_.end() ? missing : it->second;
}

Value& Value::operator[](std::size_t index) {
    type_ = Type::Array;
    if (arrayValue_.size() <= index) {
        arrayValue_.resize(index + 1);
    }
    return arrayValue_[index];
}

void Value::push_back(const Value& value) {
    type_ = Type::Array;
    arrayValue_.push_back(value);
}

bool Value::has(const std::string& key) const {
    return type_ == Type::Object && objectValue_.find(key) != objectValue_.end();
}

void Value::dumpInto(std::string& out, int indent, int level) const {
    const std::string pad(static_cast<std::size_t>(indent) * static_cast<std::size_t>(level), ' ');
    const std::string childPad(static_cast<std::size_t>(indent) * static_cast<std::size_t>(level + 1), ' ');
    switch (type_) {
        case Type::Null:
            out += "null";
            break;
        case Type::Bool:
            out += boolValue_ ? "true" : "false";
            break;
        case Type::Number: {
            std::ostringstream os;
            os << std::setprecision(10) << numberValue_;
            out += os.str();
            break;
        }
        case Type::String:
            out += escapeString(stringValue_);
            break;
        case Type::Array:
            if (arrayValue_.empty()) {
                out += "[]";
                break;
            }
            out += "[\n";
            for (std::size_t i = 0; i < arrayValue_.size(); ++i) {
                out += childPad;
                arrayValue_[i].dumpInto(out, indent, level + 1);
                if (i + 1 != arrayValue_.size()) out += ',';
                out += '\n';
            }
            out += pad + "]";
            break;
        case Type::Object:
            if (objectValue_.empty()) {
                out += "{}";
                break;
            }
            out += "{\n";
            std::size_t index = 0;
            for (const auto& entry : objectValue_) {
                out += childPad + escapeString(entry.first) + ": ";
                entry.second.dumpInto(out, indent, level + 1);
                if (++index != objectValue_.size()) out += ',';
                out += '\n';
            }
            out += pad + "}";
            break;
    }
}

std::string Value::dump(int indent) const {
    std::string out;
    dumpInto(out, indent, 0);
    return out;
}

bool Value::parse(const std::string& text, Value* out, std::string* error) {
    if (!out) {
        if (error) *error = "JSON parse output pointer is null";
        return false;
    }
    Parser parser(text, error);
    return parser.parse(*out);
}

} // namespace json
} // namespace data
} // namespace argmap
