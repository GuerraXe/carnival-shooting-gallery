#include "shell/app/Application.hpp"

#include "shell/platform/Logger.hpp"
#include "gallery/view/SceneLayout.hpp"
#include "shell/platform/Platform.hpp"
#include "shell/platform/StartupError.hpp"
#include "shell/platform/Window.hpp"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/Window/VideoMode.hpp>

#include <exception>
#include <format>
#include <type_traits>
#include <utility>

namespace shell {

namespace {

#ifdef NDEBUG
constexpr bool kDebugBuild = false;
#else
constexpr bool kDebugBuild = true;
#endif

// Preview distribution (D-033): an optimized build that is allowed to run
// with labelled stand-ins while the MVP photographs are missing. It says so
// in the window title. The strict Release build is unaffected (D-023).
#if defined(GALLERY_PREVIEW) && GALLERY_PREVIEW
constexpr bool kPreviewBuild = true;
constexpr const char* kWindowTitle = "Carnival Shooting Gallery (Preview)";
#else
constexpr bool kPreviewBuild = false;
constexpr const char* kWindowTitle = "Carnival Shooting Gallery";
#endif

template <class... Fs>
struct Overloaded : Fs... {
    using Fs::operator()...;
};

} // namespace

Application::Application(Logger& logger, LaunchOptions options)
    : log_(logger),
      options_(std::move(options)),
      paths_(options_.assetRoot ? *options_.assetRoot : AssetPaths::defaultRoot()) {
    log_.info(std::format("Carnival Shooting Gallery {} starting ({} build)", GALLERY_VERSION,
                          kDebugBuild ? "Debug" : kPreviewBuild ? "Preview" : "Release"));
    log_.info("Executable directory: " + platform::toUtf8(platform::exeDirectory()));
    log_.info("Asset root: " + platform::toUtf8(paths_.root()));
    log_.info("Log file: " + platform::toUtf8(log_.filePath()) +
              (log_.fileOpen() ? "" : " (NOT writable; stderr only)"));

    // Size the window for the display's scaling (R-6): same apparent size at
    // 100 % and 250 %, never larger than 85 % of the desktop.
    const unsigned dpi = platform::systemDpi();
    const sf::Vector2u desktop = sf::VideoMode::getDesktopMode().size;
    gallery::Size initial = gallery::initialWindowSize(
        {static_cast<float>(desktop.x), static_cast<float>(desktop.y)},
        static_cast<float>(dpi) / 96.0f);
    if (options_.windowWidth && options_.windowHeight) {
        initial = {static_cast<float>(*options_.windowWidth),
                   static_cast<float>(*options_.windowHeight)};
    }
    log_.info(std::format("Display: desktop {}x{} px, system DPI {} ({}% scaling)", desktop.x,
                          desktop.y, dpi, dpi * 100 / 96));
    platform::openWindow(window_,
                         platform::WindowConfig{.title = kWindowTitle,
                                                .width = static_cast<unsigned>(initial.width),
                                                .height = static_cast<unsigned>(initial.height)});
    const sf::Vector2u size = window_.getSize();
    const sf::ContextSettings gl = window_.getSettings();
    log_.info(std::format("Window open: {}x{} px, DPI {}, OpenGL {}.{}, AA x{}, vsync on", size.x,
                          size.y, platform::windowDpi(window_.getNativeHandle()),
                          gl.majorVersion, gl.minorVersion, gl.antiAliasingLevel));
    onResize({static_cast<float>(size.x), static_cast<float>(size.y)});

    // Assets need the GL context, so they load after the window (§3.2).
    const bool standIns = options_.standIns || kDebugBuild || kPreviewBuild;
    store_.emplace(paths_, log_, standIns ? MissingAssetPolicy::StandIn : MissingAssetPolicy::Fail);

    // Targets (D-027): shapes from the images that will be drawn, then the
    // configured lane, validated against the booth before anything moves.
    duckVisuals_.emplace(*store_, log_);
    gallery::LaneConfig lane = gallery::makeLaneConfig(options_.lanePreset);
    lane.mix = gallery::standardMix();   // decoys and golden power-up targets (D-035)
    if (options_.reaction) {
        gallery::setReaction(lane, *options_.reaction);
    }
    const auto issues = gallery::validateLane(lane, duckVisuals_->widths(),
                                              gallery::layout::kDuckHeight,
                                              gallery::layout::kOpening);
    if (!issues.empty()) {
        std::string msg = std::format("Target configuration '{}' is invalid:\n",
                                      gallery::lanePresetName(options_.lanePreset));
        for (const auto& i : issues) {
            msg += "  " + i + "\n";
        }
        throw StartupError(msg);
    }
    // The renderer is built before the game (declaration order, and so
    // teardown order, is unchanged) because the round needs the foreground
    // it draws in front of the targets: shots there hit the scenery (G-16).
    renderer_.emplace(*store_, *duckVisuals_, log_);
    game_.emplace(gallery::Round(gallery::DuckLane(std::move(lane), duckVisuals_->shapes()),
                                 gallery::ShotTuning::standard(), gallery::RoundTuning{},
                                 renderer_->buildForegroundOccluder()));
    log_.info(std::format("Targets: {} preset, {} target(s), {} reaction",
                          gallery::lanePresetName(options_.lanePreset), game_->round().lane().size(),
                          reactionName()));
    for (std::size_t i = 0; i < game_->round().lane().size(); ++i) {
        const gallery::DuckConfig& c = game_->round().lane().config(i);
        const gallery::DuckMotion& m = game_->round().lane().motionModel(i);
        log_.info(std::format("  target #{}: {} [{:.0f}, {:.0f}] speed {:.0f} u/s, turn {:.2f} s, "
                              "dwell {:.2f} s, pass {:.2f} s, cycle {:.2f} s, phase {:.2f}, "
                              "variant {}, min down {:.1f} s",
                              i, c.mode == gallery::MotionMode::Shuttle ? "shuttle" : "chain",
                              c.boundsMin, c.boundsMax, c.speed, c.turnTime, c.dwell,
                              m.halfCycle(), m.period(), c.phase,
                              static_cast<char>('A' + c.variant), c.minDownTime));
    }

    renderer_->setFlicker(options_.flicker);
    if (!options_.flicker) {
        log_.info("Lighting: steady (--no-flicker)");
    }
    audio_.emplace(*store_, log_,
                   AudioSettings{.enabled = options_.audio,
                                 .masterPercent = options_.volumePercent,
                                 .muted = options_.mute,
                                 .music = options_.music,
                                 .logCues = options_.logAudio});
    renderer_->setInspect(options_.inspectTargets ? render::Renderer::InspectMode::Lane
                          : options_.inspect      ? render::Renderer::InspectMode::Full
                                                  : render::Renderer::InspectMode::Off);
    if (options_.aimX && options_.aimY) {
        setAim(viewport_.clampToLogical({*options_.aimX, *options_.aimY}));
    }

    if (options_.fps) {
        window_.setVerticalSyncEnabled(false);
        window_.setFramerateLimit(*options_.fps);
        log_.info(std::format("Frame rate: vsync OFF, capped at {} fps (--fps)", *options_.fps));
    }
    if (options_.simDt) {
        log_.info(std::format("Simulation time: fixed {:.6f} s per frame (--sim-dt)", *options_.simDt));
    }

    // The OS pointer is shown on the boards and hidden (and confined) during
    // a round, where the game draws its own reticle (D-018, G-3, G-4).
    applyCursorPolicy();
    log_.info(std::format("Round: {:.1f} s countdown, {:.0f} s, results after {:.1f} s",
                          game_->round().clock().tuning().countdown,
                          game_->round().clock().tuning().length,
                          game_->round().clock().tuning().endDelay));
    if (automated()) {
        log_.info("Automated run (--frames / scripted input): real mouse, keyboard and focus "
                  "events are ignored; the pointer is never confined");
    }
    log_.info("Boot complete; state Title (main menu)");
}

std::string_view Application::reactionName() const noexcept {
    return game_ && game_->round().lane().size() > 0 &&
                   game_->round().lane().config(0).reaction == gallery::HitReaction::TiltDrop
               ? "drop"
               : "flip";
}

void Application::aimAtWindowPixel(gallery::Vec2 px) {
    if (!aimLocked()) {
        // Window pixels -> logical canvas through the letterbox viewport, then
        // clamped onto the canvas (D-006, D-015). The reticle is drawn at this
        // exact point and the shot resolves at it.
        setAim(viewport_.clampToLogical(viewport_.toLogical(px)));
    }
}

void Application::setAim(gallery::Vec2 logical) {
    aim_ = logical;
    renderer_->setAim(aim_);
}

// ---------------------------------------------------------------------------
// Game input and its side effects
// ---------------------------------------------------------------------------

void Application::dispatch(const gallery::InputEvent& e) {
    const gallery::AppState before = game_->state();
    if (const auto shot = game_->handle(e)) {
        logShot(*shot);
    }
    if (gallery::isUiAction(before, game_->state(), e)) {
        audio_->onUiAction();   // wooden click on a board action, this frame
    }
    syncState();
}

void Application::logShot(const gallery::ShotResult& r) {
    const gallery::Round& round = game_->round();
    const gallery::RoundStats& s = round.stats();
    switch (r.outcome) {
    case gallery::ShotOutcome::Rejected:
        switch (r.reason) {
        case gallery::RejectReason::Cooldown:
            log_.info(std::format("Click ignored: rifle cooling down ({:.0f} ms left); not a shot",
                                  round.rifle().remaining(round.time()) * 1000.0));
            break;
        case gallery::RejectReason::Empty:
            log_.info("Click on an empty magazine: dry fire, not a shot (R / right-click reloads)");
            break;
        case gallery::RejectReason::Reloading:
            log_.info("Click ignored: reloading; not a shot");
            break;
        case gallery::RejectReason::NotLive:
            log_.info(std::format("Click ignored: round {} (not live); not a shot",
                                  gallery::roundPhaseName(round.clock().phase())));
            break;
        default: log_.info("Click ignored: invalid aim"); break;
        }
        return;
    case gallery::ShotOutcome::Hit:
        log_.info(std::format("Shot {} at ({:.1f}, {:.1f}): HIT target #{} +{}  score {}", s.shots(),
                              aim_.x, aim_.y, *r.duck, round.tuning().pointsPerHit, s.score()));
        break;
    case gallery::ShotOutcome::Miss:
        log_.info(std::format("Shot {} at ({:.1f}, {:.1f}): miss  score {}", s.shots(), aim_.x,
                              aim_.y, s.score()));
        break;
    case gallery::ShotOutcome::Decoy:
        log_.info(std::format("Shot {} at ({:.1f}, {:.1f}): DECOY target #{} -{}  score {}", s.shots(),
                              aim_.x, aim_.y, *r.duck, round.tuning().decoyPenalty, s.score()));
        break;
    }
}

void Application::logEvents() {
    for (const gallery::GameEvent& e : game_->events()) {
        if (std::holds_alternative<gallery::event::RoundReset>(e)) {
            log_.info("Round reset: targets home, rifle ready, stats zero, clock at countdown");
        } else if (std::holds_alternative<gallery::event::Go>(e)) {
            log_.info(std::format("GO: round live, {:.0f} s on the clock",
                                  game_->round().clock().tuning().length));
        } else if (const auto* t = std::get_if<gallery::event::LowTimeTick>(&e)) {
            log_.info(std::format("Low time: {} s left", t->secondsLeft));
        } else if (std::holds_alternative<gallery::event::ReloadStarted>(e)) {
            log_.info("Reloading");
        } else if (std::holds_alternative<gallery::event::Reloaded>(e)) {
            log_.info(std::format("Reloaded: {} rounds", game_->round().ammo()));
        } else if (const auto* p = std::get_if<gallery::event::PowerUp>(&e)) {
            log_.info(std::format("POWER-UP: free ammo for {:.0f} s", p->duration));
        } else if (std::holds_alternative<gallery::event::PowerUpEnded>(e)) {
            log_.info("Power-up over");
        } else if (std::holds_alternative<gallery::event::TimeUp>(e)) {
            log_.info(std::format("TIME UP at round t={:.3f} s: input disabled, targets stopped",
                                  game_->round().time()));
        } else if (const auto* f = std::get_if<gallery::event::RoundFinished>(&e)) {
            const auto acc = f->stats.accuracyPercent();
            log_.info(std::format("Round finished: score {}, shots {}, hits {}, misses {}, "
                                  "decoys {}, accuracy {}",
                                  f->stats.score(), f->stats.shots(), f->stats.hits(),
                                  f->stats.misses(), f->stats.decoys(),
                                  acc ? std::format("{}%", *acc) : std::string("-")));
        }
    }
}

void Application::syncState() {
    const gallery::AppState now = game_->state();
    if (now != shownState_) {
        std::string extra;
        if (now == gallery::AppState::Playing && shownState_ != gallery::AppState::Paused) {
            extra = std::format(" (round {})", game_->roundsStarted());
        } else if (now == gallery::AppState::Paused || shownState_ == gallery::AppState::Paused) {
            extra = std::format(" ({} {:.3f} s left)",
                                gallery::roundPhaseName(game_->round().clock().phase()),
                                game_->round().clock().remaining());
        } else if (now == gallery::AppState::Results) {
            const auto best = game_->best().best();
            extra = std::format(" (session best {}{})", best ? *best : 0,
                                game_->newBest() ? ", NEW" : "");
        }
        log_.info(std::format("State {} -> {}{}", gallery::appStateName(shownState_),
                              gallery::appStateName(now), extra));
        shownState_ = now;
    }
    applyCursorPolicy();
}

void Application::applyCursorPolicy() {
    // Hidden and confined only while a round is being played in a focused
    // window; free and visible on every board and whenever focus is gone
    // (G-3, G-4). Only real changes reach the window.
    const bool capture = game_->state() == gallery::AppState::Playing && focused_;
    if (cursorCaptured_ == capture) {
        return;
    }
    window_.setMouseCursorVisible(!capture);
    // Automated runs never confine the pointer: the person at the desk keeps
    // their mouse while a smoke test plays itself.
    window_.setMouseCursorGrabbed(capture && !automated());
    cursorCaptured_ = capture;
    log_.info(capture ? (automated() ? "Cursor: hidden (round; not confined in an automated run)"
                                     : "Cursor: hidden and confined (round)")
                      : "Cursor: visible and free");
}

void Application::runScript() {
    using Kind = LaunchOptions::ScriptStep::Kind;
    for (const LaunchOptions::ScriptStep& s : options_.script) {
        if (s.frame != ticks_) {   // loop iterations: exactly once, even while minimised
            continue;
        }
        switch (s.kind) {
        case Kind::Click:
            log_.info(std::format("Script: click at ({:.0f}, {:.0f})", aim_.x, aim_.y));
            dispatch(gallery::input::PrimaryPressed{aim_});
            break;
        case Kind::Key:
            log_.info(std::format("Script: key {}", static_cast<int>(s.key)));
            dispatch(gallery::input::KeyPressed{s.key});
            break;
        case Kind::FocusLost:
            log_.info("Script: focus lost");
            focused_ = false;
            dispatch(gallery::input::FocusLost{});
            break;
        case Kind::FocusGained:
            log_.info("Script: focus gained");
            focused_ = true;
            dispatch(gallery::input::FocusGained{});
            break;
        case Kind::Close:
            log_.info("Script: close request");
            dispatch(gallery::input::CloseRequested{});
            break;
        }
    }
}

void Application::knockNextTarget() {
    if (game_->state() != gallery::AppState::Playing) {
        log_.info("Debug knock-down: only during a round");
        return;
    }
    gallery::Round& round = game_->round();
    for (std::size_t n = 0; n < round.lane().size(); ++n) {
        const std::size_t id = (knockCursor_ + n) % round.lane().size();
        if (round.debugKnockDown(id)) {
            knockCursor_ = id + 1;
            log_.info(std::format("Debug knock-down: target #{} (not a shot: no score, no stats)", id));
            return;
        }
    }
    log_.info("Debug knock-down: no upright target");
}

void Application::logTargetTransitions() {
    gallery::DuckLane& lane = game_->round().lane();
    for (const gallery::DuckTransition& t : lane.transitions()) {
        const gallery::MotionSample m = lane.motionAt(t.id, t.at);
        // A rising target has just drawn its new kind (D-035): name it.
        const std::string kind =
            t.to == gallery::DuckState::Rising
                ? std::format(" as {}", gallery::targetKindName(lane.kind(t.id)))
                : std::string();
        log_.info(std::format("Target #{} {} -> {}{} at lane t={:.3f} s (x {:.1f})", t.id,
                              gallery::duckStateName(t.from), gallery::duckStateName(t.to), kind,
                              t.at, m.left));
    }
    lane.clearTransitions();
}

void Application::logLaneSnapshot(const char* when) {
    const gallery::Round& round = game_->round();
    const gallery::RoundStats& s = round.stats();
    const auto acc = s.accuracyPercent();
    log_.info(std::format("Round ({}): state {}, phase {}, {:.3f} s left, rounds started {}", when,
                          gallery::appStateName(game_->state()),
                          gallery::roundPhaseName(round.clock().phase()), round.clock().remaining(),
                          game_->roundsStarted()));
    log_.info(std::format("Shots ({}): {} fired, {} hits, {} misses, score {}, accuracy {}", when,
                          s.shots(), s.hits(), s.misses(), s.score(),
                          acc ? std::format("{}%", *acc) : std::string("-")));
    log_.info(std::format("Target snapshot ({}): lane t={:.6f} s", when, round.lane().time()));
    for (std::size_t i = 0; i < round.lane().size(); ++i) {
        const gallery::MotionSample m = round.lane().motion(i);
        log_.info(std::format("  #{} {:<7} x={:.4f} v={:+.4f} dir={:+d} edges={}", i,
                              gallery::duckStateName(round.lane().state(i)), m.left, m.velocity,
                              m.direction, m.edges));
    }
}

Application::~Application() {
    log_.info(std::format("Shutting down: releasing audio, renderer, assets and window ({} frames "
                          "rendered)",
                          frames_));
    // Explicit and first: every voice and stream stops while the buffers it
    // reads (store_) and the window are still alive. Member order would do
    // the same; this makes it visible and keeps the log in order.
    audio_.reset();
}

// ---------------------------------------------------------------------------
// Main loop (ARCHITECTURE §5.1)
// ---------------------------------------------------------------------------

int Application::run() {
    log_.info("Entering main loop");
    // Restart the frame clock: boot time (asset loading) is not a frame.
    (void)clock_.frameDelta();
    try {
        while (window_.isOpen() && !quitRequested_) {
            ++ticks_;
            lastDt_ = clock_.frameDelta();
            // The wall time just measured is how long the previous iteration
            // took. Skip the very first frame (GL warm-up) and minimised ones.
            if (renderedLast_ && ticks_ > 2) {
                frameTimes_.add(clock_.lastRaw());
            }
            renderedLast_ = false;
            if (options_.simDt) {
                lastDt_ = *options_.simDt;   // deterministic frame-rate checks
            }

            // 1. INPUT — drained before any update (D-008). Each event is
            //    handled exactly once, by the current state only.
            while (const std::optional event = window_.pollEvent()) {
                if (const auto input = platform::translateEvent(*event)) {
                    handle(*input);
                }
            }
            runScript();
            if (options_.knockAtFrame && ticks_ == *options_.knockAtFrame) {
                knockNextTarget();
            }
            if (game_->state() == gallery::AppState::Exiting || quitRequested_) {
                break;
            }

            // 2. UPDATE — the state machine advances the round only while
            //    playing; the timer cannot run on a board or while paused.
            game_->update(lastDt_);
            // 3. REACT — presentation consumes this frame's events (shots,
            //    clock boundaries, reset) and target transitions, so sound
            //    and picture land in the same frame as the click (G-9).
            audio_->consume(game_->events(), game_->round().lane().transitions());
            logTargetTransitions();   // logs, then clears the transitions
            logEvents();
            syncState();

            renderer_->consume(game_->events());
            game_->clearEvents();
            renderer_->update(lastDt_, game_->state() == gallery::AppState::Playing);
            audio_->update(lastDt_, game_->state(), game_->round().clock().phase(), focused_);

            // 4. RENDER. Skip while minimised so the loop does not spin
            //    (vsync does not throttle an invisible window).
            if (viewport_.degenerate() || platform::isMinimized(window_.getNativeHandle())) {
                sf::sleep(sf::milliseconds(16));
                continue;
            }
            renderer_->draw(window_, viewport_, *game_,
                            render::FrameInfo{.dt = lastDt_,
                                              .focused = focused_,
                                              .showHint = kDebugBuild,
                                              .lanePreset = gallery::lanePresetName(options_.lanePreset),
                                              .reaction = reactionName()});
            ++frames_;
            renderedLast_ = true;
            const bool lastFrame = options_.maxFrames && frames_ >= *options_.maxFrames;
            if (lastFrame && options_.screenshot) {
                saveScreenshot();
            }
            window_.display();
            if (lastFrame) {
                log_.info(std::format("Frame limit ({}) reached; requesting shutdown", frames_));
                quitRequested_ = true;
            }
        }
    } catch (const std::exception& e) {
        // The per-frame path is designed not to throw; if it does, shut down
        // cleanly and report rather than terminate.
        log_.error(std::string("Unexpected error in main loop: ") + e.what());
        window_.close();
        return 1;
    }
    if (game_->state() == gallery::AppState::Exiting) {
        log_.info("Quit requested by the game (Esc / Quit / window close)");
    }
    // Hand the pointer back before the window goes.
    window_.setMouseCursorGrabbed(false);
    window_.setMouseCursorVisible(true);
    window_.close();
    logLaneSnapshot("exit");
    if (frameTimes_.count() > 0) {
        const gallery::FrameStats& f = frameTimes_;
        log_.info(std::format("Frame times ({} frames): mean {:.2f} ms ({:.0f} fps), p50 {:.1f} ms, "
                              "p95 {:.1f} ms, p99 {:.1f} ms, max {:.1f} ms; {} frame(s) over 33 ms "
                              "(AV-7)",
                              f.count(), f.mean() * 1000.0, f.mean() > 0.0 ? 1.0 / f.mean() : 0.0,
                              f.percentile(0.50) * 1000.0, f.percentile(0.95) * 1000.0,
                              f.percentile(0.99) * 1000.0, f.max() * 1000.0, f.hitches()));
    }
    log_.info("Main loop exited normally");
    return 0;
}

void Application::handle(const platform::RawInput& input) {
    using namespace platform;
    std::visit(
        Overloaded{
            [&](const raw::CloseRequested&) {
                log_.info("Close requested by the window system");
                dispatch(gallery::input::CloseRequested{});
            },
            [&](const raw::Resized& e) { onResize(e.size); },
            [&](const raw::FocusLost&) {
                if (automated()) {
                    log_.info("Real focus loss ignored (automated run; use --focus-lost)");
                    return;
                }
                focused_ = false;
                log_.info("Focus lost");
                dispatch(gallery::input::FocusLost{});
            },
            [&](const raw::FocusGained&) {
                if (automated()) {
                    return;
                }
                focused_ = true;
                log_.info("Focus gained");
                dispatch(gallery::input::FocusGained{});
            },
            [&](const raw::PointerMoved& e) {
                if (automated()) {
                    return;   // scripted runs are deterministic: real input is ignored
                }
                aimAtWindowPixel(e.px);
                dispatch(gallery::input::PointerMoved{aim_});
            },
            [&](const raw::PrimaryPressed& e) {
                if (automated()) {
                    log_.info("Real click ignored (automated run)");
                    return;
                }
                // The press position, not the last move, is where it lands.
                aimAtWindowPixel(e.px);
                dispatch(gallery::input::PrimaryPressed{aim_});
            },
            [&](const raw::SecondaryPressed& e) {
                if (automated()) {
                    return;   // scripted runs reload with --key N:r
                }
                aimAtWindowPixel(e.px);
                dispatch(gallery::input::SecondaryPressed{aim_});
            },
            [&](const raw::KeyPressed& e) {
                if (automated()) {
                    log_.info("Real key ignored (automated run)");
                    return;
                }
                switch (e.key) {
                // Game keys: the state machine decides what they mean.
                case Key::Escape: dispatch(gallery::input::KeyPressed{gallery::Key::Escape}); break;
                case Key::Space: dispatch(gallery::input::KeyPressed{gallery::Key::Space}); break;
                case Key::Enter: dispatch(gallery::input::KeyPressed{gallery::Key::Enter}); break;
                case Key::R: dispatch(gallery::input::KeyPressed{gallery::Key::R}); break;
                case Key::P: dispatch(gallery::input::KeyPressed{gallery::Key::P}); break;
                // Inspection keys: shell only, never gameplay. F3, 1-8 and K are
                // development tools (ARCHITECTURE §7: Debug-only overlay); in a
                // player build a stray digit would hide the ducks while they stay
                // hittable, so they are ignored there. --inspect / --knock remain.
                case Key::F3: if constexpr (kDebugBuild) { renderer_->toggleInspect(); } break;
                case Key::F4: renderer_->togglePostProcess(); break;
                case Key::K: if constexpr (kDebugBuild) { knockNextTarget(); } break;
                // Sound settings: shell only, any screen, never gameplay.
                case Key::M: renderer_->showNote(audio_->toggleMute()); break;
                case Key::N: renderer_->showNote(audio_->toggleMusic()); break;
                case Key::VolumeDown: renderer_->showNote(audio_->nudgeVolume(-1)); break;
                case Key::VolumeUp: renderer_->showNote(audio_->nudgeVolume(+1)); break;
                case Key::Digit1: case Key::Digit2: case Key::Digit3: case Key::Digit4:
                case Key::Digit5: case Key::Digit6: case Key::Digit7: case Key::Digit8:
                    if constexpr (!kDebugBuild) {
                        break;
                    }
                    renderer_->toggleGroup(static_cast<gallery::LayerGroup>(
                        static_cast<int>(e.key) - static_cast<int>(Key::Digit1)));
                    break;
                default: break;
                }
            },
        },
        input);
}

void Application::onResize(gallery::Size windowPx) {
    viewport_ = gallery::Viewport::fit(windowPx);
    const gallery::Rect r = viewport_.pixelRect();
    log_.info(std::format("Viewport: window {:.0f}x{:.0f} -> canvas {:.0f}x{:.0f} at "
                          "({:.0f}, {:.0f}), scale {:.4f}",
                          windowPx.width, windowPx.height, r.width, r.height, r.x, r.y,
                          viewport_.scale()));
}

void Application::saveScreenshot() {
    try {
        sf::Texture capture(window_.getSize());
        capture.update(window_);
        if (capture.copyToImage().saveToFile(*options_.screenshot)) {
            log_.info("Screenshot saved: " + platform::toUtf8(*options_.screenshot));
        } else {
            log_.warn("Screenshot could not be written: " +
                      platform::toUtf8(*options_.screenshot));
        }
    } catch (const std::exception& e) {
        log_.warn(std::string("Screenshot failed: ") + e.what());
    }
}

} // namespace shell
