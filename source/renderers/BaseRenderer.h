#pragma once

#include <nanovg.h>

class BaseRenderer {
    public:
        virtual ~BaseRenderer() = default;
        virtual void render(NVGcontext* nvgContext, int width, int height) = 0;
};
