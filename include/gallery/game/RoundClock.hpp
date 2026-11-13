#pragma once

// Round timer (ARCHITECTURE §4.2; brief G-25..G-28, AC-11). Pure logic.
//
//   Countdown (1.5 s) ──► Live (60 s) ──► Ending (1 s) ──► Finished
//   ducks move              shots accepted   ducks stopped,    the app shows
//   shots rejected          low-time ticks   input ignored     the results
//
// Time is accumulated from clamped frame deltas in double precision. A frame
// that straddles a phase boundary is split at the exact instant, so the
// targets stop moving at TimeUp to the nanosecond, at any frame rate.

#include "gallery/core/Time.hpp"

#include <cstdint>

namespace gallery {

struct RoundTuning {
    Seconds countdown = 1.5;          // G-26 "Ready… Go!"; 0 = the round starts Live
    Seconds length = 60.0;            // G-25
    Seconds endDelay = 1.0;           // G-28: ducks stop, results after about 1 s
    int lowTimeThreshold = 10;        // G-27: warning in the final 10 seconds
};

enum class RoundPhase : std::uint8_t { Countdown, Live, Ending, Finished };

[[nodiscard]] const char* roundPhaseName(RoundPhase phase) noexcept;

class RoundClock {
public:
    explicit RoundClock(RoundTuning tuning = {}) noexcept;

    // Back to the start of the countdown (or Live when countdown == 0).
    void reset() noexcept;

    // What one advance did. `moving` is the part of dt during which the
    // targets keep running (Countdown + Live); boundaries crossed are flagged.
    struct Step {
        Seconds moving = 0.0;
        bool go = false;          // Countdown -> Live
        bool timeUp = false;      // Live -> Ending (remaining reached 0)
        bool finished = false;    // Ending -> Finished
    };
    // Non-finite or non-positive dt does nothing. Finished absorbs everything.
    Step advance(Seconds dt) noexcept;

    [[nodiscard]] RoundPhase phase() const noexcept { return phase_; }
    [[nodiscard]] const RoundTuning& tuning() const noexcept { return tuning_; }
    // Seconds spent in the current phase.
    [[nodiscard]] Seconds phaseTime() const noexcept { return phaseTime_; }
    // Live time left: the full length during Countdown, 0 from TimeUp on.
    [[nodiscard]] Seconds remaining() const noexcept;
    // What the timer board shows: whole seconds, rounded UP, so it reads 60
    // at the start of Live and 0 only once time is really up.
    [[nodiscard]] int displaySeconds() const noexcept;
    // Countdown time left (0 outside the countdown).
    [[nodiscard]] Seconds countdownRemaining() const noexcept;
    [[nodiscard]] bool acceptsShots() const noexcept { return phase_ == RoundPhase::Live; }
    [[nodiscard]] bool targetsMoving() const noexcept {
        return phase_ == RoundPhase::Countdown || phase_ == RoundPhase::Live;
    }
    // Final `lowTimeThreshold` seconds of Live (G-27).
    [[nodiscard]] bool lowTime() const noexcept;

private:
    RoundTuning tuning_;
    RoundPhase phase_ = RoundPhase::Countdown;
    Seconds phaseTime_ = 0.0;
};

} // namespace gallery
