// Round timer: phases, 60-second length, display rounding, low-time ticks,
// the zero boundary, frame-rate independence, and how the round gates shots.

#include "support/test_framework.hpp"
#include "support/test_round.hpp"

#include "gallery/game/RoundClock.hpp"
#include "gallery/game/Shooting.hpp"

#include <vector>

using namespace gallery;
using gallery::test::centreOf;
using gallery::test::countEvents;
using gallery::test::kOpenBackboard;
using gallery::test::makeTimedRound;

// ---------------------------------------------------------------------------
// RoundClock
// ---------------------------------------------------------------------------

TEST_CASE("clock: starts in the countdown showing 60, shots not accepted") {
    RoundClock c;
    CHECK(c.phase() == RoundPhase::Countdown);
    CHECK_EQ(c.displaySeconds(), 60);
    CHECK_NEAR(c.remaining(), 60.0, 0.0);
    CHECK_NEAR(c.countdownRemaining(), 1.5, 1e-12);
    CHECK(!c.acceptsShots());
    CHECK(c.targetsMoving());   // ducks run during "Ready..." (G-26)
}

TEST_CASE("clock: countdown -> live -> ending -> finished at the brief's times") {
    RoundClock c;
    RoundClock::Step s = c.advance(1.4);
    CHECK(c.phase() == RoundPhase::Countdown);
    CHECK(!s.go);
    s = c.advance(0.2);                       // crosses 1.5 s
    CHECK(s.go);
    CHECK(c.phase() == RoundPhase::Live);
    CHECK_NEAR(c.phaseTime(), 0.1, 1e-12);    // leftover carried into Live
    CHECK(c.acceptsShots());
    s = c.advance(59.9);                      // exactly 60 s of live time
    CHECK(s.timeUp);
    CHECK(c.phase() == RoundPhase::Ending);
    CHECK_EQ(c.displaySeconds(), 0);
    CHECK(!c.acceptsShots());
    CHECK(!c.targetsMoving());                // ducks stop (G-28)
    s = c.advance(0.999);
    CHECK(!s.finished);
    s = c.advance(0.002);
    CHECK(s.finished);
    CHECK(c.phase() == RoundPhase::Finished);
    s = c.advance(10.0);                      // finished absorbs time
    CHECK(!s.go && !s.timeUp && !s.finished);
    CHECK(c.phase() == RoundPhase::Finished);
}

TEST_CASE("clock: 'moving' covers only countdown + live time, split at the boundary") {
    RoundClock c(RoundTuning{.countdown = 1.5, .length = 60.0, .endDelay = 1.0});
    c.advance(61.45);                          // 0.05 s of live time left
    const RoundClock::Step s = c.advance(0.1); // 0.05 live + 0.05 ending
    CHECK(s.timeUp);
    CHECK_NEAR(s.moving, 0.05, 1e-9);
}

TEST_CASE("clock: one huge step crosses every boundary in order") {
    RoundClock c;
    const RoundClock::Step s = c.advance(1000.0);
    CHECK(s.go && s.timeUp && s.finished);
    CHECK_NEAR(s.moving, 61.5, 1e-9);
    CHECK(c.phase() == RoundPhase::Finished);
}

TEST_CASE("clock: the display counts whole seconds, rounding up") {
    RoundClock c(RoundTuning{.countdown = 0.0});
    CHECK(c.phase() == RoundPhase::Live);
    CHECK_EQ(c.displaySeconds(), 60);
    c.advance(0.001);
    CHECK_EQ(c.displaySeconds(), 60);
    c.advance(0.999);
    CHECK_EQ(c.displaySeconds(), 59);
    c.advance(58.5);
    CHECK_EQ(c.displaySeconds(), 1);          // 0.5 s left still shows 1
    CHECK(c.lowTime());
}

TEST_CASE("clock: low-time warning only in the final 10 seconds of live time") {
    RoundClock c(RoundTuning{.countdown = 0.0});
    c.advance(49.9);
    CHECK(!c.lowTime());
    c.advance(0.1);
    CHECK(c.lowTime());
    c.advance(10.0);
    CHECK(!c.lowTime());                      // time up: no longer "low", it is over
}

TEST_CASE("clock: 60 s accumulated from frame deltas ends within one frame of 60 s (AC-11)") {
    for (const double hz : {30.0, 60.0, 144.0, 240.0}) {
        RoundClock c;
        const double dt = 1.0 / hz;
        int frames = 0;
        while (c.phase() != RoundPhase::Ending && frames < 100000) {
            c.advance(dt);
            ++frames;
        }
        const double elapsed = frames * dt;
        CHECK(elapsed >= 61.5 - 1e-6);
        CHECK(elapsed < 61.5 + dt + 1e-6);
    }
}

TEST_CASE("clock: bad deltas are ignored; reset returns to the countdown") {
    RoundClock c;
    c.advance(-1.0);
    c.advance(std::nan(""));
    c.advance(0.0);
    CHECK_NEAR(c.phaseTime(), 0.0, 0.0);
    c.advance(30.0);
    c.reset();
    CHECK(c.phase() == RoundPhase::Countdown);
    CHECK_NEAR(c.phaseTime(), 0.0, 0.0);
}

