#include "gui/MapImageRenderer.h"

#include <FL/Fl_Device.H>
#include <FL/Fl_Image_Surface.H>
#include <FL/Fl_RGB_Image.H>
#include <FL/fl_draw.H>

#include <vector>

#include "gui/Renderer.H"

namespace argmap {
namespace gui {

bool MapImageRenderer::render(const domain::Layout& layout,
                              const app::ViewState& state,
                              const std::string& path, int width, int height,
                              std::string* error) {
    Fl_Image_Surface surface(width, height);
    surface.set_current();

    fl_color(FL_WHITE);
    fl_rectf(0, 0, width, height);

    DrawOptions options;
    options.selectedId = state.selectedArgumentId;
    options.linkSourceId = state.linkSourceId;
    options.selectedCounterId = state.selectedCounterId;
    options.linkMode = state.linkMode;
    Renderer::draw(layout, width, height, options);

    Fl_RGB_Image* image = surface.image();
    Fl_Display_Device::display_device()->set_current();
    if (!image) {
        if (error) *error = "Could not create an offscreen image.";
        return false;
    }

    const int channels = image->d();
    const unsigned char* pixels =
        reinterpret_cast<const unsigned char*>(image->data()[0]);
    // In FLTK 1.3 ld() == 0 means "tightly packed, w() * d() bytes per row".
    const int lineSize = image->ld() != 0 ? image->ld() : width * channels;
    std::vector<unsigned char> packed;
    if (lineSize != width * channels) {
        packed.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
                      static_cast<std::size_t>(channels));
        for (int y = 0; y < height; ++y) {
            const unsigned char* source = pixels + static_cast<std::size_t>(y) *
                                                       static_cast<std::size_t>(lineSize);
            unsigned char* target = packed.data() + static_cast<std::size_t>(y) *
                                                       static_cast<std::size_t>(width) *
                                                       static_cast<std::size_t>(channels);
            for (int x = 0; x < width * channels; ++x) {
                target[x] = source[x];
            }
        }
        pixels = packed.data();
    }

    const bool ok = exporter_.writePng(path, pixels, width, height, channels, error);
    delete image;
    return ok;
}

} // namespace gui
} // namespace argmap
