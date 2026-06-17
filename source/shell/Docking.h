#pragma once

// Reposition the docked renderer window. Coordinates are relative to the CEF
// window's client area, in physical pixels. Called from the IPC bridge when
// the Vue page sends a { type: "bounds", x, y, w, h } query.
void SetDockedRendererBounds(int x, int y, int w, int h);
