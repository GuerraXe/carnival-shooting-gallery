#include "gallery/flow/Game.hpp"

#include <utility>

namespace gallery {

namespace {
template <class... Fs>
struct Overloaded : Fs... {
    using Fs::operator()...;
};

// Pausing makes sense only while the round is still running. Once time is up
// input is disabled (G-28) and the round runs out its 1 s end delay.
bool pausable(const Round& r) noexcept {
    return r.clock().targetsMoving();
}
} // namespace

// ---------------------------------------------------------------------------
// SessionBest
// ---------------------------------------------------------------------------

bool SessionBest::submit(int score) noexcept {
    if (!best_) {
        best_ = score;
        return false;   // the first round sets the best; there is nothing to beat
    }
    if (score > *best_) {
        best_ = score;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Game
// ---------------------------------------------------------------------------

Game::Game(Round round) : round_(std::move(round)) {
    // The title screen shows the booth at rest with a clean round set up.
    round_.reset();
}

std::optional<ShotResult> Game::handle(const InputEvent& e) {
    if (state_ == AppState::Exiting) {
        return std::nullopt;
    }
    return std::visit(
        Overloaded{
            [&](const input::PointerMoved& m) -> std::optional<ShotResult> {
                aim_ = m.at;
                return std::nullopt;
            },
            [&](const input::PrimaryPressed& p) { return onPress(p.at); },
            [&](const input::SecondaryPressed& p) -> std::optional<ShotResult> {
                aim_ = p.at;
                if (state_ == AppState::Playing) {
                    round_.reload();   // never a shot; ignored on every board
                }
                return std::nullopt;
            },
            [&](const input::KeyPressed& k) -> std::optional<ShotResult> {
                onKey(k.key);
                return std::nullopt;
            },
            [&](const input::FocusLost&) -> std::optional<ShotResult> {
                if (state_ == AppState::Playing && pausable(round_)) {
                    pause();   // G-29; focus regained never auto-resumes
                }
                return std::nullopt;
            },
            [&](const input::FocusGained&) -> std::optional<ShotResult> { return std::nullopt; },
            [&](const input::CloseRequested&) -> std::optional<ShotResult> {
                state_ = AppState::Exiting;
                return std::nullopt;
            },
        },
        e);
}

std::optional<ShotResult> Game::onPress(Vec2 at) {
    aim_ = at;   // the press position, not the last move, is where it lands
    switch (state_) {
    case AppState::Playing:
        // The only place a click becomes a trigger pull. Round::fire still
        // refuses it outside the live phase (countdown, time up).
        return round_.fire(at);
    case AppState::Paused:
        // A placard does its job; a click anywhere else resumes. Neither is
        // a shot (G-29, AC-15).
        if (const auto b = ui::hit(state_, at)) {
            activate(*b);
        } else {
            resume();
        }
        return std::nullopt;
    case AppState::Title:
    case AppState::Results:
        if (const auto b = ui::hit(state_, at)) {
            activate(*b);
        }
        return std::nullopt;   // menus only, never shots (G-11, AC-16)
    case AppState::Exiting: break;
    }
    return std::nullopt;
}

void Game::onKey(Key key) {
    switch (state_) {
    case AppState::Title:
        if (key == Key::Space || key == Key::Enter) {
            startRound();
        } else if (key == Key::Escape) {
            state_ = AppState::Exiting;
        }
        break;
    case AppState::Playing:
        if ((key == Key::Escape || key == Key::P) && pausable(round_)) {
            pause();
        } else if (key == Key::R) {
            round_.reload();   // D-035; restart stays on the pause board
        }
        break;
    case AppState::Paused:
        if (key == Key::Space || key == Key::Enter || key == Key::P) {
            resume();
        } else if (key == Key::R) {
            startRound();
        } else if (key == Key::Escape) {
            toTitle();
        }
        break;
    case AppState::Results:
        if (key == Key::Space || key == Key::Enter || key == Key::R) {
            startRound();
        } else if (key == Key::Escape) {
            state_ = AppState::Exiting;
        }
        break;
    case AppState::Exiting: break;
    }
}

void Game::activate(ButtonId id) {
    switch (id) {
    case ButtonId::StartRound:
    case ButtonId::RestartRound:
    case ButtonId::PlayAgain: startRound(); break;
    case ButtonId::Resume: resume(); break;
    case ButtonId::MainMenu: toTitle(); break;
    case ButtonId::Quit: state_ = AppState::Exiting; break;
    }
}

void Game::update(Seconds dt) {
    if (state_ != AppState::Playing) {
        return;   // frozen: no clock, no targets, no rifle cooldown
    }
    round_.update(dt);
    if (round_.clock().phase() == RoundPhase::Finished) {
        toResults();
    }
}

void Game::startRound() {
    round_.reset();   // emits RoundReset: the renderer clears effects
    newBest_ = false;
    ++roundsStarted_;
    state_ = AppState::Playing;
}

void Game::pause() {
    state_ = AppState::Paused;
}

void Game::resume() {
    state_ = AppState::Playing;
}

void Game::toTitle() {
    // An abandoned round is discarded, not scored, and the booth is reset so
    // the title screen never shows stale marks or knocked-down targets.
    round_.reset();
    newBest_ = false;
    state_ = AppState::Title;
}

void Game::toResults() {
    newBest_ = best_.submit(round_.stats().score());   // exactly once per round
    state_ = AppState::Results;
}

} // namespace gallery
