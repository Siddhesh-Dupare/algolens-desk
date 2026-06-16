#include <windows.h>
#include "include/cef_app.h"
#include "AlgoLensApp.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int) {
    CefMainArgs main_args(hInstance);

    CefRefPtr<AlgoLensApp> app(new AlgoLensApp);

    // Sub-processes (renderer/gpu/utility) re-enter here and exit
    int exit_code = CefExecuteProcess(main_args, app.get(), nullptr);
    if (exit_code >= 0) return exit_code;

    CefSettings settings;
    settings.no_sandbox = true;

    CefInitialize(main_args, settings, app.get(), nullptr);
    CefRunMessageLoop();
    CefShutdown();
    return 0;
}
