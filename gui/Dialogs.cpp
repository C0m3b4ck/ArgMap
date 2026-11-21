#include "gui/Dialogs.h"

#include <FL/Fl.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>

#include <memory>

namespace argmap {
namespace gui {
namespace {

std::string withExtension(std::string path, const std::string& extension) {
    if (path.empty()) return path;
    if (path.size() >= extension.size() &&
        path.compare(path.size() - extension.size(), extension.size(), extension) == 0) {
        return path;
    }
    return path + extension;
}

std::string choosePath(const char* title, const char* pattern, int type,
                       const std::string& current, const std::string& extension) {
    const char* start = current.empty() ? "." : current.c_str();
    std::unique_ptr<Fl_File_Chooser> chooser(
        new Fl_File_Chooser(start, pattern, type, title));
    chooser->show();
    while (chooser->shown()) {
        Fl::wait();
    }
    if (!chooser->value()) return std::string();
    return withExtension(chooser->value(), extension);
}

} // namespace

std::string openMapPath() {
    return choosePath("Open debate map", "Debate Maps (*.argmap)",
                      Fl_File_Chooser::SINGLE, "", "");
}

std::string saveMapPath(const std::string& current) {
    return choosePath("Save debate map", "Debate Maps (*.argmap)",
                      Fl_File_Chooser::SINGLE | Fl_File_Chooser::CREATE,
                      current, ".argmap");
}

std::string exportPngPath(const std::string& current) {
    std::string suggested = current;
    const std::size_t dot = suggested.rfind('.');
    if (dot != std::string::npos) {
        suggested = suggested.substr(0, dot);
    }
    return choosePath("Export map as PNG", "PNG Images (*.png)",
                      Fl_File_Chooser::SINGLE | Fl_File_Chooser::CREATE,
                      suggested, ".png");
}

bool confirm(const std::string& message) {
    return fl_choice("%s", "Cancel", "Continue", nullptr, message.c_str()) == 1;
}

app::SavePrompt askSaveChanges() {
    // FLTK focuses button[1] by default, so "Save" is the default action.
    const int choice = fl_choice(
        "%s", "Don't Save", "Save", "Cancel",
        "The current map has unsaved changes.\nSave before closing?");
    switch (choice) {
        case 1: return app::SavePrompt::Save;
        case 0: return app::SavePrompt::Discard;
        default: return app::SavePrompt::Cancel;
    }
}

void alert(const std::string& message) {
    fl_message("%s", message.c_str());
}

} // namespace gui
} // namespace argmap
