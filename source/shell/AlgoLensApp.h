#pragma once
#include "include/cef_app.h"
#include "../http/HTTPServer.h"

class AlgoLensApp : public CefApp, public CefBrowserProcessHandler {
    public:
        AlgoLensApp() = default;

        // CefApp
        CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }

        // CefApp — runs before Chromium starts; tweak command-line switches here.
        void OnBeforeCommandLineProcessing(
            const CefString& process_type,
            CefRefPtr<CefCommandLine> command_line) override;

        // CefbrowserProcessHandler
        void OnContextInitialized() override;

    private:
        HTTPServer httpServer_;
        IMPLEMENT_REFCOUNTING(AlgoLensApp);
};