// ---------------------------------------------------------------------------
// Round + clock
// ---------------------------------------------------------------------------

TEST_CASE("round: clicks during the countdown are not shots and do not arm the rifle") {
    Round r = makeTimedRound();
    r.clearEvents();
    const ShotResult res = r.fire(centreOf(r.lane(), 0));
    CHECK(res.outcome == ShotOutcome::Rejected);
    CHECK(res.reason == RejectReason::NotLive);
    CHECK_EQ(r.stats().shots(), 0);
    CHECK(r.events().empty());
    CHECK(r.rifle().ready(r.time()));
    r.update(1.5);                                         // Go
    CHECK_EQ(countEvents<event::Go>(r.events()), std::size_t{1});
    CHECK(r.fire(kOpenBackboard).outcome == ShotOutcome::Miss);   // first live click fires
}

TEST_CASE("round: a click in the frame that crosses zero counts; after zero nothing does") {
    Round r = makeTimedRound(RoundTuning{.countdown = 0.0});
    r.update(59.99);
    CHECK(r.clock().phase() == RoundPhase::Live);
    // Input is handled before the update that crosses zero (G-28).
    CHECK(r.fire(kOpenBackboard).outcome == ShotOutcome::Miss);
    r.clearEvents();
    r.update(1.0 / 60.0);
    CHECK(r.clock().phase() == RoundPhase::Ending);
    CHECK_EQ(countEvents<event::TimeUp>(r.events()), std::size_t{1});
    CHECK_EQ(r.stats().shots(), 1);
    r.update(0.3);                                         // rifle long since ready
    const ShotResult late = r.fire(kOpenBackboard);
    CHECK(late.outcome == ShotOutcome::Rejected);
    CHECK(late.reason == RejectReason::NotLive);
    CHECK_EQ(r.stats().shots(), 1);
}

TEST_CASE("round: targets stop at the exact instant time runs out") {
    Round a = makeTimedRound(RoundTuning{.countdown = 0.0});
    a.update(60.0);                                        // exactly to zero
    std::vector<float> stopped;
    for (std::size_t i = 0; i < a.lane().size(); ++i) {
        stopped.push_back(a.lane().motion(i).left);
    }
    Round b = makeTimedRound(RoundTuning{.countdown = 0.0});
    b.update(59.95);
    b.update(0.1);                                         // overshoots zero by 50 ms
    b.update(0.5);                                         // ending: frozen
    for (std::size_t i = 0; i < b.lane().size(); ++i) {
        CHECK_NEAR(b.lane().motion(i).left, stopped[i], 1e-3);
    }
}

TEST_CASE("round: low-time ticks 10..1 once each, then TimeUp, then RoundFinished") {
    Round r = makeTimedRound();
    std::vector<int> ticks;
    int timeUps = 0, finishes = 0, gos = 0;
    for (int f = 0; f < 60 * 70; ++f) {
        r.update(1.0 / 60.0);
        for (const GameEvent& e : r.events()) {
            if (const auto* t = std::get_if<event::LowTimeTick>(&e)) {
                ticks.push_back(t->secondsLeft);
                CHECK_EQ(timeUps, 0);
            }
            timeUps += std::holds_alternative<event::TimeUp>(e) ? 1 : 0;
            gos += std::holds_alternative<event::Go>(e) ? 1 : 0;
            if (std::holds_alternative<event::RoundFinished>(e)) {
                CHECK_EQ(timeUps, 1);
                ++finishes;
            }
        }
        r.clearEvents();
    }
    CHECK_EQ(gos, 1);
    CHECK_EQ(timeUps, 1);
    CHECK_EQ(finishes, 1);
    CHECK_EQ(ticks.size(), std::size_t{10});
    for (std::size_t i = 0; i < ticks.size(); ++i) {
        CHECK_EQ(ticks[i], 10 - static_cast<int>(i));
    }
}

TEST_CASE("round: RoundFinished carries the round's actual statistics") {
    Round r = makeTimedRound(RoundTuning{.countdown = 0.0}, ShotTuning{.fireCooldown = 0.0});
    CHECK(r.fire(centreOf(r.lane(), 2)).outcome == ShotOutcome::Hit);
    CHECK(r.fire(kOpenBackboard).outcome == ShotOutcome::Miss);
    CHECK(r.fire(kOpenBackboard).outcome == ShotOutcome::Miss);
    r.clearEvents();
    r.update(61.0);
    const event::RoundFinished* fin = nullptr;
    for (const GameEvent& e : r.events()) {
        if (const auto* f = std::get_if<event::RoundFinished>(&e)) {
            fin = f;
        }
    }
    CHECK(fin != nullptr);
    CHECK_EQ(fin->stats.score(), 10);
    CHECK_EQ(fin->stats.shots(), 3);
    CHECK_EQ(fin->stats.hits(), 1);
    CHECK_EQ(fin->stats.misses(), 2);
    CHECK_EQ(*fin->stats.accuracyPercent(), 33);
}
