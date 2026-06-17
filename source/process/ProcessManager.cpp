#include "ProcessManager.h"
#include <processthreadsapi.h>

ProcessManager::~ProcessManager() {
    KillAll();
}

bool ProcessManager::Spawn(const std::string& exePath, const std::string& args, const std::string& workingDir) {
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::string cmd = "\"" + exePath + "\"";
    if (!args.empty()) cmd += " " + args;

    BOOL ok = CreateProcessA(
        nullptr,                  // app name (take it from the command line)
        cmd.data(),               // mutable command line
        nullptr, nullptr,         // security attrs
        FALSE,                    // don't inherit handles
        CREATE_NO_WINDOW,         // no console window for the child
        nullptr,                  // inherit environment
        workingDir.empty() ? nullptr : workingDir.c_str(),
        &si, &pi);

    if (!ok) return false;

    processes_.push_back(pi);     // remember it so we can kill it later
    return true;
}

void ProcessManager::KillAll() {
    for (auto& pi : processes_) {
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    processes_.clear();
}
