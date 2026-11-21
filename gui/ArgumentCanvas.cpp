#include "gui/ArgumentCanvas.H"

#include <FL/Fl.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/fl_draw.H>

#include <algorithm>

namespace argmap {
namespace gui {

ArgumentCanvas::ArgumentCanvas(int x, int y, int w, int h, const char* label)
    : Fl_Box(x, y, w, h, label) {
    box(FL_FLAT_BOX);
    color(FL_WHITE);
    when(FL_WHEN_RELEASE);
}

DrawOptions ArgumentCanvas::options() const {
    DrawOptions options;
    if (host_) {
        const app::ViewState state = host_->state();
        options.selectedId = state.selectedArgumentId;
        options.linkSourceId = state.linkSourceId;
        options.selectedCounterId = state.selectedCounterId;
        options.linkMode = state.linkMode;
    }
    options.panX = panX_;
    options.panY = panY_;
    options.zoom = zoom_;
    return options;
}

void ArgumentCanvas::draw() {
    // Renderer draws into a (0,0,w,h) space (also used by the PNG exporter).
    // FLTK does not translate the origin for custom widgets, so pass this
    // widget's position explicitly; otherwise the canvas paints over the
    // menu bar and the areas above the canvas.
    const DrawOptions opt = options();
    if (host_) {
        Renderer::draw(host_->layout(), w(), h(), opt, x(), y());
    } else {
        fl_color(FL_WHITE);
        fl_rectf(x(), y(), w(), h());
        fl_color(FL_BLACK);
        fl_draw("Canvas is not connected", x() + 20, y() + 30);
    }
}

void ArgumentCanvas::addAtPointer(domain::Side side) {
    if (!host_) return;
    double worldX = 0.5;
    double worldY = 0.5;
    Renderer::toWorld(w(), h(), options(),
                      Fl::event_x() - x(), Fl::event_y() - y(),
                      &worldX, &worldY);
    host_->addArgument(side, worldX, worldY);
}

void ArgumentCanvas::showContextMenu(bool hitNode, const std::string& nodeId) {
    if (!host_) return;

    // In link mode, right-clicking a node is just a quicker way to pick the
    // target; no menu is needed.
    if (host_->state().linkMode) {
        if (hitNode) {
            host_->argumentClicked(nodeId);
        } else {
            host_->cancelLinkMode();
        }
        return;
    }

    if (hitNode) {
        Fl_Menu_Item items[4] = {};
        items[0].label("Edit text / properties");
        items[1].label("Mark counter from this");
        items[2].label("Delete argument");
        const Fl_Menu_Item* picked =
            items->popup(Fl::event_x_root(), Fl::event_y_root());
        if (!picked) return;
        switch (static_cast<int>(picked - items)) {
            case 0: host_->argumentDoubleClicked(nodeId); break;
            case 1:
                host_->argumentClicked(nodeId);
                host_->toggleLinkMode();
                break;
            case 2:
                host_->argumentClicked(nodeId);
                host_->deleteSelectedArgument();
                break;
            default: break;
        }
        return;
    }

    Fl_Menu_Item items[5] = {};
    items[0].label("Add Proposition here");
    items[1].label("Add Opposition here");
    items[2].label("-");
    items[2].flags = FL_MENU_DIVIDER;
    items[3].label("Auto layout");
    const Fl_Menu_Item* picked =
        items->popup(Fl::event_x_root(), Fl::event_y_root());
    if (!picked) return;
    switch (static_cast<int>(picked - items)) {
        case 0: addAtPointer(domain::Side::Proposition); break;
        case 1: addAtPointer(domain::Side::Opposition); break;
        case 3: host_->autoLayout(); break;
        default: break;
    }
}

int ArgumentCanvas::handle(int event) {
    if (!host_) return Fl_Box::handle(event);
    // Fl::event_x()/event_y() are window-relative; Renderer expects
    // coordinates relative to the canvas itself.
    const int mx = Fl::event_x() - x();
    const int my = Fl::event_y() - y();

    switch (event) {
        case FL_PUSH: {
            take_focus();
            startX_ = lastX_ = mx;
            startY_ = lastY_ = my;
            moved_ = false;

            std::string id;
            bool hit = false;
            if (Fl::event_button() == FL_RIGHT_MOUSE) {
                hit = Renderer::hitTest(host_->layout(), w(), h(), options(),
                                        mx, my, &id);
                showContextMenu(hit, id);
                return 1;
            }
            if (Fl::event_button() == FL_MIDDLE_MOUSE) {
                panning_ = true;
                return 1;
            }
            if (Fl::event_button() != FL_LEFT_MOUSE) return 0;

            hit = Renderer::hitTest(host_->layout(), w(), h(), options(),
                                    mx, my, &id);
            if (Fl::event_clicks() > 0) {
                if (hit) {
                    host_->argumentDoubleClicked(id);
                } else {
                    addAtPointer(mx < w() / 2 ? domain::Side::Proposition
                                              : domain::Side::Opposition);
                }
                return 1;
            }
            if (hit) {
                host_->argumentClicked(id);
                draggingNode_ = true;
                dragId_ = id;
                return 1;
            }
            host_->canvasClicked(0.5, 0.5);
            return 1;
        }
        case FL_DRAG: {
            if (panning_) {
                panX_ += mx - lastX_;
                panY_ += my - lastY_;
                lastX_ = mx;
                lastY_ = my;
                redraw();
                return 1;
            }
            if (draggingNode_) {
                if (std::abs(mx - startX_) + std::abs(my - startY_) > 3) {
                    moved_ = true;
                }
                if (moved_) {
                    double x = 0.5;
                    double y = 0.5;
                    Renderer::toWorld(w(), h(), options(), mx, my, &x, &y);
                    host_->argumentDragged(dragId_, x, y);
                }
                lastX_ = mx;
                lastY_ = my;
                return 1;
            }
            return 0;
        }
        case FL_RELEASE:
            draggingNode_ = false;
            panning_ = false;
            lastX_ = mx;
            lastY_ = my;
            return 1;
        case FL_MOUSEWHEEL: {
            const double factor = Fl::event_dy() < 0 ? 1.1 : 1.0 / 1.1;
            zoom_ = std::max(0.6, std::min(2.5, zoom_ * factor));
            redraw();
            return 1;
        }
        case FL_KEYDOWN:
            if (Fl::event_key() == FL_Escape) {
                host_->cancelLinkMode();
                return 1;
            }
            if (Fl::event_key() == '0') {
                zoom_ = 1.0;
                panX_ = 0.0;
                panY_ = 0.0;
                redraw();
                return 1;
            }
            return Fl_Box::handle(event);
        default:
            return Fl_Box::handle(event);
    }
}

} // namespace gui
} // namespace argmap
