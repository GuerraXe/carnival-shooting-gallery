#pragma once

// Shooting rules (ARCHITECTURE §2.2, §4.4; brief G-7..G-10, G-16, G-20..G-23).
// Pure logic, no SFML: the shell feeds logical aim points in and reads
// results and events out. Hitscan only: a shot resolves on the frame it is
// received, against the targets as they were last drawn (D-008). No projectile.

#include "gallery/core/Geometry.hpp"
#include "gallery/core/Time.hpp"
#include "gallery/game/DuckLane.hpp"
#include "gallery/game/Occluder.hpp"
#include "gallery/game/RoundClock.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace gallery {

struct ShotTuning {
    Seconds fireCooldown = 0.25;   // brief G-8: minimum interval between accepted shots
    int pointsPerHit = 10;         // brief G-20
    // D-035 (owner request). The defaults keep the classic rules (no
    // magazine); the game itself uses standard() below.
    int magazine = 0;              // rounds per load; 0 = unlimited, no reloading
    Seconds reloadTime = 1.2;      // R / right-click: magazine refilled after this long
    int decoyPenalty = 20;         // points lost for hitting a decoy
    Seconds powerUpTime = 5.0;     // golden target: free ammo (no rounds used, no reload) this long (D-036: was 8)

    // The rules the game ships with: a 6-round magazine.
    [[nodiscard]] static ShotTuning standard() noexcept {
        ShotTuning t;
        t.magazine = 6;
        return t;
    }
};

// Fire-rate control. A click during the cooldown is ignored and is NOT a shot (G-8, AC-4).
class Rifle {
public:
    explicit Rifle(Seconds cooldown = 0.25) noexcept;

    // True (and the cooldown restarts) if a shot may be fired at `now`.
    bool tryFire(Seconds now) noexcept;
    [[nodiscard]] bool ready(Seconds now) const noexcept;
    [[nodiscard]] Seconds remaining(Seconds now) const noexcept;   // 0 when ready
    void reset() noexcept;

private:
    Seconds cooldown_;
    std::optional<Seconds> last_;
};

// Shots fired (accepted only), hits, misses, score.
class RoundStats {
public:
    void recordHit(int points) noexcept;
    void recordMiss() noexcept;
    void recordDecoy(int penalty) noexcept;   // a shot that hit a decoy: a shot, not a hit
    void reset() noexcept { *this = RoundStats{}; }

    [[nodiscard]] int shots() const noexcept { return hits_ + misses_ + decoys_; }
    [[nodiscard]] int hits() const noexcept { return hits_; }
    [[nodiscard]] int misses() const noexcept { return misses_; }
    [[nodiscard]] int decoys() const noexcept { return decoys_; }
    [[nodiscard]] int score() const noexcept { return score_; }
    // round(hits / shots × 100), half up; nullopt when no shots (shown as "—", G-23).
    [[nodiscard]] std::optional<int> accuracyPercent() const noexcept;

private:
    int hits_ = 0;
    int misses_ = 0;
    int decoys_ = 0;
    int score_ = 0;
};

// Per-frame events for presentation (D-010). Read by the renderer (and later
// audio), then cleared by the owner. Order within a frame is the order in
// which things happened: shots (input) first, then clock boundaries (update).
namespace event {
struct RoundReset {};                                          // fresh round set up (clear effects)
struct Go {};                                                  // countdown over: shots accepted
struct ShotFired { Vec2 aim; };                                // every accepted shot
struct DuckHit {                                               // after ShotFired
    std::size_t id;
    Vec2 aim;
    int points;                                                // negative for a decoy
    TargetKind kind = TargetKind::Duck;
};
struct DryFire { Vec2 aim; };                                  // trigger pulled on an empty magazine
struct ReloadStarted {};
struct Reloaded {};                                            // magazine full again
struct PowerUp { Seconds duration; };                          // golden target hit: free ammo
struct PowerUpEnded {};
struct ShotMissed { Vec2 aim; };                               // after ShotFired
struct LowTimeTick { int secondsLeft; };                       // 10, 9, ... 1 (G-27)
struct TimeUp {};                                              // remaining reached 0; ducks stop
struct RoundFinished { RoundStats stats; };                    // end delay over: show results
} // namespace event
using GameEvent = std::variant<event::RoundReset, event::Go, event::ShotFired, event::DuckHit,
                               event::ShotMissed, event::LowTimeTick, event::TimeUp,
                               event::RoundFinished, event::DryFire, event::ReloadStarted,
                               event::Reloaded, event::PowerUp, event::PowerUpEnded>;

