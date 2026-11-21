#include "gui/AppWindow.H"

namespace argmap {
namespace gui {

AppWindow::AppWindow(int width, int height, const char* label)
    : Fl_Double_Window(width, height, label) {
    // Resize is handled explicitly by WindowView::layout(), not by FLTK's
    // single-child resizable mechanism.
    resizable(nullptr);
}

void AppWindow::resize(int x, int y, int width, int height) {
    Fl_Double_Window::resize(x, y, width, height);
    if (layoutCallback_) layoutCallback_(layoutData_, w(), h());
}

} // namespace gui
} // namespace argmap
