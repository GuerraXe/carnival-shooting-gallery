#pragma once

// The application state machine (ARCHITECTURE §4.1, D-011, D-029). Pure
// logic: the shell feeds it logical input and frame time, reads its state,
// and applies the side effects (cursor, window close) itself.
//
//            Start / Space / Enter                 Esc / P / focus lost
//   Title ─────────────────────────► Playing ─────────────────────────► Paused
//     ▲ │                              │  ▲ ◄── Resume / click / Space / Enter / P
//     │ └─ Esc / Quit ──► Exiting      │  └──── Restart / R (fresh round) ──┘ │
//     │                                │ round finished (TimeUp + 1 s)        │
//     │                                ▼                                      │
//     ├──── Main Menu ───────────── Results ── Play Again / R / Enter ──► Playing
//     └──── Main Menu / Esc ─────────────────────────────────────────── Paused
//   Results ── Esc ──► Exiting.     Any state ── window close ──► Exiting.
//
// One enum holds the whole application mode; the round's own progress
// (countdown, live, ending) is RoundClock's phase. There are no other flags.

#include "gallery/core/Geometry.hpp"
#include "gallery/core/Time.hpp"
#include "gallery/flow/UiLayout.hpp"
#include "gallery/game/Shooting.hpp"
#include "gallery/view/Viewport.hpp"

#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace gallery {

// The keys the game itself reacts to (debug keys stay in the shell).
enum class Key : std::uint8_t { Escape, Space, Enter, R, P };

// Input in LOGICAL coordinates (the shell converts and clamps, D-015).
namespace input {
struct PointerMoved { Vec2 at; };
struct PrimaryPressed { Vec2 at; };
struct SecondaryPressed { Vec2 at; };   // right button: reload during a round (D-035)
struct KeyPressed { Key key; };
struct FocusLost {};
struct FocusGained {};
struct CloseRequested {};
} // namespace input
using InputEvent = std::variant<input::PointerMoved, input::PrimaryPressed, input::SecondaryPressed,
                                input::KeyPressed,
                                input::FocusLost, input::FocusGained, input::CloseRequested>;

// Best score of this session (G-31). Memory only.
class SessionBest {
public:
    // Records a finished round. True when it beat an earlier round's best.
    bool submit(int score) noexcept;
    [[nodiscard]] std::optional<int> best() const noexcept { return best_; }

private:
    std::optional<int> best_;
};

class Game {
public:
    explicit Game(Round round);

    // Route one input event. Returns the shot result when the event was a
    // trigger pull during a round (accepted or rejected); nullopt when the
    // event was a menu action or ignored, i.e. could never be a shot.
    std::optional<ShotResult> handle(const InputEvent& e);

    // Advance time. Only Playing advances the round: Title, Paused and
    // Results freeze the clock and the targets (AC-15).
    void update(Seconds dt);

    [[nodiscard]] AppState state() const noexcept { return state_; }
    [[nodiscard]] const Round& round() const noexcept { return round_; }
    [[nodiscard]] Round& round() noexcept { return round_; }     // debug knock-down only
    [[nodiscard]] Vec2 aim() const noexcept { return aim_; }
    // The placard under the pointer on the current screen.
    [[nodiscard]] std::optional<ButtonId> hovered() const noexcept { return ui::hit(state_, aim_); }
    [[nodiscard]] const SessionBest& best() const noexcept { return best_; }
    // True on the Results screen when this round set a new session best.
    [[nodiscard]] bool newBest() const noexcept { return newBest_; }
    [[nodiscard]] int roundsStarted() const noexcept { return roundsStarted_; }

    [[nodiscard]] const std::vector<GameEvent>& events() const noexcept { return round_.events(); }
    void clearEvents() noexcept { round_.clearEvents(); }

private:
    std::optional<ShotResult> onPress(Vec2 at);
    void onKey(Key key);
    void activate(ButtonId id);

    void startRound();      // any state -> Playing with a clean round
    void pause();
    void resume();
    void toTitle();
    void toResults();

    Round round_;
    SessionBest best_;
    AppState state_ = AppState::Title;
    Vec2 aim_{kLogicalSize.width / 2.0f, kLogicalSize.height / 2.0f};
    bool newBest_ = false;
    int roundsStarted_ = 0;
};

} // namespace gallery
