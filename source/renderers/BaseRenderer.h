#pragma once

#include <nanovg.h>

class BaseRenderer {
    public:
        virtual ~BaseRenderer() = default;
        virtual void render(NVGcontext* nvgContext, int width, int height) = 0;

        // Overlays (e.g. the variables panel) draw over the full panel in a
        // corner; spatial structures (array/stack/queue/grid) are laid out in
        // their own bands. Default: spatial.
        virtual bool isOverlay() const { return false; }
};
