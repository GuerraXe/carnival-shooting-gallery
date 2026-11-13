#include "shell/platform/Window.hpp"

#include "shell/platform/StartupError.hpp"

#include <SFML/Window/VideoMode.hpp>

namespace shell::platform {

void openWindow(sf::RenderWindow& window, const WindowConfig& config) {
    window.create(sf::VideoMode({config.width, config.height}),
                  sf::String::fromUtf8(config.title.begin(), config.title.end()),
                  sf::Style::Default, sf::State::Windowed,
                  sf::ContextSettings{.antiAliasingLevel = 4});   // smooth reticle edges; photos unaffected
    if (!window.isOpen()) {
        throw StartupError("Could not create the game window (no usable OpenGL context). "
                           "Updating the graphics driver usually fixes this.");
    }
    window.setVerticalSyncEnabled(config.vsync);
    window.setKeyRepeatEnabled(false);   // a held key is one press, never auto-repeat
    window.setMinimumSize(sf::Vector2u{config.minWidth, config.minHeight});
}

} // namespace shell::platform
