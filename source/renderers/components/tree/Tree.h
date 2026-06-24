#pragma once

#include <vector>
#include <string>
#include <utility>
#include <unordered_map>
#include <chrono>

#include "../../BaseRenderer.h"
#include <nanovg.h>

struct TreeNodeData {
    std::string value;
    std::vector<int> children;  // child indices; binary nodes use [left, right] with -1
};

// Binary / BST / n-ary tree (also AST). Nodes are positioned by depth (rows) and
// an in-order column layout (binary) or leaf-centering (n-ary); edges join
// parents to children. `pointers_` labels nodes (root/curr/...) by index.
class Tree : public BaseRenderer {
public:
    Tree(std::vector<TreeNodeData> nodes, bool binary,
         std::unordered_map<int, std::string> highlights = {},
         std::vector<std::pair<std::string, int>> pointers = {})
        : nodes_(std::move(nodes)), binary_(binary),
          highlights_(std::move(highlights)), pointers_(std::move(pointers)) {}

    void render(NVGcontext* nvgContext, int width, int height) override;

private:
    std::vector<TreeNodeData> nodes_;
    bool binary_;
    std::unordered_map<int, std::string> highlights_;   // index -> state
    std::vector<std::pair<std::string, int>> pointers_; // label -> node index

    // When this frame's renderer was created — drives the per-step entrance tween.
    std::chrono::steady_clock::time_point birth_ = std::chrono::steady_clock::now();
};
