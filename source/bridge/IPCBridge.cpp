#include "IPCBridge.h"
#include "../ipc/IRChannel.h"

IPCBridge::IPCBridge() {
    shm_.CreateWriter(IR_SHM_NAME, IR_SHM_SIZE);
}

bool IPCBridge::OnQuery(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    int64_t query_id,
    const CefString& request,
    bool persistent,
    CefRefPtr<Callback> callback) {
        shm_.Write(request.ToString());
        callback->Success("ok");
        return true;
}
