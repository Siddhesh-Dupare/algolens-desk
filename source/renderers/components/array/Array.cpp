#include "Array.h"
#include <string>
#include <cmath>
#include <algorithm>

// Linear interpolation between two NanoVG colors (components are 0..1 floats).
static NVGcolor lerpColor(NVGcolor a, NVGcolor b, float t) {
    return nvgRGBAf(a.r + (b.r - a.r) * t,
                    a.g + (b.g - a.g) * t,
                    a.b + (b.b - a.b) * t,
                    a.a + (b.a - a.a) * t);
}

void Array::render(NVGcontext* vg, int width, int height) {
    const int n = (int)data_.size();
    if (n == 0) return;

    // Auto-fit: shrink box width + gap so the whole array fits the panel width.
    float boxW = 60.0f, gap = 12.0f;
    const float margin = 24.0f;
    const float avail = (float)width - 2.0f * margin;
    float total = n * boxW + (n - 1) * gap;
    if (avail > 0.0f && total > avail) {
        const float scale = avail / total;
        boxW *= scale;
        gap *= scale;
        total = avail;
    }
    const float boxH = boxW < 60.0f ? boxW : 60.0f;  // square when shrunk
    const float fs = boxW / 60.0f;                    // font/scale factor

    const float startX = (width - total) / 2.0f;
    const float startY = (height - boxH) / 2.0f;

    // Per-step tween progress (0..1) since this frame's renderer was created.
    using namespace std::chrono;
    float t = duration<float>(steady_clock::now() - birth_).count() / 0.22f;  // ~220ms
    if (t > 1.0f) t = 1.0f;
    const float ease = 1.0f - std::pow(1.0f - t, 3.0f);  // easeOutCubic

    const NVGcolor base = nvgRGB(45, 55, 72);
    nvgFontFace(vg, "sans");

    for (int i = 0; i < n; i++) {
        const float x = startX + i * (boxW + gap);

        // Target fill color by highlight state.
        NVGcolor target = base;
        auto it = highlights_.find(i);
        if (it != highlights_.end()) {
            const std::string& s = it->second;
            if (s == "swap")         target = nvgRGB(220, 70, 70);    // red
            else if (s == "compare") target = nvgRGB(230, 180, 50);   // amber
            else if (s == "sorted")  target = nvgRGB(70, 170, 90);    // green
            else if (s == "active")  target = nvgRGB(60, 130, 220);   // blue
        }
        const bool highlighted = (it != highlights_.end());

        // Tween: highlighted boxes fade their color in and settle from a small lift.
        const NVGcolor fill = highlighted ? lerpColor(base, target, ease) : target;
        const float lift = highlighted ? (1.0f - ease) * 10.0f : 0.0f;
        const float y = startY - lift;

        // Box.
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, boxW, boxH, 6.0f * fs);
        nvgFillColor(vg, fill);
        nvgFill(vg);

        // Value (moves with the box).
        nvgFontSize(vg, 22.0f * fs);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGB(235, 235, 235));
        nvgText(vg, x + boxW / 2.0f, y + boxH / 2.0f,
                std::to_string(data_[i]).c_str(), nullptr);

        // Index label below (fixed baseline, doesn't bounce).
        nvgFontSize(vg, std::max(9.0f, 13.0f * fs));
        nvgFillColor(vg, nvgRGB(120, 120, 130));
        nvgText(vg, x + boxW / 2.0f, startY + boxH + 14.0f * fs,
                std::to_string(i).c_str(), nullptr);
    }

    // Pointer labels above the boxes (stack upward when several share an index).
    nvgFontSize(vg, std::max(10.0f, 15.0f * fs));
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    std::unordered_map<int, int> slots;  // index -> labels already placed there
    for (const auto& p : pointers_) {
        const int idx = p.second;
        if (idx < 0 || idx >= n) continue;
        const float x = startX + idx * (boxW + gap) + boxW / 2.0f;
        const int slot = slots[idx]++;
        const float y = startY - 18.0f - slot * 18.0f;
        nvgFillColor(vg, nvgRGB(90, 200, 250));  // cyan
        nvgText(vg, x, y, p.first.c_str(), nullptr);
    }
}
