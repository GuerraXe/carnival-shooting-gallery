// Movement bounds and movement timing of a single target carriage.

#include "support/test_framework.hpp"

#include "gallery/game/DuckMotion.hpp"

#include <cmath>

using namespace gallery;

namespace {

// Bounds [0, 300], width 100 -> travel 200. v = 100, turn 0.5, dwell 0.25:
//   H = 200/100 + 0.5 = 2.5   E = 0.25 + 2.5 = 2.75   period = 5.5
DuckConfig shuttle() {
    DuckConfig c;
    c.mode = MotionMode::Shuttle;
    c.boundsMin = 0.0f;
    c.boundsMax = 300.0f;
    c.speed = 100.0f;
    c.turnTime = 0.5f;
    c.dwell = 0.25f;
    c.phase = 0.0;
    c.startDirection = +1;
    return c;
}
constexpr float kWidth = 100.0f;

} // namespace

TEST_CASE("motion: shuttle cycle timing follows T/v + turn, plus dwell") {
    const DuckMotion m(shuttle(), kWidth);
    CHECK_NEAR(m.travel(), 200.0, 1e-9);
    CHECK_NEAR(m.halfCycle(), 2.5, 1e-9);
    CHECK_NEAR(m.edgeInterval(), 2.75, 1e-9);
    CHECK_NEAR(m.period(), 5.5, 1e-9);
}

TEST_CASE("motion: shuttle visits both stops and the centre at the predicted times") {
    const DuckMotion m(shuttle(), kWidth);
    CHECK_NEAR(m.sample(0.0).left, 0.0, 1e-4);              // at the left stop, dwelling
    CHECK_NEAR(m.sample(0.25).left, 0.0, 1e-4);             // dwell over, starts moving
    CHECK_NEAR(m.sample(0.25 + 1.25).left, 100.0, 1e-3);    // half-way through the pass: centre
    CHECK_NEAR(m.sample(2.75).left, 200.0, 1e-3);           // arrives at the right stop
    CHECK_NEAR(m.sample(3.0).left, 200.0, 1e-3);            // still dwelling there
    CHECK_NEAR(m.sample(3.0 + 1.25).left, 100.0, 1e-3);     // centre on the way back
    CHECK_NEAR(m.sample(5.5).left, 0.0, 1e-3);              // full period: home again
    CHECK_NEAR(m.sample(5.5 * 7 + 1.5).left, m.sample(1.5).left, 1e-3);   // periodic
}

TEST_CASE("motion: shuttle never leaves its bounds, never exceeds cruise speed") {
    const DuckMotion m(shuttle(), kWidth);
    for (double t = 0.0; t < 3 * m.period(); t += 0.001) {
        const MotionSample s = m.sample(t);
        CHECK(s.left >= -1e-4f);
        CHECK(s.left + kWidth <= 300.0f + 1e-4f);
        CHECK(std::fabs(s.velocity) <= 100.0f + 1e-3f);
    }
}

TEST_CASE("motion: no instant reversal — velocity changes at most v/turn per second") {
    const DuckMotion m(shuttle(), kWidth);
    const double dt = 0.0005;
    const double maxAccel = 100.0 / 0.5;   // v / turnTime
    MotionSample prev = m.sample(0.0);
    for (double t = dt; t < 2 * m.period(); t += dt) {
        const MotionSample s = m.sample(t);
        CHECK(std::fabs(s.velocity - prev.velocity) <= maxAccel * dt + 1e-3);
        // And the position agrees with the velocity (no jumps).
        CHECK(std::fabs(s.left - prev.left) <= 100.0 * dt + 1e-3);
        prev = s;
    }
}

TEST_CASE("motion: shuttle cruises at exactly the configured speed mid-pass") {
    const DuckMotion m(shuttle(), kWidth);
    CHECK_NEAR(m.sample(0.25 + 1.25).velocity, 100.0, 1e-3);
    CHECK_NEAR(m.sample(2.75 + 0.25 + 1.25).velocity, -100.0, 1e-3);
    CHECK_NEAR(m.sample(0.1).velocity, 0.0, 1e-6);   // resting during dwell
}

