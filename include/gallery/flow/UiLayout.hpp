#pragma once

// Screen layout for the Title (main menu), Paused and Results overlays:
// where the painted board hangs and where its buttons are, in logical units
// (ARCHITECTURE §1.2 "UI (split)"). Pure data + hit testing; the shell's
// ScreenRenderer draws from exactly these rects, so what is drawn is what
// is clickable.

#include "gallery/core/Geometry.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace gallery {

enum class AppState : std::uint8_t { Title, Playing, Paused, Results, Exiting };

[[nodiscard]] const char* appStateName(AppState state) noexcept;

enum class ButtonId : std::uint8_t { StartRound, Quit, Resume, RestartRound, MainMenu, PlayAgain };

struct Button {
    ButtonId id;
    Rect rect;               // clickable area: the painted placard
    std::string_view label;  // painted lettering
};

namespace ui {

// The hanging board every overlay is painted on (UI_PANEL, ~1000x700),
// centred on the canvas (brief §6.2).
inline constexpr Rect kPanel{460.0f, 190.0f, 1000.0f, 700.0f};

// Placards: modest, sign-sized, never full-width slabs.
inline constexpr float kButtonWidth = 380.0f;
inline constexpr float kButtonHeight = 52.0f;
inline constexpr float kButtonGap = 14.0f;

// Buttons for a state, top to bottom. Empty in Playing and Exiting.
[[nodiscard]] std::span<const Button> buttons(AppState state) noexcept;

// The button under a logical point, if any.
[[nodiscard]] std::optional<ButtonId> hit(AppState state, Vec2 p) noexcept;

} // namespace ui
} // namespace gallery
