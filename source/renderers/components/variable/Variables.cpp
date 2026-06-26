#include "Variables.h"
#include <algorithm>
#include <string>

// Is the string a plain integer/float literal?
static bool isNumber(const std::string& s) {
    if (s.empty()) return false;
    bool dot = false, digit = false;
    size_t i = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    for (; i < s.size(); i++) {
        if (s[i] >= '0' && s[i] <= '9') digit = true;
        else if (s[i] == '.' && !dot) dot = true;
        else return false;
    }
    return digit;
}

void Variables::render(NVGcontext* vg, int width, int height) {
    if (vars_.empty()) return;

    nvgFontFace(vg, "sans");

    const float x0 = 16.0f, y0 = 16.0f;
    const float pad = 12.0f;
    const float headerH = 20.0f;
    const float rowH = 24.0f;
    const float nameSize = 13.0f, valSize = 13.0f;
    const float gap = 10.0f, chipPadX = 7.0f;

    // Truncate over-long values so the panel stays compact.
    auto clip = [](const std::string& s) {
        return s.size() > 32 ? s.substr(0, 31) + "..." : s;
    };

    // Measure to size the panel.
    float b[4];
    float maxNameW = 0.0f, maxValW = 0.0f;
    nvgFontSize(vg, nameSize);
    for (auto& v : vars_) {
        nvgTextBounds(vg, 0, 0, v.name.c_str(), nullptr, b);
        maxNameW = std::max(maxNameW, b[2] - b[0]);
    }
    nvgFontSize(vg, valSize);
    for (auto& v : vars_) {
        const std::string d = clip(v.value);
        nvgTextBounds(vg, 0, 0, d.c_str(), nullptr, b);
        maxValW = std::max(maxValW, b[2] - b[0]);
    }

    const float panelW = maxNameW + gap + maxValW + 2.0f * chipPadX + 2.0f * pad;
    const float panelH = headerH + (float)vars_.size() * rowH + pad;

    // Panel.
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x0, y0, panelW, panelH, 10.0f);
    nvgFillColor(vg, nvgRGBA(22, 25, 31, 225));
    nvgFill(vg);
    nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 20));
    nvgStrokeWidth(vg, 1.0f);
    nvgStroke(vg);

    // Header.
    nvgFontSize(vg, 10.0f);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, nvgRGB(118, 124, 138));
    nvgText(vg, x0 + pad, y0 + headerH * 0.55f, "VARIABLES", nullptr);

    const float valX = x0 + pad + maxNameW + gap;
    const float rowsTop = y0 + headerH;

    for (size_t i = 0; i < vars_.size(); i++) {
        const auto& v = vars_[i];
        const float cy = rowsTop + (float)i * rowH + rowH * 0.5f;
        const std::string val = clip(v.value);

        // Name (muted).
        nvgFontSize(vg, nameSize);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGB(150, 156, 170));
        nvgText(vg, x0 + pad, cy, v.name.c_str(), nullptr);

        // Value colour by inferred type.
        NVGcolor valColor = nvgRGB(220, 225, 235);
        if (!val.empty() && (val.front() == '\'' || val.front() == '"'))
            valColor = nvgRGB(150, 200, 120);                 // string -> green
        else if (val == "True" || val == "False" || val == "true" || val == "false")
            valColor = nvgRGB(200, 150, 235);                 // bool -> purple
        else if (val == "None" || val == "null" || val == "nullptr")
            valColor = nvgRGB(130, 135, 150);                 // none -> gray
        else if (isNumber(val))
            valColor = nvgRGB(225, 165, 95);                  // number -> orange

        // Value chip.
        nvgFontSize(vg, valSize);
        nvgTextBounds(vg, 0, 0, val.c_str(), nullptr, b);
        const float vw = b[2] - b[0];
        nvgBeginPath(vg);
        nvgRoundedRect(vg, valX - chipPadX * 0.5f, cy - rowH * 0.5f + 3.0f,
                       vw + chipPadX, rowH - 6.0f, 5.0f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 12));
        nvgFill(vg);

        nvgFillColor(vg, valColor);
        nvgText(vg, valX, cy, val.c_str(), nullptr);
    }
}
