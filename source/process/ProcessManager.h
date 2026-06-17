#pragma once
#include <string>
#include <vector>
#include <windows.h>

class ProcessManager {
    public:
        ~ProcessManager();

        // Launch an executable
        bool Spawn(const std::string& exePath,
                const std::string& args = "",
           const std::string& workingDir = "");

        void KillAll();

    private:
        std::vector<PROCESS_INFORMATION> processes_;
};
