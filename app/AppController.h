#pragma once

#include <string>

#include "app/Ports.h"
#include "domain/MapService.h"
#include "domain/Ports.h"

namespace argmap {
namespace app {

class AppController : public IAppController {
public:
    AppController(domain::MapService& service, domain::IMapRepository& repository,
                  IMapImageRenderer& imageRenderer);

    void attachView(IView* view) override;

    void newMap() override;
    void openMap() override;
    void saveMap() override;
    void saveMapAs() override;
    void exportPng() override;
    void quit() override;

    void setTitle(const std::string& title) override;
    void addArgument(domain::Side side, double x = -1.0, double y = -1.0) override;
    void setAddAsCounter(bool enabled) override;
    void commitEditor(const EditorFields& fields) override;
    void deleteSelectedArgument() override;
    void autoLayout() override;

    void toggleLinkMode() override;
    void cancelLinkMode() override;
    void deleteSelectedCounter() override;

    void canvasClicked(double x, double y) override;
    void argumentClicked(const std::string& id) override;
    void argumentDoubleClicked(const std::string& id) override;
    void argumentDragged(const std::string& id, double x, double y) override;

    void selectCounter(const std::string& id) override;

    ViewState state() const override { return state_; }
    domain::Layout layout() const override { return service_.computeLayout(); }
    domain::MapStats stats() const override { return service_.stats(); }

private:
    void publish();
    void syncEditor();
    void status(const std::string& message);
    bool saveTo(const std::string& path);
    bool saveCurrentMap();
    bool discardChangesIfNeeded();

    domain::MapService& service_;
    domain::IMapRepository& repository_;
    IMapImageRenderer& imageRenderer_;
    IView* view_ = nullptr;
    ViewState state_;
};

} // namespace app
} // namespace argmap
