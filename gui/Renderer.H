#pragma once

#include <string>

#include "domain/Layout.h"

namespace argmap {
namespace gui {

struct DrawOptions {
    std::string selectedId;
    std::string linkSourceId;
    std::string selectedCounterId;
    bool linkMode = false;
    double panX = 0.0;
    double panY = 0.0;
    double zoom = 1.0;
};

// Shared by the interactive canvas and the PNG exporter so the saved picture
// is identical to what the user sees.
class Renderer {
public:
    // `originX/originY` place the (0,0,w,h) drawing space inside its host.
    // The PNG exporter leaves them at zero; the FLTK canvas passes x()/y().
    static void draw(const domain::Layout& layout, int width, int height,
                     const DrawOptions& options,
                     int originX = 0, int originY = 0);
    static bool hitTest(const domain::Layout& layout, int width, int height,
                        const DrawOptions& options, int mouseX, int mouseY,
                        std::string* id);
    static void toWorld(int width, int height, const DrawOptions& options,
                        int pixelX, int pixelY, double* x, double* y);
};

} // namespace gui
} // namespace argmap
