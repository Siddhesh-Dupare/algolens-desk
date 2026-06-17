#include "AlgoLensApp.h"
#include "AlgoLensClient.h"

#include <windows.h>
#include <string>
#include <filesystem>

#include "include/cef_browser.h"
#include "include/wrapper/cef_helpers.h"

namespace fs = std::filesystem;

// Folder where AlgoLens.exe lives, so the app path doesn't depend on the
// working directory you launch from.
static std::string exeDir() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string p(path);
    return p.substr(0, p.find_last_of("\\/"));
}

static std::string getEnv(const char* name) {
    char buf[MAX_PATH];
    DWORD n = GetEnvironmentVariableA(name, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return "";
    return std::string(buf, n);
}

// The PATH "bun" is often a shim that CreateProcess can't run, so locate the
// real bun.exe directly.
static std::string findBunExe() {
    // 1. Standard installer: %USERPROFILE%\.bun\bin\bun.exe
    std::string up = getEnv("USERPROFILE");
    if (!up.empty()) {
        fs::path p = fs::path(up) / ".bun" / "bin" / "bun.exe";
        if (fs::exists(p)) return p.string();
    }
    // 2. npm-global install: %APPDATA%\npm\node_modules\bun\bin\bun.exe  (your case)
    std::string ad = getEnv("APPDATA");
    if (!ad.empty()) {
        fs::path p = fs::path(ad) / "npm" / "node_modules" / "bun" / "bin" / "bun.exe";
        if (fs::exists(p)) return p.string();
    }
    return "bun.exe";  // last resort: hope it's on PATH
}


static std::string findServerDir() {
    // Installed layout: <exe>\algolens-server
    fs::path installed = fs::path(exeDir()) / "algolens-server";
    if (fs::exists(installed)) return installed.string();
    // Dev layout: build\Debug -> build -> desktop -> AlgoLens\algolens-server
    fs::path dev = fs::path(exeDir()) / ".." / ".." / ".." / "algolens-server";
    if (fs::exists(dev)) return fs::canonical(dev).string();
    return "";
}

void AlgoLensApp::OnBeforeCommandLineProcessing(
    const CefString& process_type,
    CefRefPtr<CefCommandLine> command_line) {
    // Chrome upgrades http:// to https://; our embedded server is plain HTTP,
    // so the handshake fails. Turn the upgrade off for the embedded app.
    command_line->AppendSwitchWithValue("disable-features", "HttpsUpgrades");
}

void AlgoLensApp::OnContextInitialized() {
    CEF_REQUIRE_UI_THREAD();

    // Serve the Vue build (dist copied to <exe>/app) on localhost.
    httpServer_.Start(17653, exeDir() + "\\app");

    std::string bun = findBunExe();
    std::string serverDir = findServerDir();
    if (!serverDir.empty()) {
        // Run the entry file directly — NOT `bun run dev` (its --watch script
        // re-resolves bun via PATH, which a GUI process doesn't have).
        processManager_.Spawn(bun, "run src/index.ts", serverDir);
    }

    CefRefPtr<AlgoLensClient> client(new AlgoLensClient());

    CefWindowInfo window_info;
    window_info.SetAsPopup(nullptr, "AlgoLens");

    CefBrowserSettings browser_settings;

    CefBrowserHost::CreateBrowser(
        window_info, client, "http://localhost:17653",
        browser_settings, nullptr, nullptr);
}

// ---- Render process: this is what injects window.cefQuery into JS
void AlgoLensApp::OnWebKitInitialized() {
    CefMessageRouterConfig config;
    renderRouter_ = CefMessageRouterRendererSide::Create(config);
}

void AlgoLensApp::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame,
                                   CefRefPtr<CefV8Context> context) {
    renderRouter_->OnContextCreated(browser, frame, context);
}

void AlgoLensApp::OnContextReleased(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    CefRefPtr<CefV8Context> context) {
    renderRouter_->OnContextReleased(browser, frame, context);
}

bool AlgoLensApp::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                           CefRefPtr<CefFrame> frame,
                                           CefProcessId source_process,
                                           CefRefPtr<CefProcessMessage> message) {
    return renderRouter_->OnProcessMessageReceived(browser, frame, source_process, message);
}

void AlgoLensApp::StopChildren() {
    processManager_.KillAll();
}
