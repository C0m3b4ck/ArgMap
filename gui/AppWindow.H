#pragma once

#include <FL/Fl_Double_Window.H>

namespace argmap {
namespace gui {

// FLUID still owns the widget tree. This thin subclass only forwards window
// resizes to the view so the panels can be laid out responsively.
class AppWindow : public Fl_Double_Window {
public:
    using LayoutCallback = void (*)(void* data, int width, int height);

    AppWindow(int width, int height, const char* label = nullptr);

    void onLayout(LayoutCallback callback, void* data) {
        layoutCallback_ = callback;
        layoutData_ = data;
    }

    void resize(int x, int y, int width, int height) override;

private:
    LayoutCallback layoutCallback_ = nullptr;
    void* layoutData_ = nullptr;
};

} // namespace gui
} // namespace argmap
