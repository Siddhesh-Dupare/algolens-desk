#pragma once

#include <vector>
#include <string>
#include "../renderers/BaseRenderer.h"
#include <memory>
#include <SDL3/SDL.h>

// One suspended scope's structure, drawn as a small thumbnail.
struct ParkedScene {
    std::string label;
    std::vector<std::unique_ptr<BaseRenderer>> renderers;
};

// A full scene: the active structures (main stage) + parked thumbnails.
struct Scene {
    std::vector<std::unique_ptr<BaseRenderer>> main;
    std::vector<ParkedScene> parked;
};

class IRParser {
    public:
        std::vector<std::unique_ptr<BaseRenderer>> buildRenderersFromIR(const std::string& jsonText);
        Scene buildSceneFromIR(const std::string& jsonText);
};
