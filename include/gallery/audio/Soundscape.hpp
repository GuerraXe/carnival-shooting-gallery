#pragma once

// The game's soundscape as pure rules (D-030). No SFML: the shell's
// AudioSystem asks this module WHAT to play, WHEN and HOW LOUD, and only does
// the playing. Everything here is unit-tested (tests/soundscape_tests.cpp).
//
//   game events + target transitions ──► cuesFor…() ──► CueRequest list
//   app state + round phase           ──► bedLevels() ──► ambience / track / music
//   player settings                   ──► Mixer       ──► per-bus gain
//
// Real recordings only: every cue names a catalog asset (Asset Guide §4.2).
// A cue whose asset is missing is simply silent; nothing is synthesised.

#include "gallery/assets/AssetCatalog.hpp"
#include "gallery/core/Time.hpp"
#include "gallery/flow/Game.hpp"
#include "gallery/game/DuckLane.hpp"
#include "gallery/game/RoundClock.hpp"
#include "gallery/game/Shooting.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace gallery {

// One-shot sounds.
enum class Cue : std::uint8_t {
    Shot,        // every accepted shot (G-9)
    DuckHit,     // pellet strikes the sheet-metal target (G-17)
    DuckFlip,    // the target slams flat on its hinge (end of the fall)
    DuckReset,   // the target swings back upright (start of the rise)
    Miss,        // pellet into the wooden backboard
    BellStart,   // "Go!" (G-26)
    Tick,        // each of the final ten seconds (G-27)
    EndBell,     // time up (G-28), rung twice
    UiClick,     // a board action: placard pressed, pause, resume
    ScoreTick,   // the score board's counter advancing after a hit
    Results,     // the results tally rung up
    // D-035. No dedicated recordings exist yet, so these reuse cleared CC0
    // sounds (Asset Guide §4.2: real recordings only, never synthesised).
    DryFire,     // trigger on an empty chamber (wooden click)
    Reload,      // working the action to reload (hinge rattle)
    Reloaded,    // action closed, magazine full (wooden click)
    PowerUp,     // golden target: free ammo (register bell)
    Count
};
inline constexpr std::size_t kCueCount = static_cast<std::size_t>(Cue::Count);

// Mixer buses. Every sound belongs to exactly one.
enum class Bus : std::uint8_t { Effects, Interface, Ambience, Music, Count };
inline constexpr std::size_t kBusCount = static_cast<std::size_t>(Bus::Count);

struct CueSpec {
    Cue cue;
    const char* name;
    AssetId asset;
    Bus bus;
    float gain;          // relative mix level, linear amplitude (0..1]
    float pitchJitter;   // +/- fraction of playback rate per play (0 = never varies)
    float gainJitter;    // +/- fraction of amplitude per play
};

// The mix. Sources are all normalised to -1 dBFS peak (asset-src/work/audio),
// so these numbers ARE the balance: the shot leads, impacts sit just under it,
// clock and board sounds are small, mechanical detail is background.
// Order matches Cue (checked by a test).
inline constexpr std::array<CueSpec, kCueCount> kCueTable{{
    {Cue::Shot, "shot", AssetId::SfxShot, Bus::Effects, 0.90f, 0.03f, 0.08f},
    {Cue::DuckHit, "duck-hit", AssetId::SfxDuckHit, Bus::Effects, 0.80f, 0.04f, 0.08f},
    {Cue::DuckFlip, "duck-flip", AssetId::SfxDuckFlip, Bus::Effects, 0.45f, 0.06f, 0.10f},
    {Cue::DuckReset, "duck-reset", AssetId::SfxDuckReset, Bus::Effects, 0.40f, 0.06f, 0.10f},
    {Cue::Miss, "miss", AssetId::SfxMiss, Bus::Effects, 0.50f, 0.06f, 0.10f},
    {Cue::BellStart, "bell-start", AssetId::SfxBellStart, Bus::Interface, 0.70f, 0.0f, 0.0f},
    {Cue::Tick, "tick", AssetId::SfxTick, Bus::Interface, 0.55f, 0.0f, 0.0f},
    {Cue::EndBell, "end-bell", AssetId::SfxEnd, Bus::Interface, 0.75f, 0.0f, 0.0f},
    {Cue::UiClick, "ui-click", AssetId::SfxUiClick, Bus::Interface, 0.55f, 0.03f, 0.0f},
    {Cue::ScoreTick, "score-tick", AssetId::SfxScoreTick, Bus::Interface, 0.35f, 0.04f, 0.05f},
    {Cue::Results, "results", AssetId::SfxResults, Bus::Interface, 0.60f, 0.0f, 0.0f},
    {Cue::DryFire, "dry-fire", AssetId::SfxUiClick, Bus::Effects, 0.50f, 0.05f, 0.05f},
    {Cue::Reload, "reload", AssetId::SfxDuckReset, Bus::Effects, 0.55f, 0.04f, 0.05f},
    {Cue::Reloaded, "reloaded", AssetId::SfxUiClick, Bus::Effects, 0.60f, 0.03f, 0.0f},
    {Cue::PowerUp, "power-up", AssetId::SfxResults, Bus::Interface, 0.65f, 0.0f, 0.0f},
}};

