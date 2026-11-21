#include "data/JsonMapRepository.h"

#include <fstream>
#include <sstream>

#include "data/Json.h"

namespace argmap {
namespace data {
namespace {

using json::Value;

bool readFile(const std::string& path, std::string* contents, std::string* error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        if (error) *error = "Could not open '" + path + "' for reading.";
        return false;
    }
    std::ostringstream stream;
    stream << file.rdbuf();
    *contents = stream.str();
    return true;
}

bool writeFile(const std::string& path, const std::string& contents, std::string* error) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        if (error) *error = "Could not open '" + path + "' for writing.";
        return false;
    }
    file << contents;
    if (!file) {
        if (error) *error = "Failed while writing '" + path + "'.";
        return false;
    }
    return true;
}

} // namespace

bool JsonMapRepository::save(const domain::DebateMap& map, const std::string& path,
                             std::string* error) {
    Value root = Value::object();
    root["format"] = Value::string("argmap/1");
    root["title"] = Value::string(map.title);

    Value arguments = Value::array();
    for (const domain::Argument& arg : map.arguments) {
        Value item = Value::object();
        item["id"] = Value::string(arg.id);
        item["text"] = Value::string(arg.text);
        item["side"] = Value::string(arg.side == domain::Side::Proposition
                                         ? "proposition" : "opposition");
        item["weight"] = Value::number(arg.weight);
        item["color"] = Value::string(arg.color);
        item["speaker"] = Value::number(domain::speakerValue(arg.speaker));
        item["x"] = Value::number(arg.x);
        item["y"] = Value::number(arg.y);
        arguments.push_back(item);
    }
    root["arguments"] = arguments;

    Value counters = Value::array();
    for (const domain::CounterLink& link : map.counters) {
        Value item = Value::object();
        item["id"] = Value::string(link.id);
        item["counter"] = Value::string(link.counterId);
        item["target"] = Value::string(link.targetId);
        counters.push_back(item);
    }
    root["counters"] = counters;
    root["nextId"] = Value::number(map.nextId);

    return writeFile(path, root.dump(2) + "\n", error);
}

bool JsonMapRepository::load(const std::string& path, domain::DebateMap* map,
                             std::string* error) {
    if (!map) {
        if (error) *error = "Internal error: no destination map.";
        return false;
    }
    std::string text;
    if (!readFile(path, &text, error)) return false;

    Value root;
    if (!Value::parse(text, &root, error)) return false;
    if (!root.has("format") ||
        root["format"].asString() != "argmap/1") {
        if (error) *error = "Not an argmap/1 debate map file.";
        return false;
    }

    domain::DebateMap result;
    result.title = root["title"].asString("Untitled debate");
    result.nextId = root["nextId"].asInt(1);

    for (const Value& item : root["arguments"].asArray()) {
        domain::Argument arg;
        arg.id = item["id"].asString();
        arg.text = item["text"].asString();
        if (arg.id.empty() || arg.text.empty()) {
            if (error) *error = "A saved argument is missing its id or text.";
            return false;
        }
        arg.side = domain::parseSide(item["side"].asString("proposition"));
        arg.weight = item["weight"].asInt(3);
        if (arg.weight < 1) arg.weight = 1;
        if (arg.weight > 5) arg.weight = 5;
        arg.color = item["color"].asString();
        arg.speaker = domain::parseSpeaker(item["speaker"].asInt(0));
        arg.x = item["x"].asNumber(0.25);
        arg.y = item["y"].asNumber(0.25);
        result.arguments.push_back(arg);
    }

    for (const Value& item : root["counters"].asArray()) {
        domain::CounterLink link;
        link.id = item["id"].asString();
        link.counterId = item["counter"].asString();
        link.targetId = item["target"].asString();
        if (link.id.empty() || link.counterId.empty() || link.targetId.empty()) {
            if (error) *error = "A saved counter link is incomplete.";
            return false;
        }
        result.counters.push_back(link);
    }

    *map = result;
    return true;
}

} // namespace data
} // namespace argmap
