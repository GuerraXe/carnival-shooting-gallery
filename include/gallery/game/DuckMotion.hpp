#pragma once

// Horizontal motion of one target carriage, as a pure function of its own
// motion clock (seconds). The lane advances each clock by the frame delta, so
// motion is frame-rate independent by construction: position depends only on
// the summed time, never on how it was sliced (D-007, ARCHITECTURE §5.2).
//
// Shuttle profile, one half-cycle from the left stop to the right stop
// (travel distance T = boundsMax - boundsMin - width, cruise speed v,
// turn time a):
//
//   speed  v ┤      ____________________
//            │     /                    \          constant acceleration
//            │    /                      \         over `a` seconds at each
//          0 ┼───┘                        └───     end (no instant reversal)
//              dwell  a     cruise      a   dwell
//
//   half-cycle H = T/v + a     edge interval E = dwell + H     period = 2E
//
// "Edges" are the moments the carriage arrives at an end stop (Shuttle) or
// wraps behind a post (Wrap). Knocked-down targets rise at an edge.

#include "gallery/core/Time.hpp"
#include "gallery/game/DuckConfig.hpp"

#include <cstdint>

namespace gallery {

struct MotionSample {
    float left = 0.0f;          // logical x of the silhouette's left edge
    float velocity = 0.0f;      // logical units / s (signed; + = rightward)
    int direction = +1;         // travel direction, or the next one while at rest
    std::int64_t edges = 0;     // edges passed since clock 0
};

class DuckMotion {
public:
    DuckMotion() = default;
    DuckMotion(const DuckConfig& config, float width) noexcept;

    // Clock value at which this target starts (from phase and startDirection).
    [[nodiscard]] double startClock() const noexcept { return start_; }

    [[nodiscard]] MotionSample sample(double clock) const noexcept;

    // Seconds from `clock` to the first edge at or after it. Infinity when the
    // carriage cannot move (zero travel or zero speed).
    [[nodiscard]] double timeToEdge(double clock) const noexcept;

    // Shuttle: travel distance T; Wrap: loop length.
    [[nodiscard]] double travel() const noexcept { return travel_; }
    [[nodiscard]] double halfCycle() const noexcept { return half_; }      // Shuttle: H
    [[nodiscard]] double edgeInterval() const noexcept { return edge_; }   // Shuttle: E; Wrap: lap time
    [[nodiscard]] double period() const noexcept { return period_; }       // full cycle

    // Rail-rumble tilt in degrees at a given left-edge position.
    [[nodiscard]] float wobbleAt(float left) const noexcept;

private:
    // Distance travelled `u` seconds into a half-cycle, and the speed there.
    [[nodiscard]] double halfDistance(double u) const noexcept;
    [[nodiscard]] double halfSpeed(double u) const noexcept;

    MotionMode mode_ = MotionMode::Shuttle;
    double min_ = 0.0;
    double travel_ = 0.0;
    double speed_ = 0.0;
    double turn_ = 0.0;       // effective turn time (≤ T/v so the profile always fits)
    double dwell_ = 0.0;
    double half_ = 0.0;
    double edge_ = 0.0;
    double period_ = 0.0;
    double start_ = 0.0;
    int wrapDir_ = +1;
    float wobbleDeg_ = 0.0f;
    float wobblePitch_ = 64.0f;
};

} // namespace gallery