[[nodiscard]] constexpr const CueSpec& cueSpec(Cue c) noexcept {
    return kCueTable[static_cast<std::size_t>(c)];
}
[[nodiscard]] const char* busName(Bus b) noexcept;

// When a cue should start, relative to the frame that asked for it.
struct CueRequest {
    Cue cue;
    Seconds delay = 0.0;
};

// Timing of the composite cues.
inline constexpr Seconds kScoreTickDelay = 0.09;   // board counter clacks just after the ping
inline constexpr Seconds kEndBellRepeat = 0.40;    // "ding-ding" at time up

// Cues for one game event, appended to `out` (D-010 order is preserved).
// RoundReset is silent: restart must feel instant and clean (G-32).
void cuesForEvent(const GameEvent& e, std::vector<CueRequest>& out);

// Cues for one target state transition: the slam when a fall ends, the
// hinge rattle when a rise begins. Other transitions are silent (the hit
// itself is voiced by DuckHit, which comes from the shot).
void cuesForTransition(const DuckTransition& t, std::vector<CueRequest>& out);

// A board action worth a wooden click: a press or key that changed the
// application state (Start, Resume, Pause, Restart, Play Again, Main Menu).
// Pointer moves, focus changes and window closes are not clicks.
[[nodiscard]] bool isUiAction(AppState before, AppState after, const InputEvent& e) noexcept;

// ---------------------------------------------------------------------------
// Continuous beds
// ---------------------------------------------------------------------------

// Target level (0..1, before the mixer) for each continuous layer.
struct BedLevels {
    float ambience = 0.0f;   // midway crowd and machinery
    float track = 0.0f;      // the target mechanism running
    float music = 0.0f;      // distant band organ (optional asset)
};

// The booth never goes silent on a board, but it settles back while paused
// or out of focus; the target mechanism runs exactly while the targets move.
[[nodiscard]] BedLevels bedLevels(AppState state, RoundPhase phase, bool focused) noexcept;

// Linear ramp toward a target, so beds never click on or off.
class Fader {
public:
    explicit Fader(Seconds rampTime = 0.35) noexcept : rampTime_(rampTime) {}
    // Moves toward `target` by at most dt / rampTime (a full 0 -> 1 swing
    // takes rampTime). Non-finite or negative dt does nothing.
    float step(float target, Seconds dt) noexcept;
    [[nodiscard]] float value() const noexcept { return value_; }
    void snap(float v) noexcept;

private:
    Seconds rampTime_;
    float value_ = 0.0f;
};

// ---------------------------------------------------------------------------
// Player volume
// ---------------------------------------------------------------------------

// Master volume in whole steps of 10 % (0..10), per-bus trims, mute, and an
// optional-music switch. Gain is perceptual: the slider position is squared,
// so each step sounds like a similar change in loudness.
class Mixer {
public:
    static constexpr int kSteps = 10;
    static constexpr int kDefaultStep = 8;

    [[nodiscard]] int masterStep() const noexcept { return masterStep_; }
    [[nodiscard]] int masterPercent() const noexcept { return masterStep_ * 100 / kSteps; }
    void setMasterPercent(int percent) noexcept;   // rounded to the nearest step, clamped
    // +1 / -1 steps; returns true if the volume changed.
    bool nudge(int steps) noexcept;

    [[nodiscard]] bool muted() const noexcept { return muted_; }
    void setMuted(bool m) noexcept { muted_ = m; }
    void toggleMute() noexcept { muted_ = !muted_; }

    [[nodiscard]] bool musicOn() const noexcept { return musicOn_; }
    void setMusicOn(bool on) noexcept { musicOn_ = on; }
    void toggleMusic() noexcept { musicOn_ = !musicOn_; }

    // Linear amplitude (0..1) for a bus: perceptual master x bus trim, 0 when
    // muted (and for Music when music is off).
    [[nodiscard]] float gain(Bus bus) const noexcept;
    [[nodiscard]] float busTrim(Bus bus) const noexcept { return trim_[static_cast<std::size_t>(bus)]; }

private:
    int masterStep_ = kDefaultStep;
    bool muted_ = false;
    bool musicOn_ = true;
    // Ambience and music sit well under the effects so shots stay clear.
    std::array<float, kBusCount> trim_{1.0f, 0.9f, 0.45f, 0.35f};
};

// ---------------------------------------------------------------------------
// Per-play variation
// ---------------------------------------------------------------------------

// Small, deterministic variation of rate and level per play, so repeated
// shots and impacts do not sound copy-pasted. Seeded, so a scripted run
// sounds the same every time.
struct PlayParams {
    float pitch = 1.0f;
    float gain = 1.0f;   // spec gain x jitter (before the mixer)
};

class CueVariation {
public:
    explicit CueVariation(std::uint32_t seed = 0x5EED2026u) noexcept : state_(seed ? seed : 1u) {}
    [[nodiscard]] PlayParams next(const CueSpec& spec) noexcept;

private:
    float unit() noexcept;   // uniform in [-1, 1]
    std::uint32_t state_;
};

} // namespace gallery
