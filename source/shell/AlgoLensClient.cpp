#include <windows.h>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdio>

#include "AlgoLensClient.h"
#include "include/cef_app.h"
#include "include/wrapper/cef_helpers.h"
#include "Docking.h"
#include <atomic>

static std::atomic<HWND> g_dockedRenderer{nullptr};

struct FindData { HWND result = nullptr; const char* title = nullptr; };

static BOOL CALLBACK enumProc(HWND hwnd, LPARAM lparam) {
    auto* d = reinterpret_cast<FindData*>(lparam);
    char buf[256] = {};
    GetWindowTextA(hwnd, buf, sizeof(buf));
    if (IsWindowVisible(hwnd) && std::strcmp(buf, d->title) == 0) {
        d->result = hwnd;
        return FALSE;   // found it — stop enumerating
    }
    return TRUE;
}

static HWND findWindowByTitle(const char* title) {
    FindData d; d.title = title;
    EnumWindows(enumProc, reinterpret_cast<LPARAM>(&d));
    return d.result;
}

static void dockRenderer(HWND cefHwnd) {
    // Poll up to ~10s for the renderer window to appear.
    HWND child = nullptr;
    for (int i = 0; i < 100 && !child; ++i) {
        child = findWindowByTitle("AlgoLensRenderer");
        if (!child) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!child) return;

    HWND parent = GetAncestor(cefHwnd, GA_ROOT);   // top-level shell window

    // Convert the renderer from a top-level window to a child window.
    LONG_PTR style = GetWindowLongPtr(child, GWL_STYLE);
    style = (style & ~(WS_POPUP | WS_OVERLAPPEDWINDOW)) | WS_CHILD;
    SetWindowLongPtr(child, GWL_STYLE, style);

    SetParent(child, parent);
    g_dockedRenderer = child;

    // Initial placement (right half) until the Vue panel sends real bounds.
    RECT rc; GetClientRect(parent, &rc);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    SetWindowPos(child, HWND_TOP, w / 2, 0, w / 2, h,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

void SetDockedRendererBounds(int x, int y, int w, int h) {
    HWND child = g_dockedRenderer.load();
    if (!child) return;
    SetWindowPos(child, HWND_TOP, x, y, w, h, SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

AlgoLensClient::AlgoLensClient() {
    CefMessageRouterConfig config;
    router_ = CefMessageRouterBrowserSide::Create(config);
    bridge_ = std::make_unique<IPCBridge>();
    router_->AddHandler(bridge_.get(), false);
}

bool AlgoLensClient::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
    CEF_REQUIRE_UI_THREAD();
    return router_->OnProcessMessageReceived(browser, frame, source_process, message);
}

void AlgoLensClient::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    ++browserCount_;

    HWND cefHwnd = browser->GetHost()->GetWindowHandle();
    std::thread(dockRenderer, cefHwnd).detach();
}

void AlgoLensClient::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    if (--browserCount_ == 0) {
        CefQuitMessageLoop();
    }
}
