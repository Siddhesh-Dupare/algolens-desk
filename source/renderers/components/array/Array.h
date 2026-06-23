#pragma once

#include <vector>
#include <string>
#include <utility>
#include <unordered_map>
#include <chrono>

#include "../../BaseRenderer.h"
#include <nanovg.h>

class Array : public BaseRenderer {
public:
    // values + optional per-element highlight states + optional named pointers.
    Array(std::vector<int> data,
          std::unordered_map<int, std::string> highlights = {},
          std::vector<std::pair<std::string, int>> pointers = {})
        : data_(std::move(data)),
          highlights_(std::move(highlights)),
          pointers_(std::move(pointers)) {}

    void render(NVGcontext* nvgContext, int width, int height) override;

private:
    std::vector<int> data_;
    std::unordered_map<int, std::string> highlights_;    // index -> state (compare/swap/sorted/active)
    std::vector<std::pair<std::string, int>> pointers_;  // pointer name -> index

    // When this frame's renderer was created — drives the per-step entrance tween.
    std::chrono::steady_clock::time_point birth_ = std::chrono::steady_clock::now();
};
