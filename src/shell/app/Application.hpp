#pragma once

#include "gallery/core/FrameStats.hpp"
#include "gallery/core/Geometry.hpp"
#include "gallery/core/Time.hpp"
#include "gallery/game/DuckLane.hpp"
#include "gallery/flow/Game.hpp"
#include "gallery/game/Shooting.hpp"
#include "gallery/view/Viewport.hpp"
#include "shell/app/FrameClock.hpp"
#include "shell/app/LaunchOptions.hpp"
#include "shell/assets/AssetPaths.hpp"
#include "shell/assets/AssetStore.hpp"
#include "shell/audio/AudioSystem.hpp"
#include "shell/platform/Input.hpp"
#include "shell/render/DuckVisuals.hpp"
#include "shell/render/Renderer.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace shell {

class Logger;

// Owns the process lifecycle: boot, main loop, orderly teardown
// (ARCHITECTURE §3, §5). Translates SFML input into logical game input,
// drives gallery::Game (the state machine), and applies the side effects the
// logic asks for by its state: cursor capture, quitting (D-029).
class Application {
public:
    // Boot. Throws StartupError on unrecoverable problems (missing asset
    // folder, no window, missing MVP assets without --stand-ins).
    Application(Logger& logger, LaunchOptions options);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    // Runs until the window closes or quit is requested. Returns the process
    // exit code. Designed not to throw (ARCHITECTURE §6).
    [[nodiscard]] int run();

private:
    void handle(const platform::RawInput& input);
    void onResize(gallery::Size windowPx);
    void saveScreenshot();
    void aimAtWindowPixel(gallery::Vec2 px);    // mouse -> logical aim (the one conversion)
    void setAim(gallery::Vec2 logical);
    void dispatch(const gallery::InputEvent& e);   // to the game, then sync side effects
    void runScript();                           // --fire/--key/... steps due this frame
    void logShot(const gallery::ShotResult& r);
    void logEvents();                           // round clock + shots, for carnival.log
    void syncState();                           // log transitions, apply cursor policy
    void applyCursorPolicy();
    void knockNextTarget();                     // debug hit-reaction trigger (K, --knock)
    void logTargetTransitions();
    void logLaneSnapshot(const char* when);
    [[nodiscard]] std::string_view reactionName() const noexcept;
    // --aim pins the aim point (deterministic screenshots); the mouse is ignored.
    [[nodiscard]] bool aimLocked() const noexcept { return options_.aimX.has_value(); }
    // --frames or scripted input: a smoke test. Real mouse and keyboard input
    // is ignored and the pointer is never confined.
    [[nodiscard]] bool automated() const noexcept {
        return options_.maxFrames.has_value() || !options_.script.empty();
    }

    // ---------------------------------------------------------------------
    // MEMBER ORDER IS LOAD-BEARING (ARCHITECTURE §3.2). Members are built
    // top-down and destroyed bottom-up; anything that borrows must come
    // AFTER what it borrows. Do not reorder without updating §3.2.
    // The Logger is owned by main() (it must outlive a throwing constructor
    // so the failure can be logged) and borrowed here.
    // store_ and renderer_ are optional only so they can be emplaced after
    // the window has created the GL context; they are always engaged once
    // the constructor returns.
    // ---------------------------------------------------------------------
    Logger& log_;
    LaunchOptions options_;
    AssetPaths paths_;                          // validated before the window: fail fast, no flash
    sf::RenderWindow window_;                   // owns the GL context textures need
    std::optional<AssetStore> store_;           // textures, font, shader source
    std::optional<render::DuckVisuals> duckVisuals_;   // target images + hit shapes (needs store_)
    gallery::Viewport viewport_;
    std::optional<gallery::Game> game_;         // app state machine + round (logic; copies its shapes)
    std::optional<render::Renderer> renderer_;  // borrows store_, duckVisuals_; reads game_
    std::optional<AudioSystem> audio_;          // borrows store_'s sound buffers: declared after
                                                // it, so every sf::Sound dies before its buffer
    FrameClock clock_;

    // Loop state.
    gallery::Seconds lastDt_ = 0.0;
    bool focused_ = true;                       // window focus (cursor capture needs it)
    bool quitRequested_ = false;                // --frames limit or a main-loop failure
    gallery::AppState shownState_ = gallery::AppState::Title;   // last state synced to the shell
    std::optional<bool> cursorCaptured_;        // what the window was last told
    gallery::Vec2 aim_{gallery::kLogicalSize.width / 2.0f, gallery::kLogicalSize.height / 2.0f};
    std::size_t knockCursor_ = 0;
    std::uint64_t frames_ = 0;                  // frames rendered (--frames, screenshots)
    std::uint64_t ticks_ = 0;                   // loop iterations, 1-based (scripted input)
    gallery::FrameStats frameTimes_;            // wall time of every rendered frame (AV-7)
    bool renderedLast_ = false;                 // the previous iteration drew a frame
};

} // namespace shell
