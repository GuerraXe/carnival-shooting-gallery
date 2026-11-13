#include "shell/platform/Input.hpp"

namespace shell::platform {

namespace {

gallery::Vec2 toVec(sf::Vector2i p) noexcept {
    return {static_cast<float>(p.x), static_cast<float>(p.y)};
}

Key toKey(sf::Keyboard::Key code) noexcept {
    switch (code) {
    case sf::Keyboard::Key::Escape: return Key::Escape;
    case sf::Keyboard::Key::Space: return Key::Space;
    case sf::Keyboard::Key::Enter: return Key::Enter;
    case sf::Keyboard::Key::R: return Key::R;
    case sf::Keyboard::Key::F3: return Key::F3;
    case sf::Keyboard::Key::F4: return Key::F4;
    case sf::Keyboard::Key::P: return Key::P;
    case sf::Keyboard::Key::K: return Key::K;
    case sf::Keyboard::Key::Num1: case sf::Keyboard::Key::Numpad1: return Key::Digit1;
    case sf::Keyboard::Key::Num2: case sf::Keyboard::Key::Numpad2: return Key::Digit2;
    case sf::Keyboard::Key::Num3: case sf::Keyboard::Key::Numpad3: return Key::Digit3;
    case sf::Keyboard::Key::Num4: case sf::Keyboard::Key::Numpad4: return Key::Digit4;
    case sf::Keyboard::Key::Num5: case sf::Keyboard::Key::Numpad5: return Key::Digit5;
    case sf::Keyboard::Key::Num6: case sf::Keyboard::Key::Numpad6: return Key::Digit6;
    case sf::Keyboard::Key::Num7: case sf::Keyboard::Key::Numpad7: return Key::Digit7;
    case sf::Keyboard::Key::Num8: case sf::Keyboard::Key::Numpad8: return Key::Digit8;
    case sf::Keyboard::Key::M: return Key::M;
    case sf::Keyboard::Key::N: return Key::N;
    case sf::Keyboard::Key::Hyphen: case sf::Keyboard::Key::Subtract: return Key::VolumeDown;
    case sf::Keyboard::Key::Equal: case sf::Keyboard::Key::Add: return Key::VolumeUp;
    default: return Key::Other;
    }
}

} // namespace

std::optional<RawInput> translateEvent(const sf::Event& event) noexcept {
    if (event.is<sf::Event::Closed>()) {
        return raw::CloseRequested{};
    }
    if (const auto* e = event.getIf<sf::Event::Resized>()) {
        return raw::Resized{{static_cast<float>(e->size.x), static_cast<float>(e->size.y)}};
    }
    if (event.is<sf::Event::FocusLost>()) {
        return raw::FocusLost{};
    }
    if (event.is<sf::Event::FocusGained>()) {
        return raw::FocusGained{};
    }
    if (const auto* e = event.getIf<sf::Event::MouseMoved>()) {
        return raw::PointerMoved{toVec(e->position)};
    }
    if (const auto* e = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (e->button == sf::Mouse::Button::Left) {
            return raw::PrimaryPressed{toVec(e->position)};
        }
        if (e->button == sf::Mouse::Button::Right) {
            return raw::SecondaryPressed{toVec(e->position)};
        }
        return std::nullopt;
    }
    if (const auto* e = event.getIf<sf::Event::KeyPressed>()) {
        return raw::KeyPressed{toKey(e->code)};
    }
    return std::nullopt;
}

} // namespace shell::platform
