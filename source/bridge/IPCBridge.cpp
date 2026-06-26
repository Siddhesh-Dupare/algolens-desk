#include "IPCBridge.h"
#include "../ipc/IRChannel.h"
#include "../shell/Docking.h"
#include <json.hpp>
#include <windows.h>
#include <shellapi.h>
#include <urlmon.h>
#include <thread>
#include <string>
#include "include/wrapper/cef_closure_task.h"
#include "include/base/cef_callback.h"
#include "include/cef_task.h"

using json = nlohmann::json;

IPCBridge::IPCBridge() {
    shm_.CreateWriter(IR_SHM_NAME, IR_SHM_SIZE);
}

bool IPCBridge::OnQuery(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    int64_t query_id,
    const CefString& request,
    bool persistent,
    CefRefPtr<Callback> callback) {
    const std::string req = request.ToString();
    try {
        json j = json::parse(req);
        if (j.value("type", std::string()) == "bounds") {
            SetDockedRendererBounds(j.value("x", 0), j.value("y", 0),
                                    j.value("w", 0), j.value("h", 0));
            callback->Success("ok");
            return true;
        }
        if (j.value("type", std::string()) == "closeWindow") {
            browser->GetHost()->CloseBrowser(false);
            callback->Success("ok");
            return true;
        }
        if (j.value("type", std::string()) == "minimizeWindow") {
            HWND h = GetAncestor(browser->GetHost()->GetWindowHandle(), GA_ROOT);
            ShowWindow(h, SW_MINIMIZE);
            callback->Success("ok");
            return true;
        }
        if (j.value("type", std::string()) == "maximizeWindow") {
            HWND h = GetAncestor(browser->GetHost()->GetWindowHandle(), GA_ROOT);
            ShowWindow(h, IsZoomed(h) ? SW_RESTORE : SW_MAXIMIZE);
            callback->Success("ok");
            return true;
        }
        if (j.value("type", std::string()) == "openExternal") {
            // Open a URL in the system browser (only http/https for safety).
            const std::string url = j.value("url", std::string());
            if (url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0) {
                ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            }
            callback->Success("ok");
            return true;
        }
        if (j.value("type", std::string()) == "installUpdate") {
            // Download the new installer to %TEMP%, run it, then close the app so
            // its files can be replaced. Download runs off the UI thread.
            const std::string url = j.value("url", std::string());
            if (url.rfind("https://", 0) == 0) {
                CefRefPtr<CefBrowser> b = browser;
                std::thread([url, b]() {
                    char tmp[MAX_PATH] = {};
                    GetTempPathA(MAX_PATH, tmp);
                    std::string out = std::string(tmp) + "AlgoLens-Update.exe";
                    if (SUCCEEDED(URLDownloadToFileA(nullptr, url.c_str(),
                                                     out.c_str(), 0, nullptr))) {
                        ShellExecuteA(nullptr, "open", out.c_str(), nullptr, nullptr,
                                      SW_SHOWNORMAL);
                        // Close the app on the UI thread so the installer can update it.
                        CefPostTask(TID_UI, base::BindOnce([](CefRefPtr<CefBrowser> br) {
                            br->GetHost()->CloseBrowser(false);
                        }, b));
                    }
                }).detach();
            }
            callback->Success("ok");
            return true;
        }
    } catch (...) {
        // not JSON / no "type" — treat it as Visual IR below
    }
    shm_.Write(req);
    callback->Success("ok");
    return true;
}
