#include "HashMap.h"
#include <algorithm>
#include <cmath>

static NVGcolor lerpColor(NVGcolor a, NVGcolor b, float t) {
    return nvgRGBAf(a.r + (b.r - a.r) * t,
                    a.g + (b.g - a.g) * t,
                    a.b + (b.b - a.b) * t,
                    a.a + (b.a - a.a) * t);
}

void HashMap::render(NVGcontext* vg, int width, int height) {
    const int n = (int)entries_.size();
    nvgFontFace(vg, "sans");

    if (n == 0) {
        nvgFontSize(vg, 13.0f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGB(120, 120, 130));
        nvgText(vg, width / 2.0f, height / 2.0f, "empty map", nullptr);
        return;
    }

    const float margin = 20.0f;
    float tableW = std::min((float)width - 2.0f * margin, 360.0f);
    if (tableW < 120.0f) tableW = (float)width - 2.0f * margin;

    float rowH = 32.0f, gap = 6.0f, headerH = 22.0f;
    const float availH = (float)height - 2.0f * margin;
    float total = headerH + n * rowH + (n - 1) * gap;
    if (total > availH) {
        const float scale = availH / total;
        rowH *= scale;
        gap *= scale;
        headerH *= scale;
        total = availH;
    }
    const float fs = rowH / 32.0f;

    const float startX = (width - tableW) / 2.0f;
    const float startY = (height - total) / 2.0f;
    const float cellGap = 6.0f * fs;
    const float keyW = tableW * 0.46f;
    const float valX = startX + tableW * 0.5f + cellGap * 0.5f;
    const float valW = tableW * 0.5f - cellGap * 0.5f;

    // Header.
    nvgFontSize(vg, std::max(9.0f, 12.0f * fs));
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, nvgRGB(120, 120, 130));
    nvgText(vg, startX + 6.0f, startY + headerH / 2.0f, "key", nullptr);
    nvgText(vg, valX + 6.0f, startY + headerH / 2.0f, "value", nullptr);

    using namespace std::chrono;
    float t = duration<float>(steady_clock::now() - birth_).count() / 0.22f;
    if (t > 1.0f) t = 1.0f;
    const float ease = 1.0f - std::pow(1.0f - t, 3.0f);

    const NVGcolor base = nvgRGB(45, 55, 72);

    for (int i = 0; i < n; i++) {
        const float y = startY + headerH + i * (rowH + gap);

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
        const float lift = highlighted ? (1.0f - ease) * 8.0f : 0.0f;
        const float ry = y - lift;

        // Key cell.
        nvgBeginPath(vg);
        nvgRoundedRect(vg, startX, ry, keyW, rowH, 5.0f * fs);
        nvgFillColor(vg, fill);
        nvgFill(vg);
        // Value cell (slightly darker when not highlighted).
        nvgBeginPath(vg);
        nvgRoundedRect(vg, valX, ry, valW, rowH, 5.0f * fs);
        nvgFillColor(vg, highlighted ? fill : nvgRGB(38, 46, 60));
        nvgFill(vg);

        nvgFontSize(vg, std::max(10.0f, 16.0f * fs));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGB(235, 235, 235));
        nvgText(vg, startX + keyW / 2.0f, ry + rowH / 2.0f, entries_[i].first.c_str(), nullptr);
        nvgText(vg, valX + valW / 2.0f, ry + rowH / 2.0f, entries_[i].second.c_str(), nullptr);
    }
}
