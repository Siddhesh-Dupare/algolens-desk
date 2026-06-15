
#include "source/app.h"

int main(int argc, char* argv[]) {
    app App;

    if (!App.init())
        return 1;

    App.run();

    return 0;
}
