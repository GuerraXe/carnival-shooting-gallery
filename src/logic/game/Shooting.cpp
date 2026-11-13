#include "gallery/game/Shooting.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace gallery {

namespace {
// Accumulated frame times are not exact; a click at "250 ms" summed from
// frame deltas must count as 250 ms, not 249.9999999.
constexpr Seconds kTimeEps = 1e-9;
} // namespace

// ---------------------------------------------------------------------------
// Rifle
// ---------------------------------------------------------------------------

Rifle::Rifle(Seconds cooldown) noexcept
    : cooldown_(std::isfinite(cooldown) && cooldown > 0.0 ? cooldown : 0.0) {}

bool Rifle::ready(Seconds now) const noexcept {
    return !last_ || now - *last_ >= cooldown_ - kTimeEps;
}

Seconds Rifle::remaining(Seconds now) const noexcept {
    return ready(now) ? 0.0 : cooldown_ - (now - *last_);
}

bool Rifle::tryFire(Seconds now) noexcept {
    if (!ready(now)) {
        return false;
    }
    last_ = now;
    return true;
}

void Rifle::reset() noexcept {
    last_.reset();
}

// ---------------------------------------------------------------------------
// RoundStats
// ---------------------------------------------------------------------------

void RoundStats::recordHit(int points) noexcept {
    ++hits_;
    score_ += points;
}

void RoundStats::recordMiss() noexcept {
    ++misses_;
}

void RoundStats::recordDecoy(int penalty) noexcept {
    ++decoys_;
    score_ -= penalty;   // the score may go below zero
}

std::optional<int> RoundStats::accuracyPercent() const noexcept {
    const int n = shots();
    if (n <= 0) {
        return std::nullopt;
    }
    // Integer half-up rounding of hits*100/n: no floating-point ties.
    return (hits_ * 200 + n) / (2 * n);
}

// ---------------------------------------------------------------------------
// Round
// ---------------------------------------------------------------------------

Round::Round(DuckLane lane, ShotTuning shot, RoundTuning timing, Occluder foreground)
    : lane_(std::move(lane)),
      tuning_(shot),
      foreground_(std::move(foreground)),
      rifle_(shot.fireCooldown),
      clock_(timing) {
    tuning_.magazine = std::max(0, tuning_.magazine);
    ammo_ = tuning_.magazine;
    // Worst case per frame: a burst of clicks (one accepted per cooldown, so a
    // handful) plus every clock boundary. Reserved once; never grows in play.
    events_.reserve(32);
}

ShotResult Round::fire(Vec2 aim) {
    // Order matters: outside the live phase a click is not a shot at all and
    // must not touch the rifle (G-11, G-26, G-28, AC-16).
    if (!clock_.acceptsShots()) {
        return {ShotOutcome::Rejected, std::nullopt, RejectReason::NotLive};
    }
    if (!std::isfinite(aim.x) || !std::isfinite(aim.y)) {
        return {ShotOutcome::Rejected, std::nullopt, RejectReason::BadAim};
    }
    if (reloading()) {
        return {ShotOutcome::Rejected, std::nullopt, RejectReason::Reloading};
    }
    if (!rifle_.tryFire(time_)) {
        return {ShotOutcome::Rejected, std::nullopt, RejectReason::Cooldown};   // G-8
    }
    const bool freeAmmo = freeAmmoLeft_ > 0.0;
    if (tuning_.magazine > 0 && !freeAmmo) {
        if (ammo_ <= 0) {
            // The hammer falls on an empty chamber: a click, not a shot. It
            // goes through the rifle cooldown so clicking cannot spam it.
            events_.push_back(event::DryFire{aim});
            return {ShotOutcome::Rejected, std::nullopt, RejectReason::Empty};
        }
        --ammo_;
    }
    events_.push_back(event::ShotFired{aim});

    // Only an upright (active), visible target is hittable (G-16, AC-6, AC-7).
    if (const auto id = targetUnder(aim); id && lane_.knockDown(*id)) {
        const TargetKind kind = lane_.kind(*id);
        if (kind == TargetKind::Decoy) {
            stats_.recordDecoy(tuning_.decoyPenalty);
            events_.push_back(event::DuckHit{*id, aim, -tuning_.decoyPenalty, kind});
            return {ShotOutcome::Decoy, id, RejectReason::None};
        }
        stats_.recordHit(tuning_.pointsPerHit);
        events_.push_back(event::DuckHit{*id, aim, tuning_.pointsPerHit, kind});
        if (kind == TargetKind::Golden && tuning_.powerUpTime > 0.0) {
            freeAmmoLeft_ = tuning_.powerUpTime;
            ammo_ = tuning_.magazine;   // a full magazine waiting when it runs out
            reloadLeft_ = 0.0;
            events_.push_back(event::PowerUp{tuning_.powerUpTime});
        }
        return {ShotOutcome::Hit, id, RejectReason::None};
    }
    stats_.recordMiss();
    events_.push_back(event::ShotMissed{aim});
    return {ShotOutcome::Miss, std::nullopt, RejectReason::None};
}

