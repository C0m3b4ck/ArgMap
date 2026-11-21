#pragma once

#include <string>

#include "domain/Ports.h"

namespace argmap {
namespace data {

class JsonMapRepository : public domain::IMapRepository {
public:
    bool save(const domain::DebateMap& map, const std::string& path,
              std::string* error) override;
    bool load(const std::string& path, domain::DebateMap* map,
              std::string* error) override;
};

} // namespace data
} // namespace argmap
