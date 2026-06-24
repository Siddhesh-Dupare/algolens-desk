#include "Graph.h"
#include <algorithm>
#include <cmath>
#include <vector>

static NVGcolor lerpColor(NVGcolor a, NVGcolor b, float t) {
    return nvgRGBAf(a.r + (b.r - a.r) * t,
                    a.g + (b.g - a.g) * t,
                    a.b + (b.b - a.b) * t,
                    a.a + (b.a - a.a) * t);
}

void Graph::render(NVGcontext* vg, int width, int height) {
    const int n = (int)nodes_.size();
    nvgFontFace(vg, "sans");

    if (n == 0) {
        nvgFontSize(vg, 13.0f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGB(120, 120, 130));
        nvgText(vg, width / 2.0f, height / 2.0f, "empty graph", nullptr);
        return;
    }

    const float cx = width / 2.0f;
    const float cy = height / 2.0f;
    const float margin = 44.0f;  // room for nodes + pointer labels
    const float R = std::max(0.0f, std::min(width, height) / 2.0f - margin);

    // Circular layout (node 0 at the top, clockwise) — deterministic per frame.
    std::vector<float> nx(n), ny(n);
    for (int i = 0; i < n; i++) {
        if (n == 1) { nx[i] = cx; ny[i] = cy; continue; }
        const float a = -3.14159265f / 2.0f + 2.0f * 3.14159265f * i / n;
        nx[i] = cx + R * std::cos(a);
        ny[i] = cy + R * std::sin(a);
    }

    // Node radius from the spacing between adjacent nodes on the circle.
    float r = 24.0f;
    if (n > 1) r = std::min(24.0f, R * std::sin(3.14159265f / n) * 0.8f);
    if (r < 8.0f) r = 8.0f;
    const float fs = r / 24.0f;

    using namespace std::chrono;
    float t = duration<float>(steady_clock::now() - birth_).count() / 0.22f;
    if (t > 1.0f) t = 1.0f;
    const float ease = 1.0f - std::pow(1.0f - t, 3.0f);

    const NVGcolor base = nvgRGB(45, 55, 72);
    const NVGcolor link = nvgRGB(110, 120, 140);

    // Edges (behind nodes).
    nvgStrokeColor(vg, link);
    nvgStrokeWidth(vg, std::max(1.3f, 1.8f * fs));
    for (const auto& e : edges_) {
        if (e.from < 0 || e.from >= n || e.to < 0 || e.to >= n) continue;
        if (e.from == e.to) continue;  // self-loops not drawn
        // Undirected: draw each pair once (the tracer emits both directions).
        if (!directed_ && e.from > e.to) continue;

        const float x1 = nx[e.from], y1 = ny[e.from];
        const float x2 = nx[e.to], y2 = ny[e.to];
        float dx = x2 - x1, dy = y2 - y1;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1.0f) continue;
        dx /= len; dy /= len;

        // Stop the line at each node's edge.
        const float sx = x1 + dx * r, sy = y1 + dy * r;
        const float ex = x2 - dx * r, ey = y2 - dy * r;

        nvgBeginPath(vg);
        nvgMoveTo(vg, sx, sy);
        nvgLineTo(vg, ex, ey);
        nvgStroke(vg);

        if (directed_) {
            const float ah = 9.0f * fs;  // arrowhead length
            const float aw = 4.5f * fs;  // half-width
            const float px = -dy, py = dx;  // perpendicular
            nvgBeginPath(vg);
            nvgMoveTo(vg, ex, ey);
            nvgLineTo(vg, ex - dx * ah + px * aw, ey - dy * ah + py * aw);
            nvgLineTo(vg, ex - dx * ah - px * aw, ey - dy * ah - py * aw);
            nvgClosePath(vg);
            nvgFillColor(vg, link);
            nvgFill(vg);
        }

        if (!e.weight.empty()) {
            nvgFontSize(vg, std::max(9.0f, 11.0f * fs));
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGB(180, 190, 205));
            nvgText(vg, (x1 + x2) / 2.0f, (y1 + y2) / 2.0f, e.weight.c_str(), nullptr);
        }
    }

    // Nodes.
    for (int i = 0; i < n; i++) {
        NVGcolor target = base;
        auto it = highlights_.find(i);
        if (it != highlights_.end()) {
            const std::string& s = it->second;
            if (s == "swap")         target = nvgRGB(220, 70, 70);
            else if (s == "compare") target = nvgRGB(230, 180, 50);
            else if (s == "sorted")  target = nvgRGB(70, 170, 90);
            else if (s == "active")  target = nvgRGB(60, 130, 220);
        }
        const bool highlighted = (it != highlights_.end());
        const NVGcolor fill = highlighted ? lerpColor(base, target, ease) : target;
        const float rr = highlighted ? r * (0.62f + 0.38f * ease) : r;

        nvgBeginPath(vg);
        nvgCircle(vg, nx[i], ny[i], rr);
        nvgFillColor(vg, fill);
        nvgFill(vg);
        nvgStrokeColor(vg, nvgRGB(20, 24, 32));
        nvgStrokeWidth(vg, 1.5f);
        nvgStroke(vg);

        nvgFontSize(vg, std::max(9.0f, 16.0f * fs));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGB(235, 235, 235));
        nvgText(vg, nx[i], ny[i], nodes_[i].c_str(), nullptr);
    }

    // Pointer labels, pushed radially outward from each node.
    nvgFontSize(vg, std::max(9.0f, 12.0f * fs));
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    std::unordered_map<int, int> slots;
    for (const auto& p : pointers_) {
        const int idx = p.second;
        if (idx < 0 || idx >= n) continue;
        float ox = nx[idx] - cx, oy = ny[idx] - cy;
        const float ol = std::sqrt(ox * ox + oy * oy);
        if (ol < 1.0f) { ox = 0.0f; oy = -1.0f; } else { ox /= ol; oy /= ol; }
        const int s = slots[idx]++;
        const float off = r + 11.0f + s * 13.0f;
        nvgFillColor(vg, nvgRGB(90, 200, 250));
        nvgText(vg, nx[idx] + ox * off, ny[idx] + oy * off, p.first.c_str(), nullptr);
    }
}
