#include "domain/Types.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace argmap {
namespace domain {

const char* sideName(Side side) {
    return side == Side::Proposition ? "Proposition" : "Opposition";
}

const char* sideShortName(Side side) {
    return side == Side::Proposition ? "PROP" : "OPP";
}

Side parseSide(const std::string& text) {
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lower == "opposition" ? Side::Opposition : Side::Proposition;
}

Speaker parseSpeaker(int value) {
    if (value < static_cast<int>(Speaker::NoSpeaker) || value > static_cast<int>(Speaker::Four)) {
        return Speaker::NoSpeaker;
    }
    return static_cast<Speaker>(value);
}

int speakerValue(Speaker speaker) {
    return static_cast<int>(speaker);
}

Rgb parseHexColor(const std::string& hex) {
    Rgb color{};
    if (hex.size() != 7 || hex[0] != '#') {
        return color;
    }
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    const int r = nibble(hex[1]) * 16 + nibble(hex[2]);
    const int g = nibble(hex[3]) * 16 + nibble(hex[4]);
    const int b = nibble(hex[5]) * 16 + nibble(hex[6]);
    if (r < 0 || g < 0 || b < 0) {
        return Rgb{};
    }
    return Rgb{static_cast<unsigned char>(r), static_cast<unsigned char>(g),
               static_cast<unsigned char>(b)};
}

std::string toHexColor(const Rgb& rgb) {
    static const char* digits = "0123456789ABCDEF";
    std::string out = "#";
    out.push_back(digits[(rgb.r >> 4) & 0x0F]);
    out.push_back(digits[rgb.r & 0x0F]);
    out.push_back(digits[(rgb.g >> 4) & 0x0F]);
    out.push_back(digits[rgb.g & 0x0F]);
    out.push_back(digits[(rgb.b >> 4) & 0x0F]);
    out.push_back(digits[rgb.b & 0x0F]);
    return out;
}

} // namespace domain
} // namespace argmap
