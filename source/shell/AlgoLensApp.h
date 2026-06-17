#pragma once
#include "include/cef_app.h"
#include "include/cef_command_line.h"
#include "include/wrapper/cef_message_router.h"
#include "../http/HTTPServer.h"

class AlgoLensApp : public CefApp, public CefBrowserProcessHandler, public CefRenderProcessHandler {
    public:
        AlgoLensApp() = default;

        // CefApp
        CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
        CefRefPtr<CefRenderProcessHandler>  GetRenderProcessHandler()  override { return this; }

        // CefApp — runs before Chromium starts; tweak command-line switches here.
        void OnBeforeCommandLineProcessing(
            const CefString& process_type,
            CefRefPtr<CefCommandLine> command_line) override;

        // CefbrowserProcessHandler
        void OnContextInitialized() override;

        // CefRenderProcessHandler
        void OnWebKitInitialized() override;
        void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                CefRefPtr<CefV8Context> context) override;
        void OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                CefRefPtr<CefV8Context> context) override;
        bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                        CefProcessId source_process,
                                        CefRefPtr<CefProcessMessage> message) override;

    private:
        HTTPServer httpServer_;
        CefRefPtr<CefMessageRouterRendererSide> renderRouter_;
        IMPLEMENT_REFCOUNTING(AlgoLensApp);
};
