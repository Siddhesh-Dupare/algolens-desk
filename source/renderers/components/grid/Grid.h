#pragma once

#include <vector>
#include <string>
#include <utility>
#include <unordered_map>

#include "../../BaseRenderer.h"
#include <nanovg.h>

// 2-D matrix / grid (DP tables, matrices, grid traversal). Cells hold display
// strings; highlights are keyed by "row,col"; pointers map a name -> "row,col".
class Grid : public BaseRenderer {
public:
    Grid(std::vector<std::vector<std::string>> data,
         std::unordered_map<std::string, std::string> highlights = {},
         std::vector<std::pair<std::string, std::string>> pointers = {})
        : data_(std::move(data)),
          highlights_(std::move(highlights)),
          pointers_(std::move(pointers)) {}

    void render(NVGcontext* nvgContext, int width, int height) override;

private:
    std::vector<std::vector<std::string>> data_;
    std::unordered_map<std::string, std::string> highlights_;   // "r,c" -> state
    std::vector<std::pair<std::string, std::string>> pointers_; // name -> "r,c"
};