bool Round::reload() {
    if (tuning_.magazine <= 0 || !clock_.targetsMoving() || reloading() || freeAmmoLeft_ > 0.0 ||
        ammo_ >= tuning_.magazine) {
        return false;
    }
    reloadLeft_ = std::max(tuning_.reloadTime, 1e-3);
    events_.push_back(event::ReloadStarted{});
    return true;
}

float Round::reloadProgress() const noexcept {
    if (!reloading()) {
        return 0.0f;
    }
    const Seconds total = std::max(tuning_.reloadTime, 1e-3);
    return static_cast<float>(std::clamp(1.0 - reloadLeft_ / total, 0.0, 1.0));
}

std::optional<std::size_t> Round::targetUnder(Vec2 aim) const noexcept {
    // Scenery in front of the lane (wave crests, booth posts) stops the
    // pellet before it reaches whatever is behind it.
    if (foreground_.covers(aim)) {
        return std::nullopt;
    }
    return lane_.hitTest(aim);
}

void Round::update(Seconds dt) {
    if (!(dt > 0.0) || !std::isfinite(dt)) {
        return;
    }
    const Seconds before = clock_.remaining();
    const bool wasLive = clock_.phase() == RoundPhase::Live;
    const RoundClock::Step step = clock_.advance(dt);
    time_ += dt;
    // Targets run only for the part of the frame before time ran out.
    if (step.moving > 0.0) {
        lane_.update(step.moving);
    }
    // Reload and power-up run on the same time as the targets: frozen by a
    // pause, stopped at time up.
    if (step.moving > 0.0 && reloadLeft_ > 0.0) {
        reloadLeft_ -= step.moving;
        if (reloadLeft_ <= kTimeEps) {
            reloadLeft_ = 0.0;
            ammo_ = tuning_.magazine;
            events_.push_back(event::Reloaded{});
        }
    }
    if (step.moving > 0.0 && freeAmmoLeft_ > 0.0) {
        freeAmmoLeft_ -= step.moving;
        if (freeAmmoLeft_ <= kTimeEps) {
            freeAmmoLeft_ = 0.0;
            events_.push_back(event::PowerUpEnded{});
        }
    }

    if (step.go) {
        events_.push_back(event::Go{});
    }
    // Low-time ticks at remaining = N for N = threshold .. 1 (G-27). Only for
    // live time actually crossed in this frame.
    if (wasLive || step.go) {
        const Seconds after = clock_.remaining();
        for (int n = clock_.tuning().lowTimeThreshold; n >= 1; --n) {
            const auto at = static_cast<Seconds>(n);
            if (before > at + kTimeEps && after <= at + kTimeEps) {
                events_.push_back(event::LowTimeTick{n});
            }
        }
    }
    if (step.timeUp) {
        events_.push_back(event::TimeUp{});
    }
    if (step.finished) {
        events_.push_back(event::RoundFinished{stats_});
    }
}

void Round::reset() {
    lane_.reset();
    rifle_.reset();
    stats_.reset();
    clock_.reset();
    time_ = 0.0;
    ammo_ = tuning_.magazine;
    reloadLeft_ = 0.0;
    freeAmmoLeft_ = 0.0;
    events_.clear();
    events_.push_back(event::RoundReset{});
    if (clock_.phase() == RoundPhase::Live) {
        events_.push_back(event::Go{});   // no countdown configured
    }
}

} // namespace gallery
