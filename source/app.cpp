
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

// Subtle dotted-grid backdrop drawn behind every visualization.
static void drawDotGrid(NVGcontext* vg, int width, int height) {
    const float spacing = 26.0f;  // px between dots
    const float radius = 1.3f;
    nvgFillColor(vg, nvgRGBA(255, 255, 255, 14));  // faint white
    for (float y = spacing * 0.5f; y < height; y += spacing) {
        for (float x = spacing * 0.5f; x < width; x += spacing) {
            nvgBeginPath(vg);
            nvgCircle(vg, x, y, radius);
            nvgFill(vg);
        }
    }
}

void app::run() {
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            // Scroll the parked rail horizontally (vertical wheel maps to it too).
            if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                const float d = (event.wheel.x != 0.0f ? event.wheel.x : event.wheel.y);
                railScrollX_ -= d * 48.0f;
            }
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
                scene_ = parser.buildSceneFromIR(json);
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

        // Dotted-grid backdrop behind everything.
        drawDotGrid(nvgContext, pw, ph);

        const int nParked = (int)scene_.parked.size();
        if (nParked == 0) {
            // No suspended scopes: the active structures use the whole panel.
            renderGroup(scene_.main, 0.0f, 0.0f, (float)pw, (float)ph);
        } else {
            // Active structures on the main stage; parked scopes as a bottom rail.
            float stripH = (float)ph * 0.24f;
            if (stripH < 96.0f) stripH = 96.0f;
            if (stripH > (float)ph * 0.4f) stripH = (float)ph * 0.4f;
            const float mainH = (float)ph - stripH;

            renderGroup(scene_.main, 0.0f, 0.0f, (float)pw, mainH);

            // Divider line above the rail.
            nvgBeginPath(nvgContext);
            nvgRect(nvgContext, 0.0f, mainH, (float)pw, 1.0f);
            nvgFillColor(nvgContext, nvgRGB(40, 44, 54));
            nvgFill(nvgContext);

            // Cards keep a minimum width; when they overflow, the rail scrolls.
            const float pad = 8.0f;
            const float minCardW = 168.0f;
            float cardW = ((float)pw - pad * (nParked + 1)) / (float)nParked;
            if (cardW < minCardW) cardW = minCardW;
            const float contentW = pad + nParked * (cardW + pad);
            const float maxScroll = contentW - (float)pw > 0.0f ? contentW - (float)pw : 0.0f;
            if (railScrollX_ < 0.0f) railScrollX_ = 0.0f;
            if (railScrollX_ > maxScroll) railScrollX_ = maxScroll;

            const float cardLabelH = 18.0f;
            const float cy = mainH + pad;
            const float ch = stripH - 2.0f * pad - (maxScroll > 0.0f ? 6.0f : 0.0f);

            // Clip the rail so off-screen cards are hidden during scrolling.
            nvgSave(nvgContext);
            nvgIntersectScissor(nvgContext, 0.0f, mainH, (float)pw, stripH);
            for (int i = 0; i < nParked; i++) {
                ParkedScene& ps = scene_.parked[i];
                const float cx = pad + i * (cardW + pad) - railScrollX_;
                if (cx + cardW < 0.0f || cx > (float)pw) continue;  // fully off-screen

                // Card background + border.
                nvgBeginPath(nvgContext);
                nvgRoundedRect(nvgContext, cx, cy, cardW, ch, 6.0f);
                nvgFillColor(nvgContext, nvgRGB(18, 21, 28));
                nvgFill(nvgContext);
                nvgStrokeColor(nvgContext, nvgRGB(48, 53, 66));
                nvgStrokeWidth(nvgContext, 1.0f);
                nvgStroke(nvgContext);

                // Label (the variable/scope name).
                nvgFontFace(nvgContext, "sans");
                nvgFontSize(nvgContext, 12.0f);
                nvgTextAlign(nvgContext, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
                nvgFillColor(nvgContext, nvgRGB(150, 156, 170));
                nvgText(nvgContext, cx + 8.0f, cy + cardLabelH / 2.0f + 2.0f,
                        ps.label.c_str(), nullptr);

                // The structure, scaled into the card body below the label.
                renderGroup(ps.renderers, cx, cy + cardLabelH, cardW, ch - cardLabelH);
            }
            nvgRestore(nvgContext);

            // Scrollbar showing the rail position when it overflows.
            if (maxScroll > 0.0f) {
                const float trackY = (float)ph - 4.0f;
                nvgBeginPath(nvgContext);
                nvgRoundedRect(nvgContext, pad, trackY, (float)pw - 2.0f * pad, 3.0f, 1.5f);
                nvgFillColor(nvgContext, nvgRGB(30, 34, 42));
                nvgFill(nvgContext);

                const float trackW = (float)pw - 2.0f * pad;
                const float thumbW = trackW * ((float)pw / contentW);
                const float thumbX = pad + (trackW - thumbW) * (railScrollX_ / maxScroll);
                nvgBeginPath(nvgContext);
                nvgRoundedRect(nvgContext, thumbX, trackY, thumbW, 3.0f, 1.5f);
                nvgFillColor(nvgContext, nvgRGB(90, 100, 120));
                nvgFill(nvgContext);
            }
        }

        nvgEndFrame(nvgContext);

        SDL_GL_SwapWindow(window);
    }
}

void app::renderGroup(std::vector<std::unique_ptr<BaseRenderer>>& group,
                      float x, float y, float w, float h) {
    if (w <= 0.0f || h <= 0.0f) return;

    nvgSave(nvgContext);
    nvgIntersectScissor(nvgContext, x, y, w, h);
    nvgTranslate(nvgContext, x, y);

    // Spatial structures (array/stack/queue/grid/tree/graph/list) each get a
    // horizontal band; overlays (variables) draw across the whole group.
    int spatial = 0;
    for (auto& r : group) if (!r->isOverlay()) spatial++;
    if (spatial < 1) spatial = 1;
    const float bandH = h / (float)spatial;

    int bandIdx = 0;
    for (auto& r : group) {
        if (r->isOverlay()) {
            r->render(nvgContext, (int)w, (int)h);
            continue;
        }
        const float by = bandIdx * bandH;
        nvgSave(nvgContext);
        nvgIntersectScissor(nvgContext, 0.0f, by, w, bandH);
        nvgTranslate(nvgContext, 0.0f, by);
        r->render(nvgContext, (int)w, (int)bandH);
        nvgRestore(nvgContext);
        bandIdx++;
    }

    nvgRestore(nvgContext);
}
