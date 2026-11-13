#pragma once

#include <SFML/Graphics/RenderWindow.hpp>

#include <string>

namespace shell::platform {

struct WindowConfig {
    std::string title;          // ASCII/UTF-8
    unsigned width = 1280;      // initial client size, physical pixels
    unsigned height = 720;
    unsigned minWidth = 640;
    unsigned minHeight = 360;
    bool vsync = true;          // ARCHITECTURE §5.2; latency re-measured at M2 (R-2)
};

// (Re)create `window` with the game's settings. Throws StartupError if no
// window / GL context could be created.
void openWindow(sf::RenderWindow& window, const WindowConfig& config);

} // namespace shell::platform
