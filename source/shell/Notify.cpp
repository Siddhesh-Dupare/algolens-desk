#include "Notify.h"

#include <mutex>
#include <utility>
#include <vector>

#include "include/cef_task.h"
#include "include/base/cef_callback.h"
#include "include/wrapper/cef_closure_task.h"

namespace {

std::mutex g_mutex;
CefRefPtr<CefBrowser> g_browser;
std::vector<std::pair<std::string, std::string>> g_queue;

std::string JsEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '\'': out += "\\'"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            default: out += c;
        }
    }
    return out;
}

// Runs on the CEF UI thread: pushes the error into the page's toast hook.
void DeliverOnUI(std::string title, std::string message) {
    CefRefPtr<CefBrowser> browser;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        browser = g_browser;
    }
    if (!browser) return;
    CefRefPtr<CefFrame> frame = browser->GetMainFrame();
    if (!frame) return;
    std::string js = "window.__algolensError && window.__algolensError('" +
                     JsEscape(title) + "','" + JsEscape(message) + "')";
    frame->ExecuteJavaScript(js, frame->GetURL(), 0);
}

}  // namespace

namespace algolens {

void SetNotifyBrowser(CefRefPtr<CefBrowser> browser) {
    std::vector<std::pair<std::string, std::string>> pending;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_browser = browser;
        if (browser) pending.swap(g_queue);  // flush errors queued before startup
    }
    for (auto& e : pending) ReportError(e.first, e.second);
}

void ReportError(const std::string& title, const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_browser) {
            g_queue.emplace_back(title, message);  // browser not ready yet
            return;
        }
    }
    if (CefCurrentlyOn(TID_UI)) {
        DeliverOnUI(title, message);
    } else {
        CefPostTask(TID_UI, base::BindOnce(&DeliverOnUI, title, message));
    }
}

}  // namespace algolens
