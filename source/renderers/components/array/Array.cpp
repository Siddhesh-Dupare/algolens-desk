#include "Array.h"
#include <string>

void Array::render(NVGcontext* vg, int width, int height) {
    const int boxW = 60, boxH = 60, gap = 12;
    const int n = (int)data_.size();
    if (n == 0) return;

    const int total = n * boxW + (n - 1) * gap;
    const float startX = (width - total) / 2.0f;
    const float startY = (height - boxH) / 2.0f;

    nvgFontFace(vg, "sans");

    for (int i = 0; i < n; i++) {
        const float x = startX + i * (boxW + gap);

        // Fill color by highlight state (default = dark slate).
        NVGcolor fill = nvgRGB(45, 55, 72);
        auto it = highlights_.find(i);
        if (it != highlights_.end()) {
            const std::string& s = it->second;
            if (s == "swap")         fill = nvgRGB(220, 70, 70);    // red
            else if (s == "compare") fill = nvgRGB(230, 180, 50);   // amber
            else if (s == "sorted")  fill = nvgRGB(70, 170, 90);    // green
            else if (s == "active")  fill = nvgRGB(60, 130, 220);   // blue
        }

        // Box.
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, startY, (float)boxW, (float)boxH, 6);
        nvgFillColor(vg, fill);
        nvgFill(vg);

        // Value.
        nvgFontSize(vg, 22);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGB(235, 235, 235));
        nvgText(vg, x + boxW / 2.0f, startY + boxH / 2.0f,
                std::to_string(data_[i]).c_str(), nullptr);

        // Index label below the box.
        nvgFontSize(vg, 13);
        nvgFillColor(vg, nvgRGB(120, 120, 130));
        nvgText(vg, x + boxW / 2.0f, startY + boxH + 14.0f,
                std::to_string(i).c_str(), nullptr);
    }

    // Pointer labels above the boxes (stack upward when several share an index).
    nvgFontSize(vg, 15);
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
