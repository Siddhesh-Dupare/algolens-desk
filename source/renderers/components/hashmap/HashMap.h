#pragma once

#include <vector>
#include <string>
#include <utility>
#include <unordered_map>
#include <chrono>

#include "../../BaseRenderer.h"
#include <nanovg.h>

// Hash map / dictionary drawn as a key -> value table. Rows whose entry was just
// added/changed are highlighted.
class HashMap : public BaseRenderer {
public:
    HashMap(std::vector<std::pair<std::string, std::string>> entries,
            std::unordered_map<int, std::string> highlights = {})
        : entries_(std::move(entries)), highlights_(std::move(highlights)) {}

    void render(NVGcontext* nvgContext, int width, int height) override;

private:
    std::vector<std::pair<std::string, std::string>> entries_;  // key -> value
    std::unordered_map<int, std::string> highlights_;           // row index -> state

    // When this frame's renderer was created — drives the per-step entrance tween.
    std::chrono::steady_clock::time_point birth_ = std::chrono::steady_clock::now();
};
