#pragma once

#include <vector>
#include <string>
#include "../renderers/BaseRenderer.h"
#include <memory>
#include <SDL3/SDL.h>

class IRParser {
    public:
        std::vector<std::unique_ptr<BaseRenderer>> buildRenderersFromIR(const std::string& jsonText);
};
