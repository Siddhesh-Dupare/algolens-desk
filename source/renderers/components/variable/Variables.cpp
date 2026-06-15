#include "Variables.h"

void Variables::render(NVGcontext* nvgContext, int width, int height) {
    float x = 20;
    float y = 20;
    float rowH = 20;

    nvgBeginPath(nvgContext);
    nvgRoundedRect(nvgContext, 12, 12, 40, 40, 8);
    nvgFillColor(nvgContext, nvgRGBA(30, 34, 44, 200));
    nvgFill(nvgContext);

    nvgFontSize(nvgContext, 18);
    nvgFontFace(nvgContext, "sans");
    nvgTextAlign(nvgContext, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

    for (size_t i = 0; i < vars_.size(); i++) {
        float ry = y + i * rowH;

        std::string line = vars_[i].name + " = " + vars_[i].value;

        nvgFillColor(nvgContext, nvgRGB(210, 215, 255));
        nvgText(nvgContext, x, ry, line.c_str(), nullptr);
    }
}
