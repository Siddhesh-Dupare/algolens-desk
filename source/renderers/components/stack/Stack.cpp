#include "Stack.h"
#include <algorithm>
#include <cmath>

// Linear interpolation between two NanoVG colors (components are 0..1 floats).
static NVGcolor lerpColor(NVGcolor a, NVGcolor b, float t) {
    return nvgRGBAf(a.r + (b.r - a.r) * t,
                    a.g + (b.g - a.g) * t,
                    a.b + (b.b - a.b) * t,
                    a.a + (b.a - a.a) * t);
}

void Stack::render(NVGcontext* vg, int width, int height) {
    const int n = (int)data_.size();

    float boxW = 130.0f, boxH = 46.0f, gap = 6.0f;
    const float margin = 36.0f;  // leaves room for top/bottom side labels

    // Auto-fit the column height to the panel.
    const float availH = (float)height - 2.0f * margin;
    float total = n * boxH + (n - 1) * gap;
    if (n > 0 && total > availH) {
        const float scale = availH / total;
        boxH *= scale;
        gap *= scale;
        boxW *= std::max(scale, 0.6f);
        total = n * boxH + (n - 1) * gap;
    }
    const float fs = boxH / 46.0f;

    const float startX = (width - boxW) / 2.0f;
    const float blockTop = (height - total) / 2.0f;  // y of the top-most box

    // Per-step tween progress (0..1) since this frame's renderer was created.
    using namespace std::chrono;
    float t = duration<float>(steady_clock::now() - birth_).count() / 0.22f;  // ~220ms
    if (t > 1.0f) t = 1.0f;
    const float ease = 1.0f - std::pow(1.0f - t, 3.0f);  // easeOutCubic

    const NVGcolor base = nvgRGB(45, 55, 72);
    nvgFontFace(vg, "sans");

    if (n == 0) {
        // Empty stack: show a thin base line so the structure is still visible.
        nvgBeginPath(vg);
        nvgRect(vg, startX, height / 2.0f, boxW, 3.0f);
        nvgFillColor(vg, nvgRGB(70, 80, 100));
        nvgFill(vg);
        nvgFontSize(vg, 13.0f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
        nvgFillColor(vg, nvgRGB(120, 120, 130));
        nvgText(vg, width / 2.0f, height / 2.0f + 8.0f, "empty stack", nullptr);
        return;
    }

    float topBoxY = blockTop;  // resting y of the top box (tracked for the label)

    for (int i = 0; i < n; i++) {
        // index 0 = bottom, so it is drawn at the lowest row.
        const float y = blockTop + (n - 1 - i) * (boxH + gap);

        NVGcolor target = base;
        std::string state;
        auto it = highlights_.find(i);
        if (it != highlights_.end()) {
            state = it->second;
            if (state == "swap")         target = nvgRGB(220, 70, 70);
            else if (state == "compare") target = nvgRGB(230, 180, 50);
            else if (state == "sorted")  target = nvgRGB(70, 170, 90);
            else if (state == "active")  target = nvgRGB(60, 130, 220);
        }
        const bool highlighted = (it != highlights_.end());

        // Push (swap) drops the new top in from above and fades in. A resting
        // highlight (active top) just settles down a few pixels, like the array.
        float lift = 0.0f, alpha = 1.0f;
        if (state == "swap") {
            lift = (1.0f - ease) * (boxH + gap + 24.0f);
            alpha = 0.25f + 0.75f * ease;
        } else if (highlighted) {
            lift = (1.0f - ease) * 10.0f;
        }
        const float drawY = y - lift;  // start above, settle to y
        if (i == n - 1) topBoxY = drawY;

        NVGcolor fill = highlighted ? lerpColor(base, target, ease) : target;
        fill.a *= alpha;

        nvgBeginPath(vg);
        nvgRoundedRect(vg, startX, drawY, boxW, boxH, 6.0f * fs);
        nvgFillColor(vg, fill);
        nvgFill(vg);

        nvgFontSize(vg, 20.0f * fs);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        NVGcolor txt = nvgRGB(235, 235, 235);
        txt.a *= alpha;
        nvgFillColor(vg, txt);
        nvgText(vg, startX + boxW / 2.0f, drawY + boxH / 2.0f, data_[i].c_str(), nullptr);
    }

    // Side labels: "top" tracks the (possibly animating) top box; "bottom" rests.
    nvgFontSize(vg, std::max(11.0f, 14.0f * fs));
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    const float labelX = startX + boxW + 12.0f;

    nvgFillColor(vg, nvgRGB(90, 200, 250));
    nvgText(vg, labelX, topBoxY + boxH / 2.0f, "<- top", nullptr);

    const float bottomY = blockTop + (n - 1) * (boxH + gap) + boxH / 2.0f;
    nvgFillColor(vg, nvgRGB(120, 120, 130));
    nvgText(vg, labelX, bottomY, "<- bottom", nullptr);
}
