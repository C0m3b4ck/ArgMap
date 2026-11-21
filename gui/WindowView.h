#pragma once

#include <string>
#include <vector>

#include "app/Ports.h"
#include "domain/Types.h"

class MapWindow;

namespace argmap {
namespace gui {

// Adapter between FLTK widgets (declared in MapWindow.fl) and the application
// controller. It owns no debate rules and no file formats.
class WindowView : public app::IView {
public:
    explicit WindowView(MapWindow& ui);

    void attach(app::IAppController& controller);

    // app::IView
    void refresh(const domain::DebateMap& map, const app::ViewState& state) override;
    void setStatus(const std::string& message) override;
    void focusEditor() override;
    void repaintCanvas() override;
    std::string askOpenPath() override;
    std::string askSavePath(const std::string& current) override;
    std::string askPngPath(const std::string& current) override;
    bool confirm(const std::string& message) override;
    app::SavePrompt askSaveChanges() override;
    void alert(const std::string& message) override;
    void requestQuit() override;

    // Called by AppWindow whenever the window changes size.
    void layout(int width, int height);

    // Callbacks wired by MapWindow.fl
    void onNew();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onExportPng();
    void onQuit();
    void onAddProposition();
    void onAddOpposition();
    void onAddAsCounterChanged();
    void onApply();
    void onDelete();
    void onAutoLayout();
    void onToggleLink();
    void onCancelLink();
    void onDeleteCounter();
    void onSelectArgument();
    void onSelectCounter();
    void onTitleChanged();
    void onWeightChanged();
    void onSpeakerChanged();
    void onPickColor();
    void onHelp();

private:
    app::EditorFields currentFields() const;

    MapWindow& ui_;
    app::IAppController* controller_ = nullptr;
    domain::DebateMap lastMap_;
    app::ViewState lastState_;
    std::vector<std::string> counterLineIds_;
    std::string currentColor_ = "#8FC7FF";
};

} // namespace gui
} // namespace argmap
