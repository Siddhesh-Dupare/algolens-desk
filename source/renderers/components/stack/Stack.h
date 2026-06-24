#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>

#include "../../BaseRenderer.h"
#include <nanovg.h>

// LIFO stack drawn as a vertical column. Index 0 is the bottom; `top_` is the
// index of the top element (where push/pop happen), or -1 when empty.
class Stack : public BaseRenderer {
public:
    Stack(std::vector<std::string> data,
          std::unordered_map<int, std::string> highlights = {},
          int top = -1)
        : data_(std::move(data)),
          highlights_(std::move(highlights)),
          top_(top) {}

    void render(NVGcontext* nvgContext, int width, int height) override;

private:
    std::vector<std::string> data_;                    // index 0 = bottom
    std::unordered_map<int, std::string> highlights_;  // index -> state
    int top_;                                          // top index, -1 if empty

    // When this frame's renderer was created — drives the per-step entrance tween.
    std::chrono::steady_clock::time_point birth_ = std::chrono::steady_clock::now();
};
