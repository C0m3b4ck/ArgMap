#pragma once

#include <string>

#include "app/Ports.h"

namespace argmap {
namespace gui {

std::string openMapPath();
std::string saveMapPath(const std::string& current);
std::string exportPngPath(const std::string& current);
bool confirm(const std::string& message);
app::SavePrompt askSaveChanges();
void alert(const std::string& message);

} // namespace gui
} // namespace argmap
