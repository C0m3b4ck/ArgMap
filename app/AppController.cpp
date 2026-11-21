#include "app/AppController.h"

#include <utility>

namespace argmap {
namespace app {
namespace {

domain::ArgumentDraft toDraft(const EditorFields& fields) {
    domain::ArgumentDraft draft;
    draft.text = fields.text;
    draft.side = fields.side;
    draft.weight = fields.weight;
    draft.color = fields.color;
    draft.speaker = fields.speaker;
    return draft;
}

} // namespace

AppController::AppController(domain::MapService& service,
                             domain::IMapRepository& repository,
                             IMapImageRenderer& imageRenderer)
    : service_(service), repository_(repository), imageRenderer_(imageRenderer) {
    syncEditor();
}

void AppController::attachView(IView* view) {
    view_ = view;
    publish();
}

void AppController::publish() {
    syncEditor();
    if (view_) {
        view_->refresh(service_.map(), state_);
    }
}

void AppController::syncEditor() {
    const domain::Argument* arg = service_.findArgument(state_.selectedArgumentId);
    state_.hasEditorSelection = arg != nullptr;
    if (!arg) {
        state_.selectedArgumentId.clear();
        state_.editor = EditorFields{};
        return;
    }
    state_.editor.text = arg->text;
    state_.editor.side = arg->side;
    state_.editor.weight = arg->weight;
    state_.editor.color = arg->color;
    state_.editor.speaker = arg->speaker;
}

void AppController::status(const std::string& message) {
    state_.status = message;
    if (view_) view_->setStatus(message);
}

bool AppController::discardChangesIfNeeded() {
    if (!state_.dirty) return true;
    if (!view_) return true;
    return view_->confirm("The current map has unsaved changes. Discard them?");
}

bool AppController::saveTo(const std::string& path) {
    std::string error;
    if (!repository_.save(service_.map(), path, &error)) {
        if (view_) view_->alert(error);
        return false;
    }
    state_.filePath = path;
    state_.dirty = false;
    publish();
    status("Saved " + path);
    return true;
}

void AppController::newMap() {
    if (!discardChangesIfNeeded()) return;
    service_.newMap();
    state_ = ViewState{};
    publish();
    status("New map created.");
}

void AppController::openMap() {
    if (!discardChangesIfNeeded() || !view_) return;
    const std::string path = view_->askOpenPath();
    if (path.empty()) return;
    domain::DebateMap loaded;
    std::string error;
    if (!repository_.load(path, &loaded, &error)) {
        view_->alert(error);
        return;
    }
    service_.replaceMap(loaded);
    state_ = ViewState{};
    state_.filePath = path;
    publish();
    status("Opened " + path);
}

void AppController::saveMap() {
    if (state_.filePath.empty()) {
        saveMapAs();
        return;
    }
    saveTo(state_.filePath);
}

void AppController::saveMapAs() {
    if (!view_) return;
    const std::string path = view_->askSavePath(state_.filePath);
    if (path.empty()) return;
    saveTo(path);
}

void AppController::exportPng() {
    if (!view_) return;
    const std::string path = view_->askPngPath(state_.filePath);
    if (path.empty()) return;
    std::string error;
    if (!imageRenderer_.render(layout(), state_, path, 1600, 1000, &error)) {
        view_->alert(error.empty() ? "Could not export the image." : error);
        return;
    }
    status("Exported image to " + path);
}

void AppController::quit() {
    if (!discardChangesIfNeeded()) return;
    if (view_) view_->requestQuit();
}

void AppController::setTitle(const std::string& title) {
    if (service_.map().title == title) return;
    service_.setTitle(title);
    state_.dirty = true;
    publish();
}

void AppController::addArgument(domain::Side side, double x, double y) {
    domain::Side actualSide = side;
    std::string targetId;

    if (state_.addAsCounter) {
        if (state_.selectedArgumentId.empty()) {
            status("Select the argument being countered first, then add a counter.");
            return;
        }
        const domain::Argument* target = service_.findArgument(state_.selectedArgumentId);
        if (!target) {
            status("Select a valid argument first.");
            return;
        }
        actualSide = target->side == domain::Side::Proposition
                         ? domain::Side::Opposition
                         : domain::Side::Proposition;
        targetId = target->id;
    }

    EditorFields draft;
    draft.side = actualSide;
    draft.text = actualSide == domain::Side::Proposition ? "New proposition point"
                                                         : "New opposition point";
    draft.weight = 3;
    draft.color = actualSide == domain::Side::Proposition ? "#8FC7FF" : "#FFB3A7";
    try {
        const std::string id = service_.addArgument(toDraft(draft), x, y);
        if (!targetId.empty()) {
            const std::string linkId = service_.addCounter(id, targetId);
            state_.selectedCounterId = linkId;
        } else {
            state_.selectedCounterId.clear();
        }
        state_.selectedArgumentId = id;
        state_.addAsCounter = false;
        state_.dirty = true;
        publish();
        if (view_) view_->focusEditor();
        if (!targetId.empty()) {
            status("Added counter argument on the " +
                   std::string(domain::sideShortName(actualSide)) +
                   " side. Edit its text on the right.");
        } else {
            status("Added a " + std::string(domain::sideShortName(actualSide)) +
                   " argument. Edit its text on the right.");
        }
    } catch (const domain::DomainError& e) {
        if (view_) view_->alert(e.what());
    }
}

void AppController::setAddAsCounter(bool enabled) {
    if (state_.addAsCounter == enabled) return;
    state_.addAsCounter = enabled;
    publish();
    if (enabled) {
        status(state_.selectedArgumentId.empty()
                   ? "Counter mode: select the argument being countered first."
                   : "Counter mode: click Add to create and link a counterargument.");
    } else {
        status("Add mode: new arguments go to the side you choose.");
    }
}

void AppController::commitEditor(const EditorFields& fields) {
    try {
        if (state_.hasEditorSelection) {
            service_.updateArgument(state_.selectedArgumentId, toDraft(fields));
            status("Updated argument.");
        } else {
            const std::string id = service_.addArgument(toDraft(fields));
            state_.selectedArgumentId = id;
            status("Added argument.");
        }
        state_.dirty = true;
        publish();
    } catch (const domain::DomainError& e) {
        if (view_) view_->alert(e.what());
    }
}

void AppController::deleteSelectedArgument() {
    if (state_.selectedArgumentId.empty()) {
        status("Select an argument first.");
        return;
    }
    service_.removeArgument(state_.selectedArgumentId);
    state_.selectedArgumentId.clear();
    state_.selectedCounterId.clear();
    state_.linkSourceId.clear();
    state_.linkMode = false;
    state_.dirty = true;
    publish();
    status("Argument and its links deleted.");
}

void AppController::autoLayout() {
    service_.autoLayout();
    state_.dirty = true;
    publish();
    status("Arguments arranged by side.");
}

void AppController::toggleLinkMode() {
    if (state_.linkMode) {
        cancelLinkMode();
        return;
    }
    if (state_.selectedArgumentId.empty()) {
        status("Select the counterargument first, then start a link.");
        return;
    }
    state_.linkMode = true;
    state_.linkSourceId = state_.selectedArgumentId;
    state_.selectedCounterId.clear();
    publish();
    status("Now click the argument being countered (Esc cancels).");
}

void AppController::cancelLinkMode() {
    state_.linkMode = false;
    state_.linkSourceId.clear();
    publish();
    status("Counter linking cancelled.");
}

void AppController::deleteSelectedCounter() {
    if (state_.selectedCounterId.empty()) {
        status("Select a counter link in the list first.");
        return;
    }
    service_.removeCounter(state_.selectedCounterId);
    state_.selectedCounterId.clear();
    state_.dirty = true;
    publish();
    status("Counter link removed.");
}

void AppController::canvasClicked(double x, double y) {
    if (state_.linkMode) {
        cancelLinkMode();
        return;
    }
    state_.selectedArgumentId.clear();
    state_.selectedCounterId.clear();
    publish();
    status("Right-click empty space to add, or double-click to add here.");
    (void)x;
    (void)y;
}

void AppController::argumentClicked(const std::string& id) {
    if (!service_.findArgument(id)) return;

    if (state_.linkMode) {
        if (state_.linkSourceId.empty()) {
            state_.linkSourceId = id;
            publish();
            status("Now click the argument being countered.");
            return;
        }
        std::string linkId;
        try {
            linkId = service_.addCounter(state_.linkSourceId, id);
        } catch (const domain::DomainError& e) {
            if (view_) view_->alert(e.what());
            return;
        }
        state_.linkMode = false;
        state_.linkSourceId.clear();
        state_.selectedCounterId = linkId;
        state_.selectedArgumentId.clear();
        state_.dirty = true;
        publish();
        status("Counter link created.");
        return;
    }

    state_.selectedArgumentId = id;
    state_.selectedCounterId.clear();
    publish();
    status("Selected " + service_.describeArgument(id));
}

void AppController::argumentDoubleClicked(const std::string& id) {
    state_.linkMode = false;
    state_.linkSourceId.clear();
    state_.selectedArgumentId = id;
    publish();
    if (view_) view_->focusEditor();
}

void AppController::argumentDragged(const std::string& id, double x, double y) {
    service_.moveArgument(id, x, y);
    state_.dirty = true;
    if (view_) view_->repaintCanvas();
}

void AppController::selectCounter(const std::string& id) {
    state_.selectedCounterId = id;
    state_.selectedArgumentId.clear();
    publish();
    if (!id.empty()) status("Counter link selected. Use 'Remove Link' to delete it.");
}

} // namespace app
} // namespace argmap
