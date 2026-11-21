#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace argmap {
namespace domain {

enum class Side { Proposition, Opposition };
enum class Speaker { NoSpeaker = 0, One = 1, Two = 2, Three = 3, Four = 4 };

struct Rgb {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
};

struct Argument {
    std::string id;
    std::string text;
    Side side = Side::Proposition;
    int weight = 3;          // 1 (weak) .. 5 (strong)
    std::string color = "#7DB7FF";  // #RRGGBB, user override
    Speaker speaker = Speaker::NoSpeaker;
    double x = 0.25;         // normalised position, 0..1
    double y = 0.25;
};

struct CounterLink {
    std::string id;
    std::string counterId;   // the argument doing the rebutting
    std::string targetId;    // the argument being rebutted
};

struct DebateMap {
    std::string title = "Untitled debate";
    std::vector<Argument> arguments;
    std::vector<CounterLink> counters;
    int nextId = 1;
};

struct MapStats {
    int propositionArguments = 0;
    int oppositionArguments = 0;
    int propositionCounters = 0;
    int oppositionCounters = 0;
    int propositionWeight = 0;
    int oppositionWeight = 0;
};

const char* sideName(Side side);
const char* sideShortName(Side side);
Side parseSide(const std::string& text);
Speaker parseSpeaker(int value);
int speakerValue(Speaker speaker);

Rgb parseHexColor(const std::string& hex);
std::string toHexColor(const Rgb& rgb);

} // namespace domain
} // namespace argmap
