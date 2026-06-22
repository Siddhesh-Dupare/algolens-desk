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

// Frameless custom-chrome: hide the OS title bar/border via WM_NCCALCSIZE while
// keeping the window a normal overlapped window (so the OS still animates
// min/max and provides Aero Snap). WM_NCHITTEST restores edge-resizing.
static WNDPROC g_origProc = nullptr;

static LRESULT CALLBACK FramelessProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_NCCALCSIZE:
        if (wp == TRUE) {
            if (IsZoomed(hwnd)) {
                // Maximized: inset by the frame size so it fills the work area
                // exactly (no offscreen spill) and shows no border.
                NCCALCSIZE_PARAMS* p = reinterpret_cast<NCCALCSIZE_PARAMS*>(lp);
                int fx = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                int fy = GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                p->rgrc[0].left   += fx;
                p->rgrc[0].top    += fy;
                p->rgrc[0].right  -= fx;
                p->rgrc[0].bottom -= fy;
            }
            return 0;  // no visible non-client frame
        }
        break;
    case WM_NCHITTEST: {
        const int bw = 6;  // edge grab width (px)
        POINT pt{ (short)LOWORD(lp), (short)HIWORD(lp) };
        RECT rc; GetWindowRect(hwnd, &rc);
        bool L = pt.x < rc.left + bw,  R = pt.x >= rc.right - bw;
        bool T = pt.y < rc.top + bw,   B = pt.y >= rc.bottom - bw;
        if (T && L) return HTTOPLEFT;
        if (T && R) return HTTOPRIGHT;
        if (B && L) return HTBOTTOMLEFT;
        if (B && R) return HTBOTTOMRIGHT;
        if (L) return HTLEFT;
        if (R) return HTRIGHT;
        if (T) return HTTOP;
        if (B) return HTBOTTOM;
        return HTCLIENT;
    }
    }
    return CallWindowProc(g_origProc, hwnd, msg, wp, lp);
}

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

    // Start hidden (zero size). The Vue visualizer panel reports its real rect
    // via setRendererBounds; until then we don't want a floating panel.
    SetWindowPos(child, HWND_TOP, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOACTIVATE);
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

    // Hide the OS title bar/border while keeping the window "normal" (animations
    // + snap). The subclass installs once; SWP_FRAMECHANGED applies it.
    HWND top = GetAncestor(cefHwnd, GA_ROOT);
    if (!g_origProc) {
        g_origProc = (WNDPROC)SetWindowLongPtr(top, GWLP_WNDPROC, (LONG_PTR)FramelessProc);
    }
    SetWindowPos(top, nullptr, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

    std::thread(dockRenderer, cefHwnd).detach();
}

void AlgoLensClient::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    if (--browserCount_ == 0) {
        CefQuitMessageLoop();
    }
}
