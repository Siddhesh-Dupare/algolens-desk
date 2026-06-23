#include "Grid.h"
#include <string>
#include <algorithm>
#include <cstdlib>

void Grid::render(NVGcontext* vg, int width, int height) {
    const int rows = (int)data_.size();
    if (rows == 0) return;
    int cols = 0;
    for (auto& r : data_) cols = std::max(cols, (int)r.size());
    if (cols == 0) return;

    const float margin = 30.0f;  // leaves room for row/col index labels
    const float gap = 4.0f;
    const float maxCell = 64.0f;

    const float availW = (float)width - 2.0f * margin;
    const float availH = (float)height - 2.0f * margin;
    float cell = std::min({ (availW - (cols - 1) * gap) / cols,
                            (availH - (rows - 1) * gap) / rows,
                            maxCell });
    if (cell < 8.0f) cell = 8.0f;

    const float gridW = cols * cell + (cols - 1) * gap;
    const float gridH = rows * cell + (rows - 1) * gap;
    const float startX = (width - gridW) / 2.0f;
    const float startY = (height - gridH) / 2.0f;
    const float fs = cell / 64.0f;

    nvgFontFace(vg, "sans");

    // Column index labels (top).
    nvgFontSize(vg, std::max(9.0f, 12.0f * fs));
    nvgFillColor(vg, nvgRGB(120, 120, 130));
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
    for (int c = 0; c < cols; c++) {
        const float cx = startX + c * (cell + gap) + cell / 2.0f;
        nvgText(vg, cx, startY - 4.0f, std::to_string(c).c_str(), nullptr);
    }
    // Row index labels (left).
    nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
    for (int r = 0; r < rows; r++) {
        const float cy = startY + r * (cell + gap) + cell / 2.0f;
        nvgText(vg, startX - 6.0f, cy, std::to_string(r).c_str(), nullptr);
    }

    // Cells.
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < (int)data_[r].size(); c++) {
            const float x = startX + c * (cell + gap);
            const float y = startY + r * (cell + gap);

            NVGcolor fill = nvgRGB(45, 55, 72);
            auto it = highlights_.find(std::to_string(r) + "," + std::to_string(c));
            if (it != highlights_.end()) {
                const std::string& s = it->second;
                if (s == "swap")         fill = nvgRGB(220, 70, 70);
                else if (s == "compare") fill = nvgRGB(230, 180, 50);
                else if (s == "sorted")  fill = nvgRGB(70, 170, 90);
                else if (s == "active")  fill = nvgRGB(60, 130, 220);
            }

            nvgBeginPath(vg);
            nvgRoundedRect(vg, x, y, cell, cell, 4.0f * fs);
            nvgFillColor(vg, fill);
            nvgFill(vg);

            nvgFontSize(vg, std::max(9.0f, 18.0f * fs));
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGB(235, 235, 235));
            nvgText(vg, x + cell / 2.0f, y + cell / 2.0f, data_[r][c].c_str(), nullptr);
        }
    }

    // Pointer labels (name in the pointed cell's top-left), if any.
    nvgFontSize(vg, std::max(9.0f, 11.0f * fs));
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgFillColor(vg, nvgRGB(90, 200, 250));
    for (const auto& p : pointers_) {
        const std::string& rc = p.second;
        const auto comma = rc.find(',');
        if (comma == std::string::npos) continue;
        const int r = std::atoi(rc.substr(0, comma).c_str());
        const int c = std::atoi(rc.substr(comma + 1).c_str());
        if (r < 0 || r >= rows || c < 0 || c >= cols) continue;
        const float x = startX + c * (cell + gap);
        const float y = startY + r * (cell + gap);
        nvgText(vg, x + 3.0f, y + 2.0f, p.first.c_str(), nullptr);
    }
}
