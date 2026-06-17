#include "AlgoLensClient.h"
#include "include/cef_app.h"
#include "include/wrapper/cef_helpers.h"

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
}

void AlgoLensClient::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    if (--browserCount_ == 0) {
        CefQuitMessageLoop();
    }
}
