#pragma once

#include <FL/Fl_Box.H>

#include <string>

#include "app/Ports.h"
#include "gui/Renderer.H"

namespace argmap {
namespace gui {

class ArgumentCanvas : public Fl_Box {
public:
    ArgumentCanvas(int x, int y, int w, int h, const char* label = nullptr);

    void host(app::IAppController* controller) { host_ = controller; }

    void draw() override;
    int handle(int event) override;

private:
    DrawOptions options() const;
    void addAtPointer(domain::Side side);
    void showContextMenu(bool hitNode, const std::string& nodeId);

    app::IAppController* host_ = nullptr;
    bool draggingNode_ = false;
    bool panning_ = false;
    bool moved_ = false;
    std::string dragId_;
    int lastX_ = 0;
    int lastY_ = 0;
    int startX_ = 0;
    int startY_ = 0;
    double panX_ = 0.0;
    double panY_ = 0.0;
    double zoom_ = 1.0;
};

} // namespace gui
} // namespace argmap
