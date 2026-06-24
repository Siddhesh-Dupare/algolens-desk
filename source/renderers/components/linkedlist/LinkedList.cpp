#include "LinkedList.h"
#include <algorithm>
#include <cmath>

// Linear interpolation between two NanoVG colors (components are 0..1 floats).
static NVGcolor lerpColor(NVGcolor a, NVGcolor b, float t) {
    return nvgRGBAf(a.r + (b.r - a.r) * t,
                    a.g + (b.g - a.g) * t,
                    a.b + (b.b - a.b) * t,
                    a.a + (b.a - a.a) * t);
}

void LinkedList::render(NVGcontext* vg, int width, int height) {
    const int n = (int)data_.size();
    nvgFontFace(vg, "sans");

    if (n == 0) {
        nvgFontSize(vg, 13.0f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGB(120, 120, 130));
        nvgText(vg, width / 2.0f, height / 2.0f, "empty list", nullptr);
        return;
    }

    float nodeW = 58.0f, nodeH = 46.0f, arrow = 42.0f, nullW = 40.0f;
    const float margin = 24.0f;

    // Auto-fit: n nodes + n arrows (incl. the trailing one) + the "null" label.
    float total = n * nodeW + n * arrow + nullW;
    const float avail = (float)width - 2.0f * margin;
    if (avail > 0.0f && total > avail) {
        const float scale = avail / total;
        nodeW *= scale;
        arrow *= scale;
        nullW *= scale;
        nodeH = std::min(nodeH, nodeW);
        total = avail;
    }
    const float fs = nodeW / 58.0f;

    const float startX = (width - total) / 2.0f;
    const float y = (height - nodeH) / 2.0f;
    const float midY = y + nodeH / 2.0f;

    auto nodeX = [&](int i) { return startX + i * (nodeW + arrow); };

    // Per-step tween progress (0..1) since this frame's renderer was created.
    using namespace std::chrono;
    float t = duration<float>(steady_clock::now() - birth_).count() / 0.22f;  // ~220ms
    if (t > 1.0f) t = 1.0f;
    const float ease = 1.0f - std::pow(1.0f - t, 3.0f);  // easeOutCubic

    const NVGcolor base = nvgRGB(45, 55, 72);
    const NVGcolor link = nvgRGB(120, 130, 150);

    // Arrows (drawn first, behind the node boxes).
    nvgStrokeColor(vg, link);
    nvgStrokeWidth(vg, std::max(1.5f, 2.0f * fs));
    for (int i = 0; i < n; i++) {
        const float ax = nodeX(i) + nodeW;                 // right edge of node i
        const float bx = (i < n - 1) ? nodeX(i + 1) : ax + arrow;  // next node / null

        nvgBeginPath(vg);
        nvgMoveTo(vg, ax, midY);
        nvgLineTo(vg, bx - 6.0f * fs, midY);
        nvgStroke(vg);

        // Forward arrowhead (pointing right, into node i+1 or null).
        nvgBeginPath(vg);
        nvgMoveTo(vg, bx - 2.0f * fs, midY);
        nvgLineTo(vg, bx - 12.0f * fs, midY - 5.0f * fs);
        nvgLineTo(vg, bx - 12.0f * fs, midY + 5.0f * fs);
        nvgClosePath(vg);
        nvgFillColor(vg, link);
        nvgFill(vg);

        // Doubly linked: a backward arrowhead at the source side.
        if (doubly_ && i < n - 1) {
            nvgBeginPath(vg);
            nvgMoveTo(vg, ax + 2.0f * fs, midY);
            nvgLineTo(vg, ax + 12.0f * fs, midY - 5.0f * fs);
            nvgLineTo(vg, ax + 12.0f * fs, midY + 5.0f * fs);
            nvgClosePath(vg);
            nvgFill(vg);
        }
    }

    // "null" terminator after the last node.
    nvgFontSize(vg, std::max(10.0f, 13.0f * fs));
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, nvgRGB(120, 120, 130));
    nvgText(vg, nodeX(n - 1) + nodeW + arrow + 2.0f * fs, midY, "null", nullptr);

    // Node boxes.
    for (int i = 0; i < n; i++) {
        const float x = nodeX(i);

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
        const float lift = highlighted ? (1.0f - ease) * 10.0f : 0.0f;
        const float yy = y - lift;

        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, yy, nodeW, nodeH, 6.0f * fs);
        nvgFillColor(vg, fill);
        nvgFill(vg);

        nvgFontSize(vg, 20.0f * fs);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGB(235, 235, 235));
        nvgText(vg, x + nodeW / 2.0f, yy + nodeH / 2.0f, data_[i].c_str(), nullptr);
    }

    // Pointer labels above the nodes (stack upward when several share a node).
    nvgFontSize(vg, std::max(10.0f, 14.0f * fs));
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    std::unordered_map<int, int> slots;
    for (const auto& p : pointers_) {
        const int idx = p.second;
        if (idx < 0 || idx >= n) continue;
        const float x = nodeX(idx) + nodeW / 2.0f;
        const int slot = slots[idx]++;
        const float ly = y - 18.0f - slot * 16.0f;
        nvgFillColor(vg, nvgRGB(90, 200, 250));  // cyan
        nvgText(vg, x, ly, p.first.c_str(), nullptr);
    }
}