enum class ShotOutcome : std::uint8_t {
    Rejected,      // not a shot: rifle cooling down, round not live, or bad aim
    Hit,
    Miss,
    Decoy          // a shot that hit a decoy (D-035)
};

enum class RejectReason : std::uint8_t { None, Cooldown, NotLive, BadAim, Empty, Reloading };

struct ShotResult {
    ShotOutcome outcome = ShotOutcome::Rejected;
    std::optional<std::size_t> duck;          // set on Hit
    RejectReason reason = RejectReason::None; // set on Rejected
};

// One round: targets + rifle + stats + clock. The only mutator API for
// gameplay (ARCHITECTURE §2.2, §4.2, §4.4).
class Round {
public:
    // `foreground` is what is drawn in front of the targets (wave cutout,
    // booth posts, counter): a shot that lands on it hits the scenery.
    Round(DuckLane lane, ShotTuning shot = {}, RoundTuning timing = {}, Occluder foreground = {});

    // The target a shot at `aim` would hit right now: the front-most upright
    // target whose silhouette is under the aim AND not hidden behind the
    // foreground (G-16, AC-6, AC-7). Used by fire() and the F3 overlay.
    [[nodiscard]] std::optional<std::size_t> targetUnder(Vec2 aim) const noexcept;

    // Resolve a shot at a logical aim point (already clamped to the canvas by
    // the caller, D-015). Call BEFORE update() in a frame (D-008), so a click
    // in the frame whose update crosses zero still counts (G-28).
    ShotResult fire(Vec2 aim);

    // Advance the clock; the targets move only during Countdown and Live and
    // stop at the exact instant time runs out.
    void update(Seconds dt);

    // Start reloading (R / right-click). False when there is nothing to do:
    // no magazine, already full or reloading, free ammo running, or the round
    // is not running. Not a shot.
    bool reload();

    // A clean new round: targets to their start, rifle ready, stats zero,
    // clock back to the countdown, pending events dropped. Emits RoundReset.
    // Plain data in place: no allocation, no loading (G-32, AC-13).
    void reset();

    // Debug hit-reaction preview (K): knocks a target down WITHOUT a shot.
    bool debugKnockDown(std::size_t id) { return lane_.knockDown(id); }

    [[nodiscard]] const DuckLane& lane() const noexcept { return lane_; }
    [[nodiscard]] DuckLane& lane() noexcept { return lane_; }
    [[nodiscard]] const RoundStats& stats() const noexcept { return stats_; }
    [[nodiscard]] const Rifle& rifle() const noexcept { return rifle_; }
    [[nodiscard]] const RoundClock& clock() const noexcept { return clock_; }
    [[nodiscard]] const ShotTuning& tuning() const noexcept { return tuning_; }
    [[nodiscard]] const Occluder& foreground() const noexcept { return foreground_; }
    [[nodiscard]] Seconds time() const noexcept { return time_; }   // round-local time
    // Ammunition (D-035). With magazine 0 the rifle never runs dry.
    [[nodiscard]] int ammo() const noexcept { return ammo_; }
    [[nodiscard]] bool reloading() const noexcept { return reloadLeft_ > 0.0; }
    [[nodiscard]] float reloadProgress() const noexcept;    // 0..1 while reloading, else 0
    [[nodiscard]] Seconds freeAmmoLeft() const noexcept { return freeAmmoLeft_; }

    [[nodiscard]] const std::vector<GameEvent>& events() const noexcept { return events_; }
    void clearEvents() noexcept { events_.clear(); }

private:
    DuckLane lane_;
    ShotTuning tuning_;
    Occluder foreground_;
    Rifle rifle_;
    RoundStats stats_;
    RoundClock clock_;
    Seconds time_ = 0.0;
    int ammo_ = 0;
    Seconds reloadLeft_ = 0.0;
    Seconds freeAmmoLeft_ = 0.0;
    std::vector<GameEvent> events_;
};

} // namespace gallery
