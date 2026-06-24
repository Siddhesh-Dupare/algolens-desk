#pragma once

#include <SDL3/SDL.h>
#include <nanovg.h>
#include <memory>
#include <vector>

#include "renderers/BaseRenderer.h"
#include "ir/IRParser.h"
#include "ipc/SharedMemory.h"

class app {
    public:
        app();
        ~app();
        bool init();
        void run();
        void shutdown();

    private:
        // Lay out a group of renderers (band layout + variables overlay) inside
        // the rectangle (x, y, w, h).
        void renderGroup(std::vector<std::unique_ptr<BaseRenderer>>& group,
                         float x, float y, float w, float h);

        SDL_Window* window;
        SDL_GLContext glContext;

        NVGcontext* nvgContext;

        Scene scene_;

        SharedMemory shm_;
        bool shmOpen_ = false;
        uint32_t lastSeq_ = 0;

        float railScrollX_ = 0.0f;  // horizontal scroll offset of the parked rail

        bool running;
        int WIDTH;
        int HEIGHT;
};