TEST_CASE("motion: direction and phase set where a carriage starts") {
    DuckConfig c = shuttle();
    c.startDirection = -1;
    const DuckMotion left(c, kWidth);
    const MotionSample s0 = left.sample(left.startClock());
    CHECK_NEAR(s0.left, 200.0, 1e-3);                 // starts at the right stop
    CHECK_EQ(s0.direction, -1);
    CHECK(left.sample(left.startClock() + 1.0).left < 200.0f);   // heading left

    c.startDirection = +1;
    c.phase = 0.5;                                    // half a period later = right stop
    const DuckMotion half(c, kWidth);
    CHECK_NEAR(half.sample(half.startClock()).left, 200.0, 1e-3);
}

TEST_CASE("motion: a turn time longer than the pass is clamped (triangle profile)") {
    DuckConfig c = shuttle();
    c.turnTime = 10.0f;
    const DuckMotion m(c, kWidth);
    CHECK_NEAR(m.halfCycle(), 4.0, 1e-9);   // T/v + T/v
    for (double t = 0.0; t < m.period(); t += 0.01) {
        const MotionSample s = m.sample(t);
        CHECK(s.left >= -1e-4f && s.left <= 200.0f + 1e-3f);
    }
    CHECK_NEAR(m.sample(c.dwell + 4.0).left, 200.0, 1e-3);
}

TEST_CASE("motion: edges are the stop arrivals; timeToEdge counts down to them") {
    const DuckMotion m(shuttle(), kWidth);
    CHECK_NEAR(m.timeToEdge(0.0), 0.0, 1e-9);           // at the left stop now
    CHECK_NEAR(m.timeToEdge(0.1), 2.65, 1e-9);
    CHECK_NEAR(m.timeToEdge(2.75), 0.0, 1e-9);
    CHECK_NEAR(m.timeToEdge(2.76), 2.74, 1e-9);
    CHECK_EQ(m.sample(2.74).edges, std::int64_t{0});
    CHECK_EQ(m.sample(2.75).edges, std::int64_t{1});
    CHECK_EQ(m.sample(5.6).edges, std::int64_t{2});
}

TEST_CASE("motion: wrap mode loops at constant speed and wraps at the bounds") {
    DuckConfig c;
    c.mode = MotionMode::Wrap;
    c.boundsMin = 70.0f;
    c.boundsMax = 1720.0f;
    c.speed = 380.0f;
    const DuckMotion m(c, 128.0f);
    CHECK_NEAR(m.edgeInterval(), 1650.0 / 380.0, 1e-9);
    for (double t = 0.0; t < 20.0; t += 0.003) {
        const MotionSample s = m.sample(t);
        CHECK(s.left >= 70.0f && s.left < 1720.0f + 1e-3f);
        CHECK_NEAR(s.velocity, 380.0, 1e-3);
    }
    CHECK_NEAR(m.sample(1.0).left, 70.0 + 380.0, 1e-3);
    CHECK_NEAR(m.timeToEdge(1.0), 1650.0 / 380.0 - 1.0, 1e-9);

    c.startDirection = -1;
    const DuckMotion back(c, 128.0f);
    CHECK(back.sample(1.0).left < back.sample(0.5).left);
    CHECK_NEAR(back.sample(1.0).velocity, -380.0, 1e-3);
}

TEST_CASE("motion: no travel or no speed means a stationary carriage with no edges") {
    DuckConfig c = shuttle();
    c.boundsMax = 100.0f;   // exactly the width: nowhere to go
    const DuckMotion m(c, kWidth);
    CHECK_NEAR(m.sample(3.0).left, 0.0, 1e-6);
    CHECK_NEAR(m.sample(3.0).velocity, 0.0, 1e-6);
    CHECK(std::isinf(m.timeToEdge(3.0)));
    c = shuttle();
    c.speed = 0.0f;
    CHECK(std::isinf(DuckMotion(c, kWidth).timeToEdge(0.0)));
}

TEST_CASE("motion: rail wobble is tiny, bounded and a function of position only") {
    DuckConfig c = shuttle();
    c.wobbleDeg = 0.5f;
    c.wobblePitch = 64.0f;
    const DuckMotion m(c, kWidth);
    for (float x = 0.0f; x < 300.0f; x += 1.7f) {
        CHECK(std::fabs(m.wobbleAt(x)) <= 0.5f + 1e-6f);
    }
    CHECK_NEAR(m.wobbleAt(16.0f), 0.5, 1e-5);   // quarter pitch: peak
    CHECK_NEAR(m.wobbleAt(10.0f), m.wobbleAt(10.0f + 64.0f), 1e-5);
}
