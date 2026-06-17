#pragma once

#include <cstdint>
#include "include/wrapper/cef_message_router.h"
#include "../ipc/SharedMemory.h"

class IPCBridge : public CefMessageRouterBrowserSide::Handler {
    public:
        IPCBridge();

        bool OnQuery(CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            int64_t query_id,
            const CefString& request,
            bool persistent,
            CefRefPtr<Callback> callback) override;

    private:
        SharedMemory shm_;
};
