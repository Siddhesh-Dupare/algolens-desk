#include "AlgoLensApp.h"
#include "AlgoLensClient.h"

#include <windows.h>
#include <string>
#include <filesystem>
#include <winuser.h>

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

static std::string findNodeExe() {
    std::string pf = getEnv("ProgramFiles");
    if (!pf.empty()) {
        fs::path p = fs::path(pf) / "nodejs" / "node.exe";
        if (fs::exists(p)) return p.string();
    }
    std::string pf86 = getEnv("ProgramFiles(x86)");
    if (!pf86.empty()) {
        fs::path p = fs::path(pf86) / "nodejs" / "node.exe";
        if (fs::exists(p)) return p.string();
    }
    return "node.exe";  // fallback to PATH
}

static std::string findTerminalDir() {
    fs::path installed = fs::path(exeDir()) / "algolens-terminal";
    if (fs::exists(installed)) return installed.string();
    fs::path dev = fs::path(exeDir()) / ".." / ".." / ".." / "algolens-terminal";
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

    // Spawn the OpenGL renderer (lives next to the shell in build\Debug).
    processManager_.Spawn(exeDir() + "\\AlgoLensRenderer.exe");

    std::string bun = findBunExe();
    std::string serverDir = findServerDir();
    if (!serverDir.empty()) {
        // Run the entry file directly — NOT `bun run dev` (its --watch script
        // re-resolves bun via PATH, which a GUI process doesn't have).
        processManager_.Spawn(bun, "run src/index.ts", serverDir);
    }

    std::string node = findNodeExe();
    std::string termDir = findTerminalDir();
    if (!termDir.empty()) {
        processManager_.Spawn(node, "index.js", termDir);
    }

    CefRefPtr<AlgoLensClient> client(new AlgoLensClient());

    CefWindowInfo window_info;
    window_info.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
    window_info.SetAsPopup(nullptr, "AlgoLens");
    window_info.style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;

    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    int ww = sw * 4 / 5, wh = sh * 4 / 5;
    window_info.bounds.x = (sw - ww) / 2;
    window_info.bounds.y = (sh - wh) / 2;
    window_info.bounds.width = ww;
    window_info.bounds.height = wh;

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
