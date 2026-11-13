#pragma once

#include "gallery/core/Geometry.hpp"

#include <SFML/Window/Event.hpp>

#include <optional>
#include <variant>

namespace shell::platform {

// The keys the game will ever react to. Everything else is `Other`.
enum class Key {
    Escape, Space, Enter, R, F3, F4, P, K,
    Digit1, Digit2, Digit3, Digit4, Digit5, Digit6, Digit7, Digit8,   // layer toggles (M1 inspection)
    M, N, VolumeDown, VolumeUp,                                        // sound settings (D-030)
    Other
};

// SFML-independent input, still in WINDOW PIXELS. The application converts
// positions to logical space through gallery::Viewport (the only converter).
namespace raw {
struct CloseRequested {};
struct Resized { gallery::Size size; };
struct FocusLost {};
struct FocusGained {};
struct PointerMoved { gallery::Vec2 px; };
struct PrimaryPressed { gallery::Vec2 px; };
struct SecondaryPressed { gallery::Vec2 px; };
struct KeyPressed { Key key; };
} // namespace raw

using RawInput = std::variant<raw::CloseRequested, raw::Resized, raw::FocusLost,
                              raw::FocusGained, raw::PointerMoved, raw::PrimaryPressed, raw::SecondaryPressed,
                              raw::KeyPressed>;

// Translate one SFML event. Events the game ignores yield nullopt.
[[nodiscard]] std::optional<RawInput> translateEvent(const sf::Event& event) noexcept;

} // namespace shell::platform
