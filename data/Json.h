#pragma once

#include <map>
#include <string>
#include <vector>

namespace argmap {
namespace data {
namespace json {

// A deliberately small JSON reader/writer. Keeping it local means the project
// has no third-party dependency beyond FLTK and zlib.
class Value {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    Value() = default;

    static Value boolean(bool value);
    static Value number(double value);
    static Value string(const std::string& value);
    static Value array();
    static Value object();

    Type type() const { return type_; }
    bool isNull() const { return type_ == Type::Null; }
    bool asBool(bool fallback = false) const;
    double asNumber(double fallback = 0.0) const;
    int asInt(int fallback = 0) const;
    std::string asString(const std::string& fallback = "") const;
    const std::vector<Value>& asArray() const;
    const std::map<std::string, Value>& asObject() const;

    Value& operator[](const std::string& key);
    const Value& operator[](const std::string& key) const;
    Value& operator[](std::size_t index);
    void push_back(const Value& value);
    bool has(const std::string& key) const;

    std::string dump(int indent = 2) const;
    static bool parse(const std::string& text, Value* out, std::string* error);

private:
    Type type_ = Type::Null;
    bool boolValue_ = false;
    double numberValue_ = 0.0;
    std::string stringValue_;
    std::vector<Value> arrayValue_;
    std::map<std::string, Value> objectValue_;

    void dumpInto(std::string& out, int indent, int level) const;
};

} // namespace json
} // namespace data
} // namespace argmap
