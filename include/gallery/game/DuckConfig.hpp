#pragma once

// Duck target configuration (D-027). Pure data: every number that shapes a
// target's movement, timing and hit reaction lives here, so tuning never
// touches the movement code (src/logic/game/DuckMotion.cpp, DuckLane.cpp).
//
// Presets are compiled in (D-012: no runtime config parser) and selected at
// launch (--targets). validateLane() checks a configuration against the
// booth geometry before the game uses it.

#include "gallery/core/Geometry.hpp"
#include "gallery/core/Time.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace gallery {

// How a target's carriage travels along the rail.
enum class MotionMode : std::uint8_t {
    // Reciprocating carriage on its own section of the rail: constant cruise
    // speed, eased stop-and-reverse at each end stop, optional dwell at the
    // stop. The silhouette never leaves [boundsMin, boundsMax].
    Shuttle,
    // Continuous chain loop (brief G-14): the LEFT EDGE runs over
    // [boundsMin, boundsMax) and wraps; the wrap points must lie behind the
    // booth posts so the jump is never seen.
    Wrap,
};

// What a hit target does (brief G-17 / G-18).
enum class HitReaction : std::uint8_t {
    FlipBack,   // folds backward on its base hinge, clatters flat behind the wave cutout
    TiltDrop,   // tips over sideways and drops below the track, out of the lane
};

// When a knocked-down target comes back up.
enum class RiseTrigger : std::uint8_t {
    AfterDownTime,   // as soon as minDownTime has passed
    AtLaneEnd,       // at the first end stop (Shuttle) or wrap (Wrap) reached
                     // after minDownTime has passed (brief G-18, AC-8)
};

struct DuckConfig {
    // ---- movement --------------------------------------------------------
    MotionMode mode = MotionMode::Shuttle;
    float boundsMin = 0.0f;       // logical x; meaning depends on mode (see MotionMode)
    float boundsMax = 0.0f;
    float trackY = 600.0f;        // logical y of the hinge line (bottom of the silhouette)
    float speed = 120.0f;         // cruise speed, logical units per second
    float turnTime = 0.3f;        // Shuttle: seconds to brake to rest (and to accelerate back)
    float dwell = 0.15f;          // Shuttle: seconds resting at each end stop
    double phase = 0.0;           // [0,1): start offset as a fraction of the motion cycle
    int startDirection = +1;      // +1 = toward boundsMax (right), -1 = toward boundsMin

    // Rail rumble: the cutout rocks very slightly as its wheels cross rail
    // joints. A function of position, so it stops when the carriage stops.
    float wobbleDeg = 0.5f;
    float wobblePitch = 64.0f;    // logical units between rail joints

    // ---- hit reaction ------------------------------------------------------
    HitReaction reaction = HitReaction::FlipBack;
    RiseTrigger rise = RiseTrigger::AtLaneEnd;
    Seconds fallTime = 0.20;      // brief G-17: 150-250 ms
    Seconds riseTime = 0.25;
    Seconds minDownTime = 2.0;    // brief G-18
    float dropDepth = 118.0f;     // TiltDrop: how far below the track it sinks (logical units)

    // ---- look --------------------------------------------------------------
    std::uint8_t variant = 0;     // paint variant: 0 = TGT_DUCK_A, 1 = B, 2 = C
};

// What a target is worth when hit (D-035). A target keeps its kind while it
// is up; a new kind is drawn each time it rises again (and at round start).
enum class TargetKind : std::uint8_t {
    Duck,     // ordinary target: scores
    Decoy,    // "don't shoot": hitting it costs points
    Golden,   // power-up: scores and gives free ammo for a while
};

[[nodiscard]] const char* targetKindName(TargetKind k) noexcept;

// How often each kind turns up. All zero = every target is a Duck (the
// classic game; the default, so plain configurations behave as before).
struct TargetMix {
    float decoyChance = 0.0f;    // per draw
    float goldenChance = 0.0f;   // per draw; never at round start
    int maxDecoys = 2;           // decoys on the lane at once (so ducks always outnumber them)
    int maxGolden = 1;
    // Difficulty for ordinary ducks (D-036). 1 / 0 = classic behaviour.
    float duckSpeed = 1.0f;      // carriage speed multiplier while the target is a Duck
    Seconds upTime = 0.0;        // Ducks and Golden drop on their own after about this long
                                 // upright and unhit (0 = never); 60..140 % per rise. Decoys: decoyUpTime.
    Seconds decoyUpTime = 0.0;   // the same for decoys (D-037; 0 = they stay up)
    Seconds goldenEvery = 0.0;   // guarantee: if no golden target has come up for this long,
                                 // the next rise is golden (D-037; 0 = chance only)
    // Pacing of the up-time windows (D-038). 0 = off (classic / older tests).
    Seconds goldenUpTime = 0.0;  // golden targets' own window base (0 = use upTime)
    float upTimeRamp = 0.0f;     // Ducks/Golden: windows shrink by this fraction per minute of
                                 // lane time, at most 40 % (the round gets harder)
    Seconds minUpTime = 0.0;     // floor: never less than this upright (stays fair)
    Seconds dropGap = 0.0;       // no two targets scheduled to leave within this of each other
    std::uint32_t seed = 0x0DDC0DE5u;   // deterministic sequence per session
};

// The game's mix: about one target in four is a decoy, one rise in twelve a
// golden duck.
[[nodiscard]] TargetMix standardMix() noexcept;

struct LaneConfig {
    std::vector<DuckConfig> ducks;   // draw order: later entries are in front
    TargetMix mix;                   // default: all Ducks
};

enum class LanePreset : std::uint8_t {
    Sections,   // five independent shuttle carriages, one rail section each (default, D-027)
    Chain,      // the brief's single chain loop: equal speed, even spacing, wrap behind posts
};

[[nodiscard]] std::string_view lanePresetName(LanePreset p) noexcept;

// Built from the booth layout (gallery::layout).
[[nodiscard]] LaneConfig makeLaneConfig(LanePreset preset);

// Apply the same hit reaction to every target (launch switch --reaction).
void setReaction(LaneConfig& config, HitReaction reaction) noexcept;

// Problems with `config`, one human-readable line each; empty = valid.
//   duckWidths[variant] = logical width of that variant's silhouette.
//   visible = the visible gallery opening (layout::kOpening).
[[nodiscard]] std::vector<std::string> validateLane(const LaneConfig& config,
                                                    const std::vector<float>& duckWidths,
                                                    float duckHeight, Rect visible);

} // namespace gallery
