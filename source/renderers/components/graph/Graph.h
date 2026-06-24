#pragma once

#include <vector>
#include <string>
#include <utility>
#include <unordered_map>
#include <chrono>

#include "../../BaseRenderer.h"
#include <nanovg.h>

struct GraphEdge {
    int from;
    int to;
    std::string weight;  // empty when unweighted
};

// General graph (directed or undirected, optionally weighted). Nodes are placed
// on a circle (deterministic, so the layout is stable across playback frames);
// edges are lines with arrowheads when directed. `pointers_` labels the current
// vertex (u/v/curr/...) by index.
class Graph : public BaseRenderer {
public:
    Graph(std::vector<std::string> nodes, std::vector<GraphEdge> edges, bool directed,
          std::unordered_map<int, std::string> highlights = {},
          std::vector<std::pair<std::string, int>> pointers = {})
        : nodes_(std::move(nodes)), edges_(std::move(edges)), directed_(directed),
          highlights_(std::move(highlights)), pointers_(std::move(pointers)) {}

    void render(NVGcontext* nvgContext, int width, int height) override;

private:
    std::vector<std::string> nodes_;
    std::vector<GraphEdge> edges_;
    bool directed_;
    std::unordered_map<int, std::string> highlights_;   // index -> state
    std::vector<std::pair<std::string, int>> pointers_; // label -> node index

    // When this frame's renderer was created — drives the per-step entrance tween.
    std::chrono::steady_clock::time_point birth_ = std::chrono::steady_clock::now();
};
