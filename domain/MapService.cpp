#include "domain/MapService.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

namespace argmap {
namespace domain {
namespace {

const char* kPropositionColor = "#8FC7FF";
const char* kOppositionColor = "#FFB3A7";

std::string trimCopy(const std::string& text) {
    std::size_t first = 0;
    while (first < text.size() && std::isspace(static_cast<unsigned char>(text[first]))) {
        ++first;
    }
    std::size_t last = text.size();
    while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1]))) {
        --last;
    }
    return text.substr(first, last - first);
}

std::vector<std::string> wrapText(const std::string& text, std::size_t maxChars,
                                  std::size_t maxLines) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string word;
    std::string line;

    while (stream >> word) {
        if (!line.empty() && line.size() + 1 + word.size() > maxChars) {
            lines.push_back(line);
            line.clear();
        }
        if (!line.empty()) {
            line += ' ';
        }
        line += word;
    }
    if (!line.empty()) {
        lines.push_back(line);
    }
    if (lines.empty()) {
        lines.push_back("(empty argument)");
    }
    if (lines.size() > maxLines) {
        lines.resize(maxLines);
        std::string& last = lines.back();
        if (last.size() > 3) {
            last.replace(last.size() - 3, 3, "...");
        }
    }
    return lines;
}

double defaultX(Side side) {
    return side == Side::Proposition ? 0.25 : 0.75;
}

} // namespace

MapService::MapService() {
    newMap();
}

void MapService::newMap(const std::string& title) {
    map_ = DebateMap{};
    map_.title = trimCopy(title);
    if (map_.title.empty()) {
        map_.title = "Untitled debate";
    }
}

void MapService::replaceMap(const DebateMap& map) {
    map_ = map;
    map_.title = trimCopy(map_.title);
    if (map_.title.empty()) {
        map_.title = "Untitled debate";
    }
    map_.nextId = std::max(map_.nextId, 1);
    for (const Argument& arg : map_.arguments) {
        map_.nextId = std::max(map_.nextId, std::atoi(arg.id.c_str()) + 1);
    }
}

void MapService::setTitle(const std::string& title) {
    map_.title = trimCopy(title);
    if (map_.title.empty()) {
        map_.title = "Untitled debate";
    }
}

std::string MapService::nextId(char prefix) {
    std::string id;
    do {
        id = std::string(1, prefix) + std::to_string(map_.nextId++);
    } while (findArgument(id) != nullptr);
    return id;
}

