#pragma once

// HTML/JS for the setup page ("/"), kept separate so web.cpp does not grow
// too large. Served as-is by web.cpp's handleRoot().

extern const char kSetupPageHtml[];
