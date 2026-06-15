#include "Array.h"
#include <string>

void Array::render(NVGcontext* nvgContext, int width, int height) {
    int boxW = 60;
    int boxH = 60;
    int gap = 12;

    int n = (int)data_.size();

    int total = n * boxW + (n - 1) * gap;

    // Center it horizontally
    float startX = (width - total) / 2.0f;
    float startY = (height - boxH) / 2.0f;

    for (size_t i = 0; i < data_.size(); i++) {
        int x = startX + i * (boxW + gap);

        nvgBeginPath(nvgContext);
        nvgRoundedRect(nvgContext, x, startY, boxW, boxH, 6);
        nvgFillColor(nvgContext, nvgRGB(45, 55, 72));
        nvgFill(nvgContext);

        nvgFontSize(nvgContext, 22);
        nvgFontFace(nvgContext, "sans");
        nvgTextAlign(nvgContext, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(nvgContext, nvgRGB(235, 235, 235));

        std::string s = std::to_string(data_[i]);

        nvgText(nvgContext, x + boxW / 2.0f, startY + boxH / 2.0f, s.c_str(), nullptr);
    }
}
