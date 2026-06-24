
#include "app.h"

#include <cstdint>
#include <glad/glad.h>
#include <nanovg_gl.h>

#include "renderers/components/array/Array.h"
#include "renderers/components/variable/Variables.h"
#include "ir/IRParser.h"
#include "ipc/IRChannel.h"

app::app()
    : window{nullptr}, glContext{nullptr}, nvgContext{nullptr},
    running{false}, WIDTH{700}, HEIGHT{900} {}
app::~app() {
    shutdown();
}

void app::shutdown() {
    if (nvgContext)
        nvgDeleteGL3(nvgContext);
    if (glContext)
        SDL_GL_DestroyContext(glContext);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
}

bool app::init() {

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init Failed: %s", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // SDL Window
    window = SDL_CreateWindow("AlgoLensRenderer", WIDTH, HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    // OpenGL Window Context
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        SDL_Log("GL context failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    SDL_GL_MakeCurrent(window, glContext);

    // Load GL function pointers before any GL/NanoVG calls
    if (!gladLoadGLLoader(
        reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
            SDL_Log("Failed to initialize glad");
            SDL_DestroyWindow(window);
            SDL_Quit();
            return false;
    }
    SDL_Log("OpenGL %s", glGetString(GL_VERSION));

    nvgContext = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    if (!nvgContext) {
        SDL_Log("Failed to create NanoVG Context");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    // Creating a font
    int font = nvgCreateFont(nvgContext, "sans", "C:\\Windows\\Fonts\\segoeui.ttf");
    if (font == -1) {
        SDL_Log("Failed to load sans font");
    }

    shmOpen_ = shm_.OpenReader(IR_SHM_NAME, IR_SHM_SIZE );

    SDL_GL_SetSwapInterval(1); // vsync

    running = true;

    return true;
}

void app::run() {
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
        }

        if (!shmOpen_) {
            shmOpen_ = shm_.OpenReader(IR_SHM_NAME, IR_SHM_SIZE);
        }

        if (shmOpen_) {
            std::string json;
            uint32_t seq = 0;
            if (shm_.Read(json, seq) && seq != lastSeq_) {
                lastSeq_ = seq;
                IRParser parser;
                renderer_ = parser.buildRenderersFromIR(json);
            }
        }

        int pw = WIDTH;
        int ph = HEIGHT;

        // Get Width and Height in pixels
        SDL_GetWindowSizeInPixels(window, &pw, &ph);

        glViewport(0, 0, pw, ph);

        glClearColor(10.0f/255.0f, 10.0f/255.0f, 12.0f/255.0f, 1.0);
        nvgBeginFrame(nvgContext, pw, ph, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Spatial structures (array/stack/queue/grid) each get a horizontal band
        // so several can be shown at once; overlays (variables) draw full-panel.
        int spatial = 0;
        for (auto& r : renderer_) if (!r->isOverlay()) spatial++;
        if (spatial < 1) spatial = 1;
        const float bandH = (float)ph / (float)spatial;

        int bandIdx = 0;
        for (auto& r : renderer_) {
            if (r->isOverlay()) {
                r->render(nvgContext, pw, ph);
                continue;
            }
            const float y = bandIdx * bandH;
            nvgSave(nvgContext);
            nvgScissor(nvgContext, 0.0f, y, (float)pw, bandH);
            nvgTranslate(nvgContext, 0.0f, y);
            r->render(nvgContext, pw, (int)bandH);
            nvgRestore(nvgContext);
            bandIdx++;
        }

        nvgEndFrame(nvgContext);

        SDL_GL_SwapWindow(window);
    }
}
