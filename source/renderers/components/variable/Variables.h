#pragma once

#include "../../BaseRenderer.h"
#include <nanovg.h>

#include <vector>
#include <string>

struct Variable {
    std::string name;
    std::string value;
};

class Variables : public BaseRenderer {
    public:
        Variables(std::vector<Variable> vars) : vars_(std::move(vars)) {}

        void render(NVGcontext* nvgContext, int width, int height) override;

    private:
        std::vector<Variable> vars_;
};
