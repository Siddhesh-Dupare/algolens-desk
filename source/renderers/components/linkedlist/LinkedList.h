#pragma once

#include <vector>
#include <string>
#include <utility>
#include <unordered_map>
#include <chrono>

#include "../../BaseRenderer.h"
#include <nanovg.h>

// Singly/doubly linked list: a horizontal chain of node boxes joined by arrows,
// ending in "null". `pointers_` labels nodes (head/curr/prev/...) by index.
class LinkedList : public BaseRenderer {
public:
    LinkedList(std::vector<std::string> data,
               std::unordered_map<int, std::string> highlights = {},
               std::vector<std::pair<std::string, int>> pointers = {},
               bool doubly = false)
        : data_(std::move(data)),
          highlights_(std::move(highlights)),
          pointers_(std::move(pointers)),
          doubly_(doubly) {}

    void render(NVGcontext* nvgContext, int width, int height) override;

private:
    std::vector<std::string> data_;                     // node display values
    std::unordered_map<int, std::string> highlights_;   // index -> state
    std::vector<std::pair<std::string, int>> pointers_; // label -> node index
    bool doubly_;

    // When this frame's renderer was created — drives the per-step entrance tween.
    std::chrono::steady_clock::time_point birth_ = std::chrono::steady_clock::now();
};
