#include "gallery/flow/UiLayout.hpp"

#include <array>

namespace gallery {

const char* appStateName(AppState state) noexcept {
    switch (state) {
    case AppState::Title: return "Title";
    case AppState::Playing: return "Playing";
    case AppState::Paused: return "Paused";
    case AppState::Results: return "Results";
    case AppState::Exiting: return "Exiting";
    }
    return "?";
}

namespace ui {

namespace {

// Placard `row` of a stack whose first placard's top edge is at `top`.
constexpr Rect placard(float top, int row) noexcept {
    return {kPanel.x + (kPanel.width - kButtonWidth) / 2.0f,
            top + static_cast<float>(row) * (kButtonHeight + kButtonGap), kButtonWidth,
            kButtonHeight};
}

// The stacks end the same distance above the panel's bottom edge.
constexpr float stackTop(int count) noexcept {
    constexpr float bottomMargin = 56.0f;
    return kPanel.y + kPanel.height - bottomMargin -
           static_cast<float>(count) * kButtonHeight -
           static_cast<float>(count - 1) * kButtonGap;
}

constexpr std::array<Button, 2> kTitle{{
    {ButtonId::StartRound, placard(stackTop(2), 0), "Start Round"},
    {ButtonId::Quit, placard(stackTop(2), 1), "Quit"},
}};

constexpr std::array<Button, 3> kPaused{{
    {ButtonId::Resume, placard(stackTop(3), 0), "Resume"},
    {ButtonId::RestartRound, placard(stackTop(3), 1), "Restart Round"},
    {ButtonId::MainMenu, placard(stackTop(3), 2), "Main Menu"},
}};

constexpr std::array<Button, 2> kResults{{
    {ButtonId::PlayAgain, placard(stackTop(2), 0), "Play Again"},
    {ButtonId::MainMenu, placard(stackTop(2), 1), "Main Menu"},
}};

} // namespace

std::span<const Button> buttons(AppState state) noexcept {
    switch (state) {
    case AppState::Title: return kTitle;
    case AppState::Paused: return kPaused;
    case AppState::Results: return kResults;
    case AppState::Playing:
    case AppState::Exiting: break;
    }
    return {};
}

std::optional<ButtonId> hit(AppState state, Vec2 p) noexcept {
    for (const Button& b : buttons(state)) {
        if (b.rect.contains(p)) {
            return b.id;
        }
    }
    return std::nullopt;
}

} // namespace ui
} // namespace gallery
