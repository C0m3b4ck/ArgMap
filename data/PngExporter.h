#pragma once

#include <string>

#include "domain/Ports.h"

namespace argmap {
namespace data {

class PngExporter : public domain::IImageExporter {
public:
    bool writePng(const std::string& path, const unsigned char* pixels,
                  int width, int height, int channels,
                  std::string* error) override;
};

} // namespace data
} // namespace argmap
