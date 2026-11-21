#include "gui/Renderer.H"

#include <FL/fl_draw.H>

#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>

namespace argmap {
namespace gui {
namespace {

struct Point {
    double x = 0.0;
    double y = 0.0;
};

struct Transform {
    double cx = 0.0;
    double cy = 0.0;
    double sx = 1.0;
    double sy = 1.0;
    double panX = 0.0;
    double panY = 0.0;
    double zoom = 1.0;
    double uiScale = 1.0;
    int originX = 0;
    int originY = 0;
};

Transform makeTransform(int width, int height, const DrawOptions& options,
                        int originX = 0, int originY = 0) {
    Transform t;
    const double pad = std::max(22.0, std::min(width, height) * 0.04);
    t.cx = width / 2.0;
    t.cy = height / 2.0;
    t.originX = originX;
    t.originY = originY;
    t.sx = std::max(1.0, width - 2.0 * pad);
    t.sy = std::max(1.0, height - 2.0 * pad);
    t.panX = options.panX;
    t.panY = options.panY;
    t.zoom = options.zoom > 0.0 ? options.zoom : 1.0;
    t.uiScale = std::max(0.75, std::min(1.8,
                                        std::min(width / 900.0, height / 560.0)));
    return t;
}

int scaledPx(const Transform& t, double value) {
    return static_cast<int>(std::lround(value * t.uiScale));
}

Point toPixel(const Transform& t, double x, double y) {
    return Point{t.originX + t.cx + ((x - 0.5) * t.sx + t.panX) * t.zoom,
                 t.originY + t.cy + ((y - 0.5) * t.sy + t.panY) * t.zoom};
}

Point toWorldPoint(const Transform& t, int px, int py) {
    return Point{0.5 + ((px - t.cx) / t.zoom - t.panX) / t.sx,
                 0.5 + ((py - t.cy) / t.zoom - t.panY) / t.sy};
}

struct Rect {
    Point centre;
    double halfW = 0.0;
    double halfH = 0.0;
};

Rect nodeRect(const Transform& t, const domain::LayoutNode& node) {
    Rect rect;
    rect.centre = toPixel(t, node.x, node.y);
    rect.halfW = node.w * t.sx * t.zoom / 2.0;
    rect.halfH = node.h * t.sy * t.zoom / 2.0;
    return rect;
}

Point edgePoint(const Rect& rect, const Point& towards) {
    const double dx = towards.x - rect.centre.x;
    const double dy = towards.y - rect.centre.y;
    if (std::abs(dx) < 0.0001 && std::abs(dy) < 0.0001) {
        return rect.centre;
    }
    const double tx = std::abs(dx) < 0.0001 ? 1e12 : rect.halfW / std::abs(dx);
    const double ty = std::abs(dy) < 0.0001 ? 1e12 : rect.halfH / std::abs(dy);
    const double scale = std::min(tx, ty);
    return Point{rect.centre.x + dx * scale, rect.centre.y + dy * scale};
}

Fl_Color rgb(const domain::Rgb& color) {
    return fl_rgb_color(color.r, color.g, color.b);
}

Fl_Color darken(const domain::Rgb& color, double factor = 0.65) {
    return fl_rgb_color(static_cast<unsigned char>(color.r * factor),
                        static_cast<unsigned char>(color.g * factor),
                        static_cast<unsigned char>(color.b * factor));
}

Fl_Color textColor(const domain::Rgb& color) {
    const int brightness = static_cast<int>(color.r) * 3 +
                           static_cast<int>(color.g) * 6 +
                           static_cast<int>(color.b);
    return brightness > 1050 ? FL_BLACK : FL_WHITE;
}

void drawArrowHead(const Point& tip, const Point& from, Fl_Color color,
                   double scale) {
    const double angle = std::atan2(tip.y - from.y, tip.x - from.x);
    const double length = 11.0 * scale;
    const double spread = 0.42;
    Point left{tip.x - length * std::cos(angle - spread),
               tip.y - length * std::sin(angle - spread)};
    Point right{tip.x - length * std::cos(angle + spread),
                tip.y - length * std::sin(angle + spread)};
    fl_color(color);
    fl_polygon(static_cast<int>(tip.x), static_cast<int>(tip.y),
               static_cast<int>(left.x), static_cast<int>(left.y),
               static_cast<int>(right.x), static_cast<int>(right.y));
}

void drawBackground(const Transform& t, int width, int height,
                    const domain::Layout& layout, const DrawOptions& options) {
    const int ox = t.originX;
    const int oy = t.originY;
    const int x0 = ox;
    const int y0 = oy;
    const int y1 = oy + height;

    fl_color(FL_WHITE);
    fl_rectf(x0, y0, width, height);

    const Point split = toPixel(t, 0.5, 0.5);
    const int sx = static_cast<int>(split.x);
    const int blueW = std::max(0, std::min(width, sx - x0));

    fl_color(fl_rgb_color(232, 242, 255));
    fl_rectf(x0, y0, blueW, height);
    fl_color(fl_rgb_color(255, 238, 234));
    fl_rectf(x0 + blueW, y0, width - blueW, height);

    fl_color(fl_rgb_color(205, 216, 228));
    fl_line(sx, y0, sx, y1);

    const int headerH = scaledPx(t, 30);
    fl_color(fl_rgb_color(52, 90, 140));
    fl_rectf(x0, y0, blueW, headerH);
    fl_color(fl_rgb_color(150, 70, 55));
    fl_rectf(x0 + blueW, y0, width - blueW, headerH);
    fl_font(FL_HELVETICA_BOLD, scaledPx(t, 13));
    fl_color(FL_WHITE);
    fl_draw("PROPOSITION", x0 + scaledPx(t, 12), y0 + scaledPx(t, 20));
    const std::string rightLabel = "OPPOSITION";
    fl_draw(rightLabel.c_str(),
            x0 + blueW + scaledPx(t, 12), y0 + scaledPx(t, 20));

    fl_font(FL_HELVETICA, scaledPx(t, 12));
    fl_color(fl_rgb_color(90, 100, 115));
    if (!layout.title.empty()) {
        fl_draw(layout.title.c_str(), x0 + scaledPx(t, 12), y1 - scaledPx(t, 10));
    }
    if (options.linkMode) {
        fl_color(fl_rgb_color(140, 30, 100));
        fl_draw("LINK MODE: click the argument that is being countered",
                x0 + scaledPx(t, 12), y0 + headerH + scaledPx(t, 18));
    }
}

void drawLinks(const Transform& t, const domain::Layout& layout,
               const DrawOptions& options) {
    std::map<std::string, const domain::LayoutNode*> nodes;
    for (const domain::LayoutNode& node : layout.nodes) {
        nodes[node.id] = &node;
    }
    for (const domain::LayoutLink& link : layout.links) {
        const auto fromIt = nodes.find(link.counterId);
        const auto toIt = nodes.find(link.targetId);
        if (fromIt == nodes.end() || toIt == nodes.end()) continue;
        const Rect from = nodeRect(t, *fromIt->second);
        const Rect to = nodeRect(t, *toIt->second);
        const Point start = edgePoint(from, to.centre);
        const Point end = edgePoint(to, from.centre);
        const bool selected = link.id == options.selectedCounterId;
        Fl_Color color = fromIt->second->side == domain::Side::Proposition
                             ? fl_rgb_color(40, 90, 170)
                             : fl_rgb_color(175, 55, 35);
        if (selected) color = fl_rgb_color(220, 150, 0);
        fl_color(color);
        fl_line_style(FL_SOLID, selected ? scaledPx(t, 3) : scaledPx(t, 2));
        fl_line(static_cast<int>(start.x), static_cast<int>(start.y),
                static_cast<int>(end.x), static_cast<int>(end.y));
        fl_line_style(0);
        drawArrowHead(end, start, color, t.uiScale);
    }
}

void drawNode(const Transform& t, const domain::LayoutNode& node,
              const DrawOptions& options) {
    const Rect rect = nodeRect(t, node);
    const int x = static_cast<int>(rect.centre.x - rect.halfW);
    const int y = static_cast<int>(rect.centre.y - rect.halfH);
    const int w = static_cast<int>(rect.halfW * 2);
    const int h = static_cast<int>(rect.halfH * 2);

    fl_color(rgb(node.color));
    fl_rectf(x, y, w, h);
    fl_color(darken(node.color));
    fl_rect(x, y, w, h);

    const int stripH = scaledPx(t, 20);
    const int pad = scaledPx(t, 8);
    fl_color(darken(node.color, 0.55));
    fl_rectf(x + 1, y + 1, w - 2, stripH);
    fl_font(FL_HELVETICA_BOLD, scaledPx(t, 11));
    fl_color(FL_WHITE);
    fl_draw(domain::sideShortName(node.side), x + scaledPx(t, 7),
            y + scaledPx(t, 14));

    if (node.speaker != domain::Speaker::NoSpeaker) {
        const std::string badge = "S" + std::to_string(domain::speakerValue(node.speaker));
        const int badgeW = scaledPx(t, 24);
        const int badgeH = scaledPx(t, 15);
        fl_color(fl_rgb_color(60, 60, 70));
        fl_rectf(x + w - badgeW - scaledPx(t, 6), y + scaledPx(t, 3),
                 badgeW, badgeH);
        fl_color(FL_WHITE);
        fl_draw(badge.c_str(), x + w - badgeW - scaledPx(t, 2),
                y + scaledPx(t, 14));
    }

    const int lineStep = scaledPx(t, 15);
    fl_font(FL_HELVETICA, scaledPx(t, 12));
    fl_color(textColor(node.color));
    int textY = y + stripH + scaledPx(t, 16);
    for (const std::string& line : node.lines) {
        if (textY > y + h - scaledPx(t, 20)) break;
        fl_draw(line.c_str(), x + pad, textY);
        textY += lineStep;
    }

    const int barW = scaledPx(t, 7);
    const int barH = scaledPx(t, 5);
    const int barGap = scaledPx(t, 9);
    const int barY = y + h - scaledPx(t, 10);
    for (int i = 0; i < 5; ++i) {
        const int barX = x + pad + i * barGap;
        if (i < node.weight) {
            fl_color(fl_rgb_color(45, 55, 70));
            fl_rectf(barX, barY, barW, barH);
        } else {
            fl_color(fl_rgb_color(150, 155, 165));
            fl_rect(barX, barY, barW, barH);
        }
    }
    fl_font(FL_HELVETICA, scaledPx(t, 10));
    fl_color(fl_rgb_color(60, 65, 75));
    const std::string weight = "W" + std::to_string(node.weight);
    fl_draw(weight.c_str(), x + w - scaledPx(t, 28), y + h - scaledPx(t, 3));

    if (node.id == options.linkSourceId) {
        fl_color(fl_rgb_color(190, 30, 140));
        fl_rect(x - scaledPx(t, 3), y - scaledPx(t, 3),
                w + scaledPx(t, 6), h + scaledPx(t, 6));
        fl_rect(x - scaledPx(t, 5), y - scaledPx(t, 5),
                w + scaledPx(t, 10), h + scaledPx(t, 10));
    }
    if (node.id == options.selectedId) {
        fl_color(fl_rgb_color(255, 190, 0));
        fl_rect(x - scaledPx(t, 2), y - scaledPx(t, 2),
                w + scaledPx(t, 4), h + scaledPx(t, 4));
        fl_rect(x - scaledPx(t, 4), y - scaledPx(t, 4),
                w + scaledPx(t, 8), h + scaledPx(t, 8));
    }
}

} // namespace

void Renderer::draw(const domain::Layout& layout, int width, int height,
                    const DrawOptions& options, int originX, int originY) {
    if (width <= 0 || height <= 0) return;
    const Transform t = makeTransform(width, height, options, originX, originY);
    drawBackground(t, width, height, layout, options);
    drawLinks(t, layout, options);
    for (const domain::LayoutNode& node : layout.nodes) {
        drawNode(t, node, options);
    }
}

bool Renderer::hitTest(const domain::Layout& layout, int width, int height,
                       const DrawOptions& options, int mouseX, int mouseY,
                       std::string* id) {
    const Transform t = makeTransform(width, height, options);
    for (auto it = layout.nodes.rbegin(); it != layout.nodes.rend(); ++it) {
        const Rect rect = nodeRect(t, *it);
        if (mouseX >= rect.centre.x - rect.halfW &&
            mouseX <= rect.centre.x + rect.halfW &&
            mouseY >= rect.centre.y - rect.halfH &&
            mouseY <= rect.centre.y + rect.halfH) {
            if (id) *id = it->id;
            return true;
        }
    }
    return false;
}

void Renderer::toWorld(int width, int height, const DrawOptions& options,
                       int pixelX, int pixelY, double* x, double* y) {
    const Transform t = makeTransform(width, height, options);
    const Point p = toWorldPoint(t, pixelX, pixelY);
    if (x) *x = p.x;
    if (y) *y = p.y;
}

} // namespace gui
} // namespace argmap