std::string MapService::normaliseColor(const std::string& color, Side side) {
    std::string result = trimCopy(color);
    if (result.size() != 7 || result[0] != '#') {
        return side == Side::Proposition ? kPropositionColor : kOppositionColor;
    }
    for (std::size_t i = 1; i < result.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(result[i]);
        if (!std::isxdigit(c)) {
            return side == Side::Proposition ? kPropositionColor : kOppositionColor;
        }
    }
    for (char& c : result) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

std::string MapService::trimmed(const std::string& text) {
    return trimCopy(text);
}

double MapService::clamp01(double value) {
    if (std::isnan(value)) return 0.5;
    return std::max(0.0, std::min(1.0, value));
}

std::string MapService::addArgument(const ArgumentDraft& draft, double x, double y) {
    Argument arg;
    arg.id = nextId('a');
    arg.text = trimmed(draft.text);
    if (arg.text.empty()) {
        throw DomainError("An argument needs some text before it can be added.");
    }
    arg.side = draft.side;
    arg.weight = std::max(1, std::min(5, draft.weight));
    arg.color = normaliseColor(draft.color, arg.side);
    arg.speaker = draft.speaker;
    arg.x = clamp01(x < 0.0 ? defaultX(arg.side) : x);
    arg.y = clamp01(y < 0.0 ? (0.16 + 0.13 * static_cast<double>(map_.arguments.size())) : y);
    arg.y = std::min(arg.y, 0.88);
    map_.arguments.push_back(arg);
    return arg.id;
}

void MapService::updateArgument(const std::string& id, const ArgumentDraft& draft) {
    Argument* arg = mutableArgument(id);
    if (!arg) {
        throw DomainError("The selected argument no longer exists.");
    }
    const std::string text = trimmed(draft.text);
    if (text.empty()) {
        throw DomainError("An argument cannot have empty text.");
    }
    arg->text = text;
    arg->side = draft.side;
    arg->weight = std::max(1, std::min(5, draft.weight));
    arg->color = normaliseColor(draft.color, arg->side);
    arg->speaker = draft.speaker;
}

void MapService::moveArgument(const std::string& id, double x, double y) {
    Argument* arg = mutableArgument(id);
    if (!arg) {
        return;
    }
    arg->x = clamp01(x);
    arg->y = clamp01(y);
}

bool MapService::removeArgument(const std::string& id) {
    const auto before = map_.arguments.size();
    map_.arguments.erase(
        std::remove_if(map_.arguments.begin(), map_.arguments.end(),
                       [&id](const Argument& arg) { return arg.id == id; }),
        map_.arguments.end());
    if (map_.arguments.size() == before) {
        return false;
    }
    map_.counters.erase(
        std::remove_if(map_.counters.begin(), map_.counters.end(),
                       [&id](const CounterLink& link) {
                           return link.counterId == id || link.targetId == id;
                       }),
        map_.counters.end());
    return true;
}

std::string MapService::addCounter(const std::string& counterId,
                                   const std::string& targetId) {
    if (counterId == targetId) {
        throw DomainError("An argument cannot counter itself.");
    }
    const Argument* counter = findArgument(counterId);
    const Argument* target = findArgument(targetId);
    if (!counter || !target) {
        throw DomainError("Both ends of a counter must be existing arguments.");
    }
    if (counter->side == target->side) {
        throw DomainError("A counter must come from the opposing side.");
    }
    for (const CounterLink& link : map_.counters) {
        if (link.counterId == counterId && link.targetId == targetId) {
            throw DomainError("That counter is already marked.");
        }
    }
    CounterLink link;
    link.id = nextId('c');
    link.counterId = counterId;
    link.targetId = targetId;
    map_.counters.push_back(link);
    return link.id;
}

bool MapService::removeCounter(const std::string& id) {
    const auto before = map_.counters.size();
    map_.counters.erase(
        std::remove_if(map_.counters.begin(), map_.counters.end(),
                       [&id](const CounterLink& link) { return link.id == id; }),
        map_.counters.end());
    return map_.counters.size() != before;
}

void MapService::autoLayout() {
    int propIndex = 0;
    int oppIndex = 0;
    for (Argument& arg : map_.arguments) {
        const bool prop = arg.side == Side::Proposition;
        const int index = prop ? propIndex++ : oppIndex++;
        const int total = prop ? static_cast<int>(std::count_if(
                                     map_.arguments.begin(), map_.arguments.end(),
                                     [](const Argument& a) {
                                         return a.side == Side::Proposition;
                                     }))
                               : static_cast<int>(map_.arguments.size()) - propIndex;
        arg.x = defaultX(arg.side);
        arg.y = total <= 1 ? 0.45 : 0.14 + 0.68 * index / (total - 1);
    }
}

Layout MapService::computeLayout() const {
    Layout layout;
    layout.title = map_.title;
    for (const Argument& arg : map_.arguments) {
        LayoutNode node;
        node.id = arg.id;
        node.text = arg.text;
        node.side = arg.side;
        node.weight = arg.weight;
        node.speaker = arg.speaker;
        node.color = parseHexColor(arg.color);
        node.lines = wrapText(arg.text, 23, 4);
        node.w = 0.29;
        node.h = 0.072 + 0.031 * static_cast<double>(node.lines.size());
        node.x = arg.x <= 0.0 ? defaultX(arg.side) : clamp01(arg.x);
        node.y = arg.y <= 0.0 ? 0.45 : clamp01(arg.y);
        node.x = std::max(node.w / 2 + 0.015, std::min(1.0 - node.w / 2 - 0.015, node.x));
        node.y = std::max(node.h / 2 + 0.075, std::min(1.0 - node.h / 2 - 0.025, node.y));
        layout.nodes.push_back(std::move(node));
    }
    for (const CounterLink& link : map_.counters) {
        layout.links.push_back(LayoutLink{link.id, link.counterId, link.targetId});
    }
    return layout;
}

MapStats MapService::stats() const {
    MapStats stats;
    for (const Argument& arg : map_.arguments) {
        if (arg.side == Side::Proposition) {
            ++stats.propositionArguments;
            stats.propositionWeight += arg.weight;
        } else {
            ++stats.oppositionArguments;
            stats.oppositionWeight += arg.weight;
        }
    }
    for (const CounterLink& link : map_.counters) {
        const Argument* counter = findArgument(link.counterId);
        if (!counter) continue;
        if (counter->side == Side::Proposition) {
            ++stats.propositionCounters;
        } else {
            ++stats.oppositionCounters;
        }
    }
    return stats;
}

const Argument* MapService::findArgument(const std::string& id) const {
    for (const Argument& arg : map_.arguments) {
        if (arg.id == id) return &arg;
    }
    return nullptr;
}

Argument* MapService::mutableArgument(const std::string& id) {
    for (Argument& arg : map_.arguments) {
        if (arg.id == id) return &arg;
    }
    return nullptr;
}

std::string MapService::describeArgument(const std::string& id) const {
    const Argument* arg = findArgument(id);
    if (!arg) return "(missing argument)";
    std::string text = arg->text;
    if (text.size() > 32) {
        text = text.substr(0, 29) + "...";
    }
    return std::string("[") + sideShortName(arg->side) + "] " + text;
}

} // namespace domain
} // namespace argmap
