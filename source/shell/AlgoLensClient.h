#pragma once
#include <memory>
#include "include/cef_client.h"
#include "include/cef_life_span_handler.h"
#include "include/wrapper/cef_message_router.h"
#include "../bridge/IPCBridge.h"

class AlgoLensClient : public CefClient, public CefLifeSpanHandler {
    public:
        AlgoLensClient();

        // Cefclient
        CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }

        bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            CefProcessId source_process,
            CefRefPtr<CefProcessMessage> message) override;

        // CefLifeSpandHandler
        void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
        void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    private:
        CefRefPtr<CefMessageRouterBrowserSide> router_;
        std::unique_ptr<IPCBridge> bridge_;
        int browserCount_ = 0;
        IMPLEMENT_REFCOUNTING(AlgoLensClient);
};
