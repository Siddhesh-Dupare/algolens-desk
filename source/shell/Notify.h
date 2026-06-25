#pragma once

#include <string>
#include "include/cef_browser.h"

namespace algolens {

// Set the browser used to deliver errors to the web UI (null on close).
void SetNotifyBrowser(CefRefPtr<CefBrowser> browser);

// Report a desktop-side error; shown as a bottom-right toast in the Vue app.
// Safe to call before the browser exists (queued and flushed once it is ready).
void ReportError(const std::string& title, const std::string& message);

}  // namespace algolens
