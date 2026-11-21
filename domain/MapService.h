#pragma once

#include <stdexcept>
#include <string>

#include "domain/Layout.h"
#include "domain/Types.h"

namespace argmap {
namespace domain {

class DomainError : public std::runtime_error {
public:
    explicit DomainError(const std::string& message) : std::runtime_error(message) {}
};

struct ArgumentDraft {
    std::string text;
    Side side = Side::Proposition;
    int weight = 3;
    std::string color;
    Speaker speaker = Speaker::NoSpeaker;
};

// All debate rules live here. This class has zero FLTK includes.
class MapService {
public:
    MapService();

    const DebateMap& map() const { return map_; }
    DebateMap& map() { return map_; }

    void newMap(const std::string& title = "Untitled debate");
    void replaceMap(const DebateMap& map);
    void setTitle(const std::string& title);

    std::string addArgument(const ArgumentDraft& draft, double x = -1.0, double y = -1.0);
    void updateArgument(const std::string& id, const ArgumentDraft& draft);
    void moveArgument(const std::string& id, double x, double y);
    bool removeArgument(const std::string& id);

    std::string addCounter(const std::string& counterId, const std::string& targetId);
    bool removeCounter(const std::string& id);

    void autoLayout();
    Layout computeLayout() const;
    MapStats stats() const;

    const Argument* findArgument(const std::string& id) const;
    std::string describeArgument(const std::string& id) const;

private:
    Argument* mutableArgument(const std::string& id);
    std::string nextId(char prefix);
    static std::string normaliseColor(const std::string& color, Side side);
    static std::string trimmed(const std::string& text);
    static double clamp01(double value);

    DebateMap map_;
};

} // namespace domain
} // namespace argmap
