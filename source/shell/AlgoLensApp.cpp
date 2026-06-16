#include "AlgoLensApp.h"
#include "AlgoLensClient.h"

#include <windows.h>
#include <string>

#include "include/cef_browser.h"
#include "include/wrapper/cef_helpers.h"

// Folder where AlgoLens.exe lives, so the app path doesn't depend on the
// working directory you launch from.
static std::string exeDir() {
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string p(path);
    return p.substr(0, p.find_last_of("\\/"));
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

    CefRefPtr<AlgoLensClient> client(new AlgoLensClient());

    CefWindowInfo window_info;
    window_info.SetAsPopup(nullptr, "AlgoLens");

    CefBrowserSettings browser_settings;

    CefBrowserHost::CreateBrowser(
        window_info, client, "http://localhost:17653",
        browser_settings, nullptr, nullptr);
}
