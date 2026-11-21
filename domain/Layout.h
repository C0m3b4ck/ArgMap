#pragma once

#include <string>
#include <vector>

#include "domain/Types.h"

namespace argmap {
namespace domain {

// Pure geometry description: no FLTK or file I/O here. Both the on-screen
// canvas and the PNG exporter render exactly the same Layout structure.
struct LayoutNode {
    std::string id;
    std::string text;
    Side side = Side::Proposition;
    int weight = 3;
    Speaker speaker = Speaker::NoSpeaker;
    Rgb color;
    std::vector<std::string> lines;
    double x = 0.0;  // centre, normalised 0..1
    double y = 0.0;
    double w = 0.0;  // size, normalised 0..1
    double h = 0.0;
};

struct LayoutLink {
    std::string id;
    std::string counterId;
    std::string targetId;
};

struct Layout {
    std::string title;
    std::vector<LayoutNode> nodes;
    std::vector<LayoutLink> links;
};

} // namespace domain
} // namespace argmap
