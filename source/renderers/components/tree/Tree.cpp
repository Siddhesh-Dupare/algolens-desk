#include "Tree.h"
#include <algorithm>
#include <cmath>
#include <functional>

static NVGcolor lerpColor(NVGcolor a, NVGcolor b, float t) {
    return nvgRGBAf(a.r + (b.r - a.r) * t,
                    a.g + (b.g - a.g) * t,
                    a.b + (b.b - a.b) * t,
                    a.a + (b.a - a.a) * t);
}

void Tree::render(NVGcontext* vg, int width, int height) {
    const int n = (int)nodes_.size();
    nvgFontFace(vg, "sans");

    if (n == 0) {
        nvgFontSize(vg, 13.0f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGB(120, 120, 130));
        nvgText(vg, width / 2.0f, height / 2.0f, "empty tree", nullptr);
        return;
    }

    auto childAt = [&](int i, int k) -> int {
        if (i < 0 || i >= n || k >= (int)nodes_[i].children.size()) return -1;
        const int c = nodes_[i].children[k];
        return (c >= 0 && c < n) ? c : -1;
    };

    std::vector<float> slot(n, 0.0f);  // horizontal position in column units
    std::vector<int> depth(n, 0);
    std::vector<char> visited(n, 0);    // guards against cyclic children

    if (binary_) {
        // In-order column assignment: left subtree, node, right subtree.
        int counter = 0;
        std::function<void(int, int)> inorder = [&](int i, int d) {
            if (i < 0 || visited[i]) return;
            visited[i] = 1;
            depth[i] = d;
            inorder(childAt(i, 0), d + 1);
            slot[i] = (float)counter++;
            inorder(childAt(i, 1), d + 1);
        };
        inorder(0, 0);
    } else {
        // n-ary: leaves get sequential columns, parents center over their kids.
        int leaf = 0;
        std::function<void(int, int)> lay = [&](int i, int d) {
            if (i < 0 || visited[i]) return;
            visited[i] = 1;
            depth[i] = d;
            std::vector<int> kids;
            for (int c : nodes_[i].children)
                if (c >= 0 && c < n && !visited[c]) kids.push_back(c);
            if (kids.empty()) {
                slot[i] = (float)leaf++;
            } else {
                float sum = 0.0f;
                for (int c : kids) { lay(c, d + 1); sum += slot[c]; }
                slot[i] = sum / (float)kids.size();
            }
        };
        lay(0, 0);
    }

    float maxSlot = 0.0f;
    int maxDepth = 0;
    for (int i = 0; i < n; i++) {
        maxSlot = std::max(maxSlot, slot[i]);
        maxDepth = std::max(maxDepth, depth[i]);
    }

    const float margin = 34.0f;
    const float availW = (float)width - 2.0f * margin;
    const float availH = (float)height - 2.0f * margin;

    // Node radius from the tightest spacing, capped.
    const float colSpace = availW / (maxSlot + 1.0f);
    const float rowSpace = availH / (float)(maxDepth + 1);
    float r = std::min({ colSpace * 0.42f, rowSpace * 0.34f, 24.0f });
    if (r < 7.0f) r = 7.0f;
    const float fs = r / 24.0f;

    auto px = [&](int i) {
        const float fx = (maxSlot > 0.0f) ? slot[i] / maxSlot : 0.5f;
        return margin + fx * availW;
    };
    auto py = [&](int i) {
        const float fy = (maxDepth > 0) ? (float)depth[i] / (float)maxDepth : 0.5f;
        return margin + fy * availH;
    };

    // Per-step tween progress (0..1) since this frame's renderer was created.
    using namespace std::chrono;
    float t = duration<float>(steady_clock::now() - birth_).count() / 0.22f;
    if (t > 1.0f) t = 1.0f;
    const float ease = 1.0f - std::pow(1.0f - t, 3.0f);

    const NVGcolor base = nvgRGB(45, 55, 72);

    // Edges first (behind the nodes).
    nvgStrokeColor(vg, nvgRGB(110, 120, 140));
    nvgStrokeWidth(vg, std::max(1.3f, 1.8f * fs));
    for (int i = 0; i < n; i++) {
        for (int c : nodes_[i].children) {
            if (c < 0 || c >= n) continue;
            nvgBeginPath(vg);
            nvgMoveTo(vg, px(i), py(i));
            nvgLineTo(vg, px(c), py(c));
            nvgStroke(vg);
        }
    }

    // Nodes (circles). Highlighted nodes fade colour in and pop their radius.
    for (int i = 0; i < n; i++) {
        NVGcolor target = base;
        auto it = highlights_.find(i);
        if (it != highlights_.end()) {
            const std::string& s = it->second;
            if (s == "swap")         target = nvgRGB(220, 70, 70);
            else if (s == "compare") target = nvgRGB(230, 180, 50);
            else if (s == "sorted")  target = nvgRGB(70, 170, 90);
            else if (s == "active")  target = nvgRGB(60, 130, 220);
        }
        const bool highlighted = (it != highlights_.end());
        const NVGcolor fill = highlighted ? lerpColor(base, target, ease) : target;
        const float rr = highlighted ? r * (0.62f + 0.38f * ease) : r;

        nvgBeginPath(vg);
        nvgCircle(vg, px(i), py(i), rr);
        nvgFillColor(vg, fill);
        nvgFill(vg);
        nvgStrokeColor(vg, nvgRGB(20, 24, 32));
        nvgStrokeWidth(vg, 1.5f);
        nvgStroke(vg);

        nvgFontSize(vg, std::max(9.0f, 18.0f * fs));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGB(235, 235, 235));
        nvgText(vg, px(i), py(i), nodes_[i].value.c_str(), nullptr);
    }

    // Pointer labels above the nodes (stack upward when several share one).
    nvgFontSize(vg, std::max(9.0f, 12.0f * fs));
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
    std::unordered_map<int, int> slots;
    for (const auto& p : pointers_) {
        const int idx = p.second;
        if (idx < 0 || idx >= n) continue;
        const int s = slots[idx]++;
        nvgFillColor(vg, nvgRGB(90, 200, 250));
        nvgText(vg, px(idx), py(idx) - r - 4.0f - s * 13.0f, p.first.c_str(), nullptr);
    }
}
