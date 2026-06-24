#include "Queue.h"
#include <algorithm>
#include <cmath>

// Linear interpolation between two NanoVG colors (components are 0..1 floats).
static NVGcolor lerpColor(NVGcolor a, NVGcolor b, float t) {
    return nvgRGBAf(a.r + (b.r - a.r) * t,
                    a.g + (b.g - a.g) * t,
                    a.b + (b.b - a.b) * t,
                    a.a + (b.a - a.a) * t);
}

void Queue::render(NVGcontext* vg, int width, int height) {
    const int n = (int)data_.size();

    float boxW = 60.0f, boxH = 60.0f, gap = 10.0f;
    const float margin = 30.0f;

    nvgFontFace(vg, "sans");

    if (n == 0) {
        nvgFontSize(vg, 13.0f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGB(120, 120, 130));
        nvgText(vg, width / 2.0f, height / 2.0f, "empty queue", nullptr);
        return;
    }

    // Auto-fit: shrink boxes + gap so the whole row fits the panel width.
    const float avail = (float)width - 2.0f * margin;
    float total = n * boxW + (n - 1) * gap;
    if (avail > 0.0f && total > avail) {
        const float scale = avail / total;
        boxW *= scale;
        boxH = boxW;  // keep square
        gap *= scale;
        total = avail;
    }
    const float fs = boxW / 60.0f;

    const float startX = (width - total) / 2.0f;
    const float startY = (height - boxH) / 2.0f;

    // Per-step tween progress (0..1) since this frame's renderer was created.
    using namespace std::chrono;
    float t = duration<float>(steady_clock::now() - birth_).count() / 0.22f;  // ~220ms
    if (t > 1.0f) t = 1.0f;
    const float ease = 1.0f - std::pow(1.0f - t, 3.0f);  // easeOutCubic

    const NVGcolor base = nvgRGB(45, 55, 72);

    for (int i = 0; i < n; i++) {
        const float x = startX + i * (boxW + gap);

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

        // Enqueue (swap) slides the new rear in from the right and fades in. A
        // resting highlight (active front) just settles up a few pixels.
        float slideX = 0.0f, liftY = 0.0f, alpha = 1.0f;
        if (state == "swap") {
            slideX = (1.0f - ease) * (boxW + gap + 24.0f);
            alpha = 0.25f + 0.75f * ease;
        } else if (highlighted) {
            liftY = (1.0f - ease) * 10.0f;
        }
        const float drawX = x + slideX;     // start right, settle to x
        const float drawY = startY - liftY;

        NVGcolor fill = highlighted ? lerpColor(base, target, ease) : target;
        fill.a *= alpha;

        nvgBeginPath(vg);
        nvgRoundedRect(vg, drawX, drawY, boxW, boxH, 6.0f * fs);
        nvgFillColor(vg, fill);
        nvgFill(vg);

        nvgFontSize(vg, 22.0f * fs);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        NVGcolor txt = nvgRGB(235, 235, 235);
        txt.a *= alpha;
        nvgFillColor(vg, txt);
        nvgText(vg, drawX + boxW / 2.0f, drawY + boxH / 2.0f, data_[i].c_str(), nullptr);
    }

    // "front" under the dequeue end, "rear" under the enqueue end (resting).
    nvgFontSize(vg, std::max(11.0f, 14.0f * fs));
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
    const float labelY = startY + boxH + 10.0f * fs;

    if (front_ >= 0 && front_ < n) {
        const float fx = startX + front_ * (boxW + gap) + boxW / 2.0f;
        nvgFillColor(vg, nvgRGB(90, 200, 250));
        nvgText(vg, fx, labelY, "front", nullptr);
    }
    if (rear_ >= 0 && rear_ < n && rear_ != front_) {
        const float rx = startX + rear_ * (boxW + gap) + boxW / 2.0f;
        nvgFillColor(vg, nvgRGB(120, 120, 130));
        nvgText(vg, rx, labelY, "rear", nullptr);
    }
}
