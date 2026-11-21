#pragma once

#include <string>

#include "domain/Layout.h"
#include "domain/Types.h"

namespace argmap {
namespace app {

// Answer to "save changes before ...?" used for quit/close/new/open.
enum class SavePrompt { Save, Discard, Cancel };

struct EditorFields {
    std::string text;
    domain::Side side = domain::Side::Proposition;
    int weight = 3;
    std::string color;
    domain::Speaker speaker = domain::Speaker::NoSpeaker;
};

struct ViewState {
    std::string selectedArgumentId;
    std::string selectedCounterId;
    std::string linkSourceId;
    bool linkMode = false;
    bool addAsCounter = false;
    std::string status;
    std::string filePath;
    bool dirty = false;
    bool hasEditorSelection = false;
    EditorFields editor;
};

// The application asks the GUI for files and confirmations through this port.
// It never includes an FLTK header.
class IView {
public:
    virtual ~IView() = default;
    virtual void refresh(const domain::DebateMap& map, const ViewState& state) = 0;
    virtual void setStatus(const std::string& message) = 0;
    virtual void focusEditor() = 0;
    virtual void repaintCanvas() = 0;
    virtual std::string askOpenPath() = 0;
    virtual std::string askSavePath(const std::string& current) = 0;
    virtual std::string askPngPath(const std::string& current) = 0;
    virtual bool confirm(const std::string& message) = 0;
    virtual SavePrompt askSaveChanges() = 0;
    virtual void alert(const std::string& message) = 0;
    virtual void requestQuit() = 0;
};

// Rendering for export lives in the GUI layer; the controller only supplies a
// Layout and a destination path.
class IMapImageRenderer {
public:
    virtual ~IMapImageRenderer() = default;
    virtual bool render(const domain::Layout& layout, const ViewState& state,
                        const std::string& path, int width, int height,
                        std::string* error) = 0;
};

// Everything the FLTK widgets are allowed to ask the application to do.
class IAppController {
public:
    virtual ~IAppController() = default;

    virtual void attachView(IView* view) = 0;
    virtual void newMap() = 0;
    virtual void openMap() = 0;
    virtual void saveMap() = 0;
    virtual void saveMapAs() = 0;
    virtual void exportPng() = 0;
    virtual void quit() = 0;

    virtual void setTitle(const std::string& title) = 0;
    virtual void addArgument(domain::Side side, double x = -1.0, double y = -1.0) = 0;
    virtual void setAddAsCounter(bool enabled) = 0;
    virtual void commitEditor(const EditorFields& fields) = 0;
    virtual void deleteSelectedArgument() = 0;
    virtual void autoLayout() = 0;

    virtual void toggleLinkMode() = 0;
    virtual void cancelLinkMode() = 0;
    virtual void deleteSelectedCounter() = 0;

    virtual void canvasClicked(double x, double y) = 0;
    virtual void argumentClicked(const std::string& id) = 0;
    virtual void argumentDoubleClicked(const std::string& id) = 0;
    virtual void argumentDragged(const std::string& id, double x, double y) = 0;

    virtual void selectCounter(const std::string& id) = 0;

    virtual ViewState state() const = 0;
    virtual domain::Layout layout() const = 0;
    virtual domain::MapStats stats() const = 0;
};

} // namespace app
} // namespace argmap
