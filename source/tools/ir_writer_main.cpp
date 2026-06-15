#include "../ipc/SharedMemory.h"
#include "../ipc/IRChannel.h"

#include <windows.h>
#include <string>

int main() {
    SharedMemory shm;
    if (!shm.CreateWriter(IR_SHM_NAME, IR_SHM_SIZE)) {
        return 1;
    }

    const std::string irA = R"({
        "version": 1,
        "components": [
            { "type": "array", "values": [5, 2, 8, 1, 7] },
            { "type": "variables", "items": [
                { "name": "i",       "value": "3" },
                { "name": "sum",     "value": "12" },
                { "name": "swapped", "value": "true" }
            ]}
        ]
    })";

    const std::string irB = R"({
        "version": 1,
        "components": [
            { "type": "array", "values": [1, 2, 5, 7, 8] },
            { "type": "variables", "items": [
                { "name": "i",       "value": "5" },
                { "name": "sum",     "value": "23" },
                { "name": "swapped", "value": "false" }
            ]}
        ]
    })";

    bool toggle = false;
    while (true) {
        shm.Write(toggle ? irB : irA);
        toggle = !toggle;
        Sleep(3000);
    }

    return 0;
}
