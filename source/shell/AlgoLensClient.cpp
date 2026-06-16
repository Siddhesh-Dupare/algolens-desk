#include "AlgoLensClient.h"
#include "include/cef_app.h"
#include "include/wrapper/cef_helpers.h"

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
