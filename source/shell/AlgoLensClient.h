#pragma once
#include "include/cef_client.h"
#include "include/cef_life_span_handler.h"

class AlgoLensClient : public CefClient, public CefLifeSpanHandler {
    public:
        AlgoLensClient() = default;

        // Cefclient
        CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }

        // CefLifeSpandHandler
        void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
        void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    private:
        int browserCount_ = 0;
        IMPLEMENT_REFCOUNTING(AlgoLensClient);
};
