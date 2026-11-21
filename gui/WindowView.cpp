#include "gui/WindowView.h"

#include <FL/Fl.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

#include "MapWindow.H"
#include "gui/Dialogs.h"

namespace argmap {
namespace gui {
namespace {

std::string oneLine(std::string text) {
    std::replace(text.begin(), text.end(), '\n', ' ');
    std::replace(text.begin(), text.end(), '\r', ' ');
    return text;
}

std::string truncated(const std::string& text, std::size_t limit) {
    if (limit < 4) limit = 4;
    return text.size() > limit ? text.substr(0, limit - 3) + "..." : text;
}

const domain::Argument* findArgument(const domain::DebateMap& map,
                                     const std::string& id) {
    for (const domain::Argument& arg : map.arguments) {
        if (arg.id == id) return &arg;
    }
    return nullptr;
}

std::string browserText(const domain::Argument& arg) {
    std::string line = std::string("[") + domain::sideShortName(arg.side) + "] " +
                       oneLine(arg.text) + "   (W" + std::to_string(arg.weight);
    if (arg.speaker != domain::Speaker::NoSpeaker) {
        line += ", S" + std::to_string(domain::speakerValue(arg.speaker));
    }
    line += ")";
    return line;
}

std::string statsText(const domain::MapStats& stats) {
    std::ostringstream os;
    os << "Proposition: " << stats.propositionArguments << " arguments, weight "
       << stats.propositionWeight << ", counters " << stats.propositionCounters << "\n"
       << "Opposition:  " << stats.oppositionArguments << " arguments, weight "
       << stats.oppositionWeight << ", counters " << stats.oppositionCounters << "\n"
       << "Total links: " << stats.propositionCounters + stats.oppositionCounters;
    return os.str();
}

int clampInt(int value, int low, int high) {
    return std::max(low, std::min(high, value));
}

int scaled(double value, int low, int high) {
    return clampInt(static_cast<int>(std::lround(value)), low, high);
}

void place(Fl_Widget* widget, int x, int y, int width, int height) {
    widget->resize(x, y, std::max(1, width), std::max(1, height));
}

void setActive(Fl_Widget* widget, bool active) {
    if (active) {
        widget->activate();
    } else {
        widget->deactivate();
    }
}

} // namespace

WindowView::WindowView(MapWindow& ui) : ui_(ui) {}

void WindowView::attach(app::IAppController& controller) {
    controller_ = &controller;
    ui_.canvas->host(&controller);
    controller.attachView(this);
}

void WindowView::refresh(const domain::DebateMap& map, const app::ViewState& state) {
    lastMap_ = map;
    lastState_ = state;

    ui_.titleInput->value(map.title.c_str());

    ui_.argBrowser->clear();
    int selectedArgumentLine = 0;
    for (std::size_t i = 0; i < map.arguments.size(); ++i) {
        ui_.argBrowser->add(browserText(map.arguments[i]).c_str());
        if (map.arguments[i].id == state.selectedArgumentId) {
            selectedArgumentLine = static_cast<int>(i) + 1;
        }
    }
    if (selectedArgumentLine > 0) {
        ui_.argBrowser->select(selectedArgumentLine);
    }

    ui_.counterBrowser->clear();
    counterLineIds_.clear();
    int selectedCounterLine = 0;
    const std::size_t perText =
        static_cast<std::size_t>(std::max(10, (ui_.counterBrowser->w() - 24) / 14));
    for (const domain::CounterLink& link : map.counters) {
        const domain::Argument* counter = findArgument(map, link.counterId);
        const domain::Argument* target = findArgument(map, link.targetId);
        if (!counter || !target) continue;
        counterLineIds_.push_back(link.id);
        const std::string line = truncated(browserText(*counter), perText) + " -> " +
                                 truncated(browserText(*target), perText);
        ui_.counterBrowser->add(line.c_str());
        if (link.id == state.selectedCounterId) {
            selectedCounterLine = static_cast<int>(counterLineIds_.size());
        }
    }
    if (selectedCounterLine > 0) {
        ui_.counterBrowser->select(selectedCounterLine);
    }

    currentColor_ = state.editor.color.empty() ? "#8FC7FF" : state.editor.color;
    ui_.textInput->value(state.editor.text.c_str());
    ui_.weightSlider->value(static_cast<double>(state.editor.weight));
    ui_.sideCheck->value(state.editor.side == domain::Side::Opposition ? 1 : 0);
    ui_.speakerChoice->value(static_cast<int>(state.editor.speaker));
    const domain::Rgb rgb = domain::parseHexColor(currentColor_);
    ui_.colorButton->color(fl_rgb_color(rgb.r, rgb.g, rgb.b));
    ui_.colorButton->copy_label(currentColor_.c_str());
    ui_.colorButton->redraw();

    // Make the current action obvious instead of relying on hidden modes.
    const bool hasSelection = state.hasEditorSelection;
    ui_.counterCheck->value(state.addAsCounter ? 1 : 0);
    ui_.counterCheck->redraw();
    if (state.linkMode) {
        ui_.linkButton->copy_label("2. Click target...");
        ui_.linkButton->color(fl_rgb_color(190, 40, 135));
        ui_.linkButton->labelcolor(FL_WHITE);
        ui_.linkButton->activate();
        ui_.hintBox->copy_label("LINK MODE: click the argument being countered. Esc cancels.");
        ui_.editorHint->copy_label("Link mode: click the target argument on the map.");
    } else if (state.addAsCounter) {
        ui_.linkButton->copy_label("Mark Counter");
        ui_.linkButton->color(FL_GRAY);
        ui_.linkButton->labelcolor(FL_BLACK);
        setActive(ui_.linkButton, hasSelection);
        if (hasSelection) {
            ui_.hintBox->copy_label(
                "COUNTER MODE: click + to add a linked counter, or type its text on the right.");
            ui_.editorHint->copy_label(
                "Counter mode: click + Proposition/Opposition to create and link a counter.");
        } else {
            ui_.hintBox->copy_label("COUNTER MODE: select the argument being countered first.");
            ui_.editorHint->copy_label("Counter mode: select the target argument on the map.");
        }
    } else {
        ui_.linkButton->copy_label(hasSelection ? "Mark Counter" : "Mark Counter (select first)");
        ui_.linkButton->color(FL_GRAY);
        ui_.linkButton->labelcolor(FL_BLACK);
        if (hasSelection) {
            ui_.linkButton->activate();
            ui_.hintBox->copy_label(
                "Selected: edit on the right, or check 'As counter' and click + to rebut.");
            ui_.editorHint->copy_label(
                "Edit fields, then Save changes. Weight/colour/speaker apply immediately.");
        } else {
            ui_.linkButton->deactivate();
            ui_.hintBox->copy_label("Click a box to select. Use + buttons to add arguments.");
            ui_.editorHint->copy_label("Select a box, or type text here and click Add from editor.");
        }
    }
    ui_.linkButton->redraw();

    setActive(ui_.cancelLinkButton, state.linkMode);
    setActive(ui_.deleteCounterButton, !state.selectedCounterId.empty());
    ui_.applyButton->copy_label(hasSelection ? "Save changes" : "Add from editor");
    setActive(ui_.deleteButton, hasSelection);

    ui_.statusBox->copy_label(state.status.c_str());

    const domain::MapStats stats =
        controller_ ? controller_->stats() : domain::MapStats{};
    ui_.statsBox->copy_label(statsText(stats).c_str());

    std::string title = map.title;
    if (state.dirty) title += " *";
    title += "  -  ArgMap";
    ui_.window->copy_label(title.c_str());
    ui_.canvas->redraw();
    ui_.hintBox->redraw();
    ui_.editorHint->redraw();
}

void WindowView::setStatus(const std::string& message) {
    lastState_.status = message;
    ui_.statusBox->copy_label(message.c_str());
}

void WindowView::focusEditor() {
    ui_.textInput->take_focus();
    ui_.textInput->position(ui_.textInput->size(), ui_.textInput->size());
}

void WindowView::repaintCanvas() {
    ui_.canvas->redraw();
}

std::string WindowView::askOpenPath() {
    return gui::openMapPath();
}

std::string WindowView::askSavePath(const std::string& current) {
    return gui::saveMapPath(current);
}

std::string WindowView::askPngPath(const std::string& current) {
    return gui::exportPngPath(current);
}

bool WindowView::confirm(const std::string& message) {
    return gui::confirm(message);
}

void WindowView::alert(const std::string& message) {
    gui::alert(message);
}

void WindowView::requestQuit() {
    std::exit(0);
}

void WindowView::layout(int width, int height) {
    width = std::max(1, width);
    height = std::max(1, height);

    const int margin = 8;
    const int menuH = 28;
    const int statusH = 24;
    const int mainY = menuH;
    const int mainH = std::max(260, height - mainY - statusH);

    place(ui_.menuBar, 0, 0, width, menuH);

    int sidebarW = scaled(width * 0.24, 180, 288);
    int editorW = scaled(width * 0.29, 268, 344);
    if (sidebarW + editorW + 300 > width) {
        sidebarW = std::max(180, width / 5);
        editorW = std::max(256, width / 4);
    }
    const int canvasW = std::max(220, width - sidebarW - editorW);
    const int canvasX = sidebarW;
    const int editorX = canvasX + canvasW;

    place(ui_.sidebar, 0, mainY, sidebarW, mainH);
    place(ui_.canvasPanel, canvasX, mainY, canvasW, mainH);
    place(ui_.canvas, canvasX + 2, mainY + 2, canvasW - 4, mainH - 4);
    place(ui_.editorPanel, editorX, mainY, editorW, mainH);

    // Left panel: argument/list browsers only. No command widgets live here.
    const int pad = margin;
    const int listW = std::max(1, sidebarW - 2 * pad);
    const int hintH = scaled(mainH * 0.15, 44, 108);
    const int listsH = std::max(80, mainH - hintH - 3 * pad);
    const int argH = scaled(listsH * 0.59, 60, 320);
    const int counterH = std::max(48, listsH - argH - pad);
    place(ui_.argBrowser, pad, mainY + pad, listW, argH);
    place(ui_.counterBrowser, pad, mainY + pad + argH + pad, listW, counterH);
    place(ui_.hintBox, pad, mainY + mainH - hintH - pad, listW, hintH);

    // Right panel: all command buttons live above the editor fields.
    const int ex = editorX + pad;
    const int iw = std::max(1, editorW - 2 * pad);
    const bool compact = mainH < 560;
    const int btnH = compact ? 20 : 24;
    const int btnGap = compact ? 2 : 4;
    const int colGap = 6;
    const int halfW = std::max(1, (iw - colGap) / 2);
    int y = mainY + pad;

    place(ui_.addPropButton, ex, y, halfW, btnH);
    place(ui_.addOppButton, ex + halfW + colGap, y, halfW, btnH);
    y += btnH + btnGap;
    place(ui_.counterCheck, ex, y, iw, btnH);
    y += btnH + btnGap;
    place(ui_.linkButton, ex, y, halfW, btnH);
    place(ui_.cancelLinkButton, ex + halfW + colGap, y, halfW, btnH);
    y += btnH + btnGap;
    place(ui_.deleteCounterButton, ex, y, halfW, btnH);
    place(ui_.layoutButton, ex + halfW + colGap, y, halfW, btnH);
    y += btnH + btnGap;
    place(ui_.openButton, ex, y, halfW, btnH);
    place(ui_.saveButton, ex + halfW + colGap, y, halfW, btnH);
    y += btnH + btnGap;
    place(ui_.exportButton, ex, y, halfW, btnH);
    place(ui_.helpButton, ex + halfW + colGap, y, halfW, btnH);
    y += btnH + btnGap;

    // Editor fields share the space between the buttons and the bottom hint.
    const int editorHintH = scaled(mainH * 0.06, 26, 44);
    const int hintY = mainY + mainH - editorHintH - pad;
    const int titleStep = compact ? 26 : 30;
    const int textStep = compact ? 12 : 16;
    const int weightStep = compact ? 26 : 32;
    const int speakerStep = compact ? 30 : 36;
    const int colorStep = compact ? 32 : 40;
    const int applyStep = compact ? 34 : 40;
    const int fixedSteps = titleStep + textStep + weightStep + speakerStep +
                           colorStep + applyStep;
    const int flexibleH = std::max(70, hintY - pad - y - fixedSteps);
    const int textH = clampInt(static_cast<int>(flexibleH * 0.55), 40, 170);
    const int statsH = clampInt(flexibleH - textH, 32, 130);

    place(ui_.titleInput, ex, y, iw, 22);
    y += titleStep;
    place(ui_.textInput, ex, y, iw, textH);
    y += textH + textStep;

    place(ui_.weightSlider, ex, y, iw, 18);
    y += weightStep;

    place(ui_.speakerChoice, ex, y, iw, 22);
    y += speakerStep;

    const int splitW = iw - 8;
    const int colorW = splitW * 48 / 100;
    place(ui_.colorButton, ex, y, colorW, 24);
    place(ui_.sideCheck, ex + colorW + 8, y, splitW - colorW, 24);
    y += colorStep;

    const int applyW = splitW * 62 / 100;
    place(ui_.applyButton, ex, y, applyW, 26);
    place(ui_.deleteButton, ex + applyW + 8, y, splitW - applyW, 26);
    y += applyStep;

    place(ui_.statsBox, ex, y, iw,
          std::max(1, std::min(statsH, hintY - pad - y)));
    place(ui_.editorHint, ex, hintY, iw, editorHintH);

    place(ui_.statusBox, margin, height - statusH + 2,
          std::max(1, width - 2 * margin), statusH - 4);

    ui_.sidebar->redraw();
    ui_.editorPanel->redraw();
    ui_.canvasPanel->redraw();
    ui_.statusBox->redraw();
}

app::EditorFields WindowView::currentFields() const {
    app::EditorFields fields;
    fields.text = ui_.textInput->value() ? ui_.textInput->value() : "";
    fields.side = ui_.sideCheck->value() ? domain::Side::Opposition
                                         : domain::Side::Proposition;
    fields.weight = static_cast<int>(ui_.weightSlider->value());
    fields.color = currentColor_;
    fields.speaker = domain::parseSpeaker(static_cast<int>(ui_.speakerChoice->value()));
    return fields;
}

void WindowView::onNew() {
    if (controller_) controller_->newMap();
}

void WindowView::onOpen() {
    if (controller_) controller_->openMap();
}

void WindowView::onSave() {
    if (controller_) controller_->saveMap();
}

void WindowView::onSaveAs() {
    if (controller_) controller_->saveMapAs();
}

void WindowView::onExportPng() {
    if (controller_) controller_->exportPng();
}

void WindowView::onQuit() {
    if (controller_) controller_->quit();
}

void WindowView::onAddProposition() {
    if (controller_) controller_->addArgument(domain::Side::Proposition);
}

void WindowView::onAddOpposition() {
    if (controller_) controller_->addArgument(domain::Side::Opposition);
}

void WindowView::onAddAsCounterChanged() {
    if (controller_) controller_->setAddAsCounter(ui_.counterCheck->value() != 0);
}

void WindowView::onApply() {
    if (controller_) controller_->commitEditor(currentFields());
}

void WindowView::onDelete() {
    if (controller_) controller_->deleteSelectedArgument();
}

void WindowView::onAutoLayout() {
    if (controller_) controller_->autoLayout();
}

void WindowView::onToggleLink() {
    if (controller_) controller_->toggleLinkMode();
}

void WindowView::onCancelLink() {
    if (controller_) controller_->cancelLinkMode();
}

void WindowView::onDeleteCounter() {
    if (controller_) controller_->deleteSelectedCounter();
}

void WindowView::onSelectArgument() {
    if (!controller_) return;
    const int line = ui_.argBrowser->value();
    if (line <= 0 || line > static_cast<int>(lastMap_.arguments.size())) return;
    controller_->argumentClicked(lastMap_.arguments[static_cast<std::size_t>(line - 1)].id);
}

void WindowView::onSelectCounter() {
    if (!controller_) return;
    const int line = ui_.counterBrowser->value();
    if (line <= 0 || line > static_cast<int>(counterLineIds_.size())) return;
    controller_->selectCounter(counterLineIds_[static_cast<std::size_t>(line - 1)]);
}

void WindowView::onTitleChanged() {
    if (controller_) controller_->setTitle(ui_.titleInput->value());
}

void WindowView::onWeightChanged() {
    if (controller_ && lastState_.hasEditorSelection) onApply();
}

void WindowView::onSpeakerChanged() {
    if (controller_ && lastState_.hasEditorSelection) onApply();
}

void WindowView::onPickColor() {
    const domain::Rgb current = domain::parseHexColor(currentColor_);
    unsigned char r = current.r;
    unsigned char g = current.g;
    unsigned char b = current.b;
    if (fl_color_chooser("Argument colour", r, g, b) != 0) {
        currentColor_ = domain::toHexColor(domain::Rgb{r, g, b});
        ui_.colorButton->color(fl_rgb_color(r, g, b));
        ui_.colorButton->copy_label(currentColor_.c_str());
        ui_.colorButton->redraw();
        if (lastState_.hasEditorSelection) onApply();
    }
}

void WindowView::onHelp() {
    alert(
        "Quick guide\n\n"
        "ADD: click '+ Proposition' or '+ Opposition' to add to that side. "
        "The editor gets focus so you can type the text straight away.\n\n"
        "COUNTER: select the argument being countered, tick 'As counter to "
        "selected', then click '+'. A new argument is created on the opposing "
        "side and linked automatically. You can also right-click empty canvas "
        "space.\n\n"
        "EDIT: click an argument box (or its row in the left list), change the "
        "text/weight/colour/speaker on the right, then click 'Save changes'.\n\n"
        "LINK EXISTING: select the rebutting argument, click 'Mark Counter', "
        "then click the argument being answered. Esc cancels.\n\n"
        "MOVE: drag a box. Middle-drag pans, wheel zooms. 'Auto Layout' tidies up.\n\n"
        "SAVE / EXPORT: File > Save writes .argmap; File > Export Image writes PNG.\n\n"
        "The layout lives in gui/MapWindow.fl and is edited with FLUID.");
}

} // namespace gui
} // namespace argmap
