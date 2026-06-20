#include "IPCBridge.h"
#include "../ipc/IRChannel.h"
#include "../shell/Docking.h"
#include <json.hpp>

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
    } catch (...) {
        // not JSON / no "type" — treat it as Visual IR below
    }
    shm_.Write(req);
    callback->Success("ok");
    return true;
}
