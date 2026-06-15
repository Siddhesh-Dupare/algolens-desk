
#include <vector>

#include "../../BaseRenderer.h"

#include <nanovg.h>

class Array : public BaseRenderer {
    // Work on Step C
    public:
        Array(std::vector<int> data) : data_(std::move(data)) {}

        void render(NVGcontext* nvgContext, int width, int height) override;

    private:
        std::vector<int> data_;
};
