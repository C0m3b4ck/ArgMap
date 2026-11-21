#include <FL/Fl.H>

#include "MapWindow.H"
#include "app/AppController.h"
#include "data/JsonMapRepository.h"
#include "data/PngExporter.h"
#include "gui/MapImageRenderer.h"
#include "gui/WindowView.h"

namespace {

// Do NOT pass this through Fl_Widget::callback(cb, data): that overload writes
// `data` into the window's user_data, which FLUID uses to find the MapWindow
// from every generated button/menu callback. Overwriting it makes all controls
// call through a bogus pointer and appear dead.
argmap::app::IAppController* closeController = nullptr;

void closeCallback(Fl_Widget*) {
    if (closeController) closeController->quit();
}

} // namespace

int main(int argc, char** argv) {
    Fl::scheme("plastic");

    argmap::domain::MapService service;
    argmap::data::JsonMapRepository repository;
    argmap::data::PngExporter exporter;
    argmap::gui::MapImageRenderer imageRenderer(exporter);

    argmap::app::AppController controller(service, repository, imageRenderer);

    MapWindow ui;
    argmap::gui::WindowView view(ui);
    view.attach(controller);
    ui.attachView(&view);

    ui.window->onLayout(
        [](void* data, int width, int height) {
            static_cast<argmap::gui::WindowView*>(data)->layout(width, height);
        },
        &view);
    view.layout(ui.window->w(), ui.window->h());

    closeController = &controller;

    ui.window->size_range(760, 520);
    ui.window->callback(closeCallback);  // one-arg overload preserves user_data
    ui.window->show(argc, argv);

    return Fl::run();
}
