#pragma once

#include <string>

#include "domain/Types.h"

namespace argmap {
namespace domain {

// Implemented by the data layer. The business layer never opens a file.
class IMapRepository {
public:
    virtual ~IMapRepository() = default;
    virtual bool save(const DebateMap& map, const std::string& path,
                      std::string* error) = 0;
    virtual bool load(const std::string& path, DebateMap* map,
                      std::string* error) = 0;
};

// Implemented by the data layer. Receives raw RGB/RGBA pixels only.
class IImageExporter {
public:
    virtual ~IImageExporter() = default;
    virtual bool writePng(const std::string& path, const unsigned char* pixels,
                          int width, int height, int channels,
                          std::string* error) = 0;
};

} // namespace domain
} // namespace argmap
