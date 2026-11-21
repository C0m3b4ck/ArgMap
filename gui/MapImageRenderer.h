#pragma once

#include <string>

#include "app/Ports.h"
#include "domain/Ports.h"

namespace argmap {
namespace gui {

// Uses the same Renderer as the screen canvas, then hands the raw pixels to
// the data layer's PNG writer.
class MapImageRenderer : public app::IMapImageRenderer {
public:
    explicit MapImageRenderer(domain::IImageExporter& exporter) : exporter_(exporter) {}

    bool render(const domain::Layout& layout, const app::ViewState& state,
                const std::string& path, int width, int height,
                std::string* error) override;

private:
    domain::IImageExporter& exporter_;
};

} // namespace gui
} // namespace argmap
