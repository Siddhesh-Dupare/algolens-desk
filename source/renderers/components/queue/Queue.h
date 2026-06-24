#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>

#include "../../BaseRenderer.h"
#include <nanovg.h>

// FIFO queue drawn as a horizontal row. `front_` is the dequeue end (index 0),
// `rear_` is the enqueue end (last index); both are -1 when empty.
class Queue : public BaseRenderer {
public:
    Queue(std::vector<std::string> data,
          std::unordered_map<int, std::string> highlights = {},
          int front = -1, int rear = -1)
        : data_(std::move(data)),
          highlights_(std::move(highlights)),
          front_(front), rear_(rear) {}

    void render(NVGcontext* nvgContext, int width, int height) override;

private:
    std::vector<std::string> data_;
    std::unordered_map<int, std::string> highlights_;  // index -> state
    int front_;
    int rear_;

    // When this frame's renderer was created — drives the per-step entrance tween.
    std::chrono::steady_clock::time_point birth_ = std::chrono::steady_clock::now();
};
