#pragma once

#include <SDL3/SDL.h>
#include <nanovg.h>
#include <memory>
#include <vector>

#include "renderers/BaseRenderer.h"
#include "ipc/SharedMemory.h"

class app {
    public:
        app();
        ~app();
        bool init();
        void run();
        void shutdown();

    private:
        SDL_Window* window;
        SDL_GLContext glContext;

        NVGcontext* nvgContext;

        std::vector<std::unique_ptr<BaseRenderer>> renderer_;

        SharedMemory shm_;
        bool shmOpen_ = false;
        uint32_t lastSeq_ = 0;

        bool running;
        int WIDTH;
        int HEIGHT;
};
