#include "gallery/game/RoundClock.hpp"

#include <algorithm>
#include <cmath>

namespace gallery {

namespace {
// Frame deltas summed over a minute are not exact: 3600 x (1/60) is not 60.0.
// A boundary closer than this counts as reached.
constexpr Seconds kTimeEps = 1e-9;

Seconds sanitize(Seconds s) noexcept {
    return std::isfinite(s) && s > 0.0 ? s : 0.0;
}
} // namespace

const char* roundPhaseName(RoundPhase phase) noexcept {
    switch (phase) {
    case RoundPhase::Countdown: return "Countdown";
    case RoundPhase::Live: return "Live";
    case RoundPhase::Ending: return "Ending";
    case RoundPhase::Finished: return "Finished";
    }
    return "?";
}

RoundClock::RoundClock(RoundTuning tuning) noexcept
    : tuning_{sanitize(tuning.countdown), sanitize(tuning.length), sanitize(tuning.endDelay),
              std::max(0, tuning.lowTimeThreshold)} {
    reset();
}

void RoundClock::reset() noexcept {
    phase_ = tuning_.countdown > 0.0 ? RoundPhase::Countdown : RoundPhase::Live;
    phaseTime_ = 0.0;
}

RoundClock::Step RoundClock::advance(Seconds dt) noexcept {
    Step step;
    if (!(dt > 0.0) || !std::isfinite(dt)) {
        return step;
    }
    Seconds left = dt;
    // Walk across as many boundaries as dt covers (a huge dt in a test can
    // cross all three); each phase consumes only up to its own end.
    while (left > 0.0 && phase_ != RoundPhase::Finished) {
        Seconds span = 0.0;
        switch (phase_) {
        case RoundPhase::Countdown: span = tuning_.countdown; break;
        case RoundPhase::Live: span = tuning_.length; break;
        case RoundPhase::Ending: span = tuning_.endDelay; break;
        case RoundPhase::Finished: break;
        }
        const Seconds use = std::min(left, std::max(0.0, span - phaseTime_));
        phaseTime_ += use;
        left -= use;
        if (targetsMoving()) {
            step.moving += use;
        }
        if (phaseTime_ < span - kTimeEps) {
            break;   // still inside this phase: dt is used up
        }
        // Boundary reached: carry the leftover into the next phase.
        phaseTime_ = 0.0;
        switch (phase_) {
        case RoundPhase::Countdown:
            phase_ = RoundPhase::Live;
            step.go = true;
            break;
        case RoundPhase::Live:
            phase_ = RoundPhase::Ending;
            step.timeUp = true;
            break;
        case RoundPhase::Ending:
            phase_ = RoundPhase::Finished;
            step.finished = true;
            break;
        case RoundPhase::Finished: break;
        }
    }
    return step;
}

Seconds RoundClock::remaining() const noexcept {
    switch (phase_) {
    case RoundPhase::Countdown: return tuning_.length;
    case RoundPhase::Live: return std::max(0.0, tuning_.length - phaseTime_);
    case RoundPhase::Ending:
    case RoundPhase::Finished: return 0.0;
    }
    return 0.0;
}

int RoundClock::displaySeconds() const noexcept {
    // Ceil with a tolerance so 59.9999999999 (summed frames) still reads 60.
    return static_cast<int>(std::ceil(remaining() - kTimeEps * 1000.0));
}

Seconds RoundClock::countdownRemaining() const noexcept {
    return phase_ == RoundPhase::Countdown ? std::max(0.0, tuning_.countdown - phaseTime_) : 0.0;
}

bool RoundClock::lowTime() const noexcept {
    return phase_ == RoundPhase::Live &&
           remaining() <= static_cast<Seconds>(tuning_.lowTimeThreshold) + kTimeEps;
}

} // namespace gallery
