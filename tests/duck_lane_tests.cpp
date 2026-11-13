// Duck targets: configuration, bounds, frame-rate independence, state
// transitions, re-entry, poses and hit regions.

#include "support/test_framework.hpp"

#include "gallery/game/DuckLane.hpp"
#include "gallery/view/SceneLayout.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

using namespace gallery;
namespace L = gallery::layout;

namespace {

constexpr float kStandInWidth = L::kDuckHeight * L::kDuckStandInAspect;

DuckShape box(float w = kStandInWidth, float h = L::kDuckHeight) {
    return DuckShape{{w, h}, {w, h}, {}};
}

std::vector<DuckShape> boxes(float w = kStandInWidth) {
    return {box(w), box(w), box(w)};
}

// 20x20 image: solid disc of radius 8 in the middle, transparent corners.
DuckShape disc(float w, float h) {
    std::vector<std::uint8_t> px(20 * 20 * 4, 255);
    for (int y = 0; y < 20; ++y) {
        for (int x = 0; x < 20; ++x) {
            const float dx = static_cast<float>(x) + 0.5f - 10.0f;
            const float dy = static_cast<float>(y) + 0.5f - 10.0f;
            px[static_cast<std::size_t>((y * 20 + x) * 4 + 3)] = dx * dx + dy * dy <= 64.0f ? 255 : 0;
        }
    }
    return DuckShape{{w, h}, {20.0f, 20.0f}, AlphaMask::fromRgba(px, 20, 20)};
}

float minX(const Quad& q) {
    return std::min({q.p[0].x, q.p[1].x, q.p[2].x, q.p[3].x});
}
float maxX(const Quad& q) {
    return std::max({q.p[0].x, q.p[1].x, q.p[2].x, q.p[3].x});
}
float minY(const Quad& q) {
    return std::min({q.p[0].y, q.p[1].y, q.p[2].y, q.p[3].y});
}
float maxY(const Quad& q) {
    return std::max({q.p[0].y, q.p[1].y, q.p[2].y, q.p[3].y});
}

// Deterministic jittery frame times in [2 ms, 100 ms] (the clamp ceiling).
struct Jitter {
    std::uint32_t s = 12345;
    double operator()() {
        s = s * 1664525u + 1013904223u;
        return 0.002 + 0.098 * static_cast<double>(s >> 8) / static_cast<double>(1u << 24);
    }
};

// Step the lane to exactly `target` using frames from `nextDt` (the last
// frame is shortened so scripted events land on the same instant at every
// frame rate).
void runTo(DuckLane& lane, double target, const std::function<double()>& nextDt) {
    while (lane.time() < target - 1e-12) {
        lane.update(std::min(nextDt(), target - lane.time()));
    }
}

struct Snapshot {
    std::vector<float> left;
    std::vector<DuckState> state;
};

Snapshot snap(const DuckLane& lane) {
    Snapshot s;
    for (std::size_t i = 0; i < lane.size(); ++i) {
        s.left.push_back(lane.motion(i).left);
        s.state.push_back(lane.state(i));
    }
    return s;
}

// A scripted 40 s session: knockdowns at fixed times, snapshots at checkpoints.
struct Session {
    std::vector<Snapshot> checkpoints;
    std::vector<DuckTransition> transitions;
};

Session play(LaneConfig cfg, const std::function<double()>& nextDt) {
    DuckLane lane(std::move(cfg), boxes());
    const std::vector<std::pair<double, std::size_t>> knocks{
        {1.0, 2}, {1.0, 4}, {5.0, 0}, {7.25, 2}, {12.5, 1}, {20.0, 3}, {20.1, 4}, {31.0, 0}};
    const std::vector<double> checks{0.5, 3.3, 9.9, 15.0, 22.2, 33.3, 40.0};
    std::vector<std::pair<double, int>> events;   // -1 = checkpoint, else knock index
    for (std::size_t k = 0; k < knocks.size(); ++k) {
        events.emplace_back(knocks[k].first, static_cast<int>(k));
    }
    for (const double c : checks) {
        events.emplace_back(c, -1);
    }
    std::sort(events.begin(), events.end());
    Session s;
    for (const auto& [t, what] : events) {
        runTo(lane, t, nextDt);
        if (what < 0) {
            s.checkpoints.push_back(snap(lane));
        } else {
            lane.knockDown(knocks[static_cast<std::size_t>(what)].second);
        }
    }
    s.transitions = lane.transitions();
    return s;
}

void checkSameSession(const Session& a, const Session& b) {
    CHECK_EQ(a.checkpoints.size(), b.checkpoints.size());
    for (std::size_t c = 0; c < a.checkpoints.size(); ++c) {
        for (std::size_t i = 0; i < a.checkpoints[c].left.size(); ++i) {
            CHECK_NEAR(a.checkpoints[c].left[i], b.checkpoints[c].left[i], 1e-3);
            CHECK(a.checkpoints[c].state[i] == b.checkpoints[c].state[i]);
        }
    }
    CHECK_EQ(a.transitions.size(), b.transitions.size());
    for (std::size_t k = 0; k < a.transitions.size(); ++k) {
        CHECK_EQ(a.transitions[k].id, b.transitions[k].id);
        CHECK(a.transitions[k].to == b.transitions[k].to);
        CHECK_NEAR(a.transitions[k].at, b.transitions[k].at, 1e-6);
    }
}

DuckConfig single(MotionMode mode = MotionMode::Shuttle) {
    DuckConfig c;
    c.mode = mode;
    c.boundsMin = 400.0f;
    c.boundsMax = 800.0f;
    c.trackY = L::kTrackY;
    c.speed = 100.0f;
    c.turnTime = 0.3f;
    c.dwell = 0.2f;
    c.wobbleDeg = 0.0f;
    c.fallTime = 0.2;
    c.riseTime = 0.25;
    c.minDownTime = 2.0;
    return c;
}

} // namespace

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

TEST_CASE("targets: both presets have exactly five targets and validate cleanly") {
    for (const LanePreset p : {LanePreset::Sections, LanePreset::Chain}) {
        const LaneConfig cfg = makeLaneConfig(p);
        CHECK_EQ(cfg.ducks.size(), std::size_t{5});
        const std::vector<float> widths(3, kStandInWidth);
        const auto issues = validateLane(cfg, widths, L::kDuckHeight, L::kOpening);
        for (const auto& i : issues) {
            std::cout << "    issue (" << lanePresetName(p) << "): " << i << "\n";
        }
        CHECK(issues.empty());
    }
}

TEST_CASE("targets: sections preset still validates for wider photographed ducks") {
    // A real photo may be wider than the stand-in card (aspect up to ~2:1).
    const std::vector<float> widths{L::kDuckHeight * 1.6f, L::kDuckHeight * 1.9f,
                                    L::kDuckHeight * 2.0f};
    CHECK(validateLane(makeLaneConfig(LanePreset::Sections), widths, L::kDuckHeight, L::kOpening)
              .empty());
}

TEST_CASE("targets: sections preset varies speed and timing modestly, never in lockstep") {
    const LaneConfig cfg = makeLaneConfig(LanePreset::Sections);
    double sum = 0.0;
    for (const auto& c : cfg.ducks) {
        sum += c.speed;
    }
    const double mean = sum / static_cast<double>(cfg.ducks.size());
    bool anyDiffer = false;
    for (const auto& c : cfg.ducks) {
        CHECK(std::fabs(c.speed - mean) <= 0.10 * mean);   // modest: within ±10 %
        CHECK(c.dwell <= 0.5f);                            // no long, odd pauses
        CHECK(c.minDownTime >= 2.0);                       // brief G-18
        anyDiffer = anyDiffer || std::fabs(c.speed - cfg.ducks[0].speed) > 1.0f;
    }
    CHECK(anyDiffer);
    // Not synchronised: at no sampled instant are all five at the same
    // fraction of their travel moving the same way.
    DuckLane lane(cfg, boxes());
    for (int f = 0; f < 60 * 60; ++f) {
        lane.update(1.0 / 60.0);
        float lo = 2.0f, hi = -1.0f;
        int dirSum = 0;
        for (std::size_t i = 0; i < lane.size(); ++i) {
            const auto& c = lane.config(i);
            const float frac = (lane.motion(i).left - c.boundsMin) /
                               (c.boundsMax - c.boundsMin - kStandInWidth);
            lo = std::min(lo, frac);
            hi = std::max(hi, frac);
            dirSum += lane.motion(i).direction;
        }
        CHECK(hi - lo > 0.05f || std::abs(dirSum) != 5);
    }
}

TEST_CASE("targets: validation rejects overlap, out-of-gallery bounds and bad numbers") {
    const std::vector<float> widths(3, kStandInWidth);
    auto issuesFor = [&](LaneConfig cfg) {
        return validateLane(cfg, widths, L::kDuckHeight, L::kOpening);
    };
    LaneConfig cfg = makeLaneConfig(LanePreset::Sections);
    cfg.ducks[1].boundsMin -= 40.0f;   // pokes into section 0
    CHECK(!issuesFor(cfg).empty());

    cfg = makeLaneConfig(LanePreset::Sections);
    cfg.ducks[0].boundsMin = 150.0f;   // behind the left post
    CHECK(!issuesFor(cfg).empty());

    cfg = makeLaneConfig(LanePreset::Sections);
    cfg.ducks[3].boundsMax = cfg.ducks[3].boundsMin + kStandInWidth;   // no travel
    CHECK(!issuesFor(cfg).empty());

    for (auto mutate : std::vector<std::function<void(DuckConfig&)>>{
             [](DuckConfig& c) { c.speed = 0.0f; },
             [](DuckConfig& c) { c.speed = std::nanf(""); },
             [](DuckConfig& c) { c.fallTime = 0.0; },
             [](DuckConfig& c) { c.phase = 1.0; },
             [](DuckConfig& c) { c.startDirection = 0; },
             [](DuckConfig& c) { c.variant = 7; },
             [](DuckConfig& c) { c.trackY = 2000.0f; },
             [](DuckConfig& c) { c.wobbleDeg = 10.0f; }}) {
        cfg = makeLaneConfig(LanePreset::Sections);
        mutate(cfg.ducks[2]);
        CHECK(!issuesFor(cfg).empty());
    }

    cfg = makeLaneConfig(LanePreset::Chain);
    cfg.ducks[2].speed *= 1.05f;       // a chain cannot have one faster link
    CHECK(!issuesFor(cfg).empty());
    cfg = makeLaneConfig(LanePreset::Chain);
    cfg.ducks[1].phase = cfg.ducks[0].phase + 0.01;   // two ducks on top of each other
    CHECK(!issuesFor(cfg).empty());
    cfg = makeLaneConfig(LanePreset::Chain);
    cfg.ducks[0].mode = MotionMode::Shuttle;          // mixed modes on one track
    CHECK(!issuesFor(cfg).empty());
    CHECK(!issuesFor(LaneConfig{}).empty());
}

// ---------------------------------------------------------------------------
// Movement bounds
// ---------------------------------------------------------------------------

TEST_CASE("targets: every silhouette stays inside its section and the visible gallery") {
    for (const HitReaction r : {HitReaction::FlipBack, HitReaction::TiltDrop}) {
        LaneConfig cfg = makeLaneConfig(LanePreset::Sections);
        setReaction(cfg, r);
        DuckLane lane(cfg, boxes());
        Jitter jitter;
        std::size_t next = 0;
        for (int f = 0; f < 6000; ++f) {   // ~5 minutes at mixed frame times
            lane.update(f % 2 ? 1.0 / 60.0 : jitter());
            if (f % 97 == 0) {
                lane.knockDown(next++ % lane.size());   // exercise every reaction
            }
            for (std::size_t i = 0; i < lane.size(); ++i) {
                const auto& c = lane.config(i);
                const Quad q = lane.quad(i);
                // Half the section gap of slack for wobble and the reaction tip.
                CHECK(minX(q) >= c.boundsMin - 10.0f);
                CHECK(maxX(q) <= c.boundsMax + 10.0f);
                CHECK(minX(q) >= L::kOpening.x);
                CHECK(maxX(q) <= L::kOpening.x + L::kOpening.width);
                // Upright motion is horizontal only: the hinge never leaves the track.
                CHECK_NEAR(lane.pose(i).hinge.y, L::kTrackY, 1e-4);
                if (i + 1 < lane.size()) {
                    CHECK(maxX(q) <= minX(lane.quad(i + 1)));   // neighbours never overlap
                }
            }
        }
    }
}

TEST_CASE("targets: upright silhouettes sit in the 40-60 % lane band") {
    DuckLane lane(makeLaneConfig(LanePreset::Sections), boxes());
    for (int f = 0; f < 600; ++f) {
        lane.update(1.0 / 60.0);
        for (std::size_t i = 0; i < lane.size(); ++i) {
            const Quad q = lane.quad(i);
            CHECK(minY(q) >= L::kLaneBand.y - 1.0f);
            CHECK(maxY(q) <= L::kLaneBand.y + L::kLaneBand.height + 1.0f);
        }
    }
}

TEST_CASE("targets: chain preset ducks are evenly spaced and wrap only out of sight") {
    DuckLane lane(makeLaneConfig(LanePreset::Chain), boxes());
    const float spacing = L::kLoopLength / 5.0f;
    for (int f = 0; f < 60 * 30; ++f) {
        lane.update(1.0 / 60.0);
        for (std::size_t i = 0; i < lane.size(); ++i) {
            float gap = lane.motion((i + 1) % 5).left - lane.motion(i).left;
            if (gap < 0.0f) {
                gap += L::kLoopLength;
            }
            CHECK_NEAR(gap, spacing, 0.05);
            CHECK_NEAR(lane.motion(i).velocity, 0.25f * L::kOpening.width, 1e-3);
        }
    }
}

// ---------------------------------------------------------------------------
// Movement timing: frame-rate independence (D-007, ARCHITECTURE §7.2)
// ---------------------------------------------------------------------------

TEST_CASE("targets: a scripted session is identical at 30/60/144/240 Hz and jittery dt") {
    for (const LanePreset p : {LanePreset::Sections, LanePreset::Chain}) {
        for (const HitReaction r : {HitReaction::FlipBack, HitReaction::TiltDrop}) {
            LaneConfig cfg = makeLaneConfig(p);
            setReaction(cfg, r);
            const Session ref = play(cfg, [] { return 1.0 / 60.0; });
            CHECK(ref.transitions.size() >= 20);   // the script really exercised the states
            for (const double hz : {30.0, 144.0, 240.0}) {
                checkSameSession(ref, play(cfg, [hz] { return 1.0 / hz; }));
            }
            Jitter jitter;
            checkSameSession(ref, play(cfg, [&] { return jitter(); }));
            // One huge step per event: still the same outcome.
            checkSameSession(ref, play(cfg, [] { return 1000.0; }));
        }
    }
}

TEST_CASE("targets: motion over one second covers speed x time mid-pass") {
    DuckConfig c = single();
    c.phase = 0.0;
    DuckLane lane({{c}}, {box()});
    // dwell 0.2 + turn 0.3 -> cruising from t = 0.5.
    runTo(lane, 0.6, [] { return 1.0 / 60.0; });
    const float x0 = lane.motion(0).left;
    runTo(lane, 1.6, [] { return 1.0 / 144.0; });
    CHECK_NEAR(lane.motion(0).left - x0, 100.0, 1e-2);
}

TEST_CASE("targets: motionAt reports the exact position at a past lane time") {
    DuckLane lane(makeLaneConfig(LanePreset::Sections), boxes());
    runTo(lane, 1.0, [] { return 1.0 / 60.0; });
    const float at1 = lane.motion(3).left;
    runTo(lane, 1.75, [] { return 1.0 / 60.0; });
    CHECK_NEAR(lane.motionAt(3, 1.0).left, at1, 1e-3);
    CHECK_NEAR(lane.motionAt(3, lane.time()).left, lane.motion(3).left, 0.0);
}

TEST_CASE("targets: negative, zero and NaN frame times do nothing") {
    DuckLane lane(makeLaneConfig(LanePreset::Sections), boxes());
    const Snapshot before = snap(lane);
    lane.update(-0.5);
    lane.update(0.0);
    lane.update(std::nan(""));
    const Snapshot after = snap(lane);
    CHECK_NEAR(lane.time(), 0.0, 0.0);
    for (std::size_t i = 0; i < before.left.size(); ++i) {
        CHECK_NEAR(before.left[i], after.left[i], 0.0);
    }
}

// ---------------------------------------------------------------------------
// State transitions
// ---------------------------------------------------------------------------

TEST_CASE("targets: knockDown works only on an upright target") {
    DuckLane lane({{single()}}, {box()});
    CHECK(lane.active(0));
    CHECK(lane.knockDown(0));
    CHECK(lane.state(0) == DuckState::Falling);
    CHECK(!lane.active(0));
    CHECK(!lane.knockDown(0));          // already falling
    CHECK(!lane.knockDown(5));          // no such target
    lane.update(0.3);
    CHECK(lane.state(0) == DuckState::Down);
    CHECK(!lane.knockDown(0));          // already down
}

TEST_CASE("targets: falling takes exactly fallTime, rising exactly riseTime") {
    DuckConfig c = single();
    c.rise = RiseTrigger::AfterDownTime;
    DuckLane lane({{c}}, {box()});
    lane.knockDown(0);
    lane.update(0.199);
    CHECK(lane.state(0) == DuckState::Falling);
    lane.update(0.002);
    CHECK(lane.state(0) == DuckState::Down);
    runTo(lane, 0.2 + 1.999, [] { return 1.0 / 60.0; });
    CHECK(lane.state(0) == DuckState::Down);           // minDownTime 2 s not yet over
    runTo(lane, 0.2 + 2.001, [] { return 1.0 / 60.0; });
    CHECK(lane.state(0) == DuckState::Rising);
    runTo(lane, 0.2 + 2.0 + 0.249, [] { return 1.0 / 60.0; });
    CHECK(lane.state(0) == DuckState::Rising);
    runTo(lane, 0.2 + 2.0 + 0.251, [] { return 1.0 / 60.0; });
    CHECK(lane.state(0) == DuckState::Upright);
    CHECK(lane.active(0));

    const auto& tr = lane.transitions();
    CHECK_EQ(tr.size(), std::size_t{4});
    CHECK(tr[0].from == DuckState::Upright && tr[0].to == DuckState::Falling);
    CHECK(tr[1].to == DuckState::Down);
    CHECK(tr[2].to == DuckState::Rising);
    CHECK(tr[3].to == DuckState::Upright);
    CHECK_NEAR(tr[1].at, 0.2, 1e-9);
    CHECK_NEAR(tr[2].at, 2.2, 1e-9);
    CHECK_NEAR(tr[3].at, 2.45, 1e-9);
}

TEST_CASE("targets: one long step walks through every state in order at exact times") {
    DuckConfig c = single();
    c.rise = RiseTrigger::AfterDownTime;
    DuckLane lane({{c}}, {box()});
    lane.knockDown(0);
    lane.update(10.0);
    CHECK(lane.state(0) == DuckState::Upright);
    const auto& tr = lane.transitions();
    CHECK_EQ(tr.size(), std::size_t{4});
    CHECK_NEAR(tr[3].at, 2.45, 1e-9);
    CHECK_NEAR(lane.stateTime(0), 10.0 - 2.45, 1e-9);
}

TEST_CASE("targets: shuttle re-entry happens at the first end stop after the down time") {
    LaneConfig cfg = makeLaneConfig(LanePreset::Sections);
    DuckLane lane(cfg, boxes());
    runTo(lane, 1.0, [] { return 1.0 / 60.0; });
    CHECK(lane.knockDown(2));
    const DuckConfig& c = lane.config(2);
    const double eligible = 1.0 + c.fallTime + c.minDownTime;
    const DuckMotion& m = lane.motionModel(2);
    const double expected = eligible + m.timeToEdge(m.startClock() + eligible);
    runTo(lane, expected - 0.01, [] { return 1.0 / 60.0; });
    CHECK(lane.state(2) == DuckState::Down);
    runTo(lane, expected + 0.01, [] { return 1.0 / 60.0; });
    CHECK(lane.state(2) == DuckState::Rising);
    bool found = false;
    for (const auto& t : lane.transitions()) {
        if (t.id == 2 && t.to == DuckState::Rising) {
            CHECK_NEAR(t.at, expected, 1e-9);
            found = true;
        }
    }
    CHECK(found);
    // It comes back up at an end stop, not mid-pass.
    const MotionSample s = m.sample(m.startClock() + expected);
    const bool atStop = std::fabs(s.left - c.boundsMin) < 1e-2f ||
                        std::fabs(s.left - (c.boundsMax - kStandInWidth)) < 1e-2f;
    CHECK(atStop);
}

TEST_CASE("targets: chain duck hit just before the wrap stays down another lap (AC-8)") {
    DuckLane lane(makeLaneConfig(LanePreset::Chain), boxes());
    const double lap = lane.motionModel(0).edgeInterval();   // duck 0 wraps at k * lap
    runTo(lane, lap - 0.5, [] { return 1.0 / 60.0; });
    CHECK(lane.knockDown(0));
    runTo(lane, 2.0 * lap - 0.01, [] { return 1.0 / 60.0; });
    CHECK(lane.state(0) == DuckState::Down);          // first wrap came too soon
    runTo(lane, 2.0 * lap + 0.01, [] { return 1.0 / 60.0; });
    CHECK(lane.state(0) == DuckState::Rising);        // rises on the next wrap
    // ...which happens at the hidden wrap point behind the left post.
    const DuckMotion& m = lane.motionModel(0);
    const MotionSample atRise = m.sample(m.startClock() + 2.0 * lap);
    CHECK_NEAR(atRise.left, L::kLoopStart, 1e-2);
    CHECK(atRise.left + kStandInWidth <= L::kLeftPost.x + L::kLeftPost.width);
}

TEST_CASE("targets: equal-time transitions in one frame are logged chronologically") {
    LaneConfig cfg = makeLaneConfig(LanePreset::Sections);
    DuckLane lane(cfg, boxes());
    lane.knockDown(4);
    lane.update(0.1);
    lane.knockDown(0);
    lane.update(30.0);   // both complete full cycles inside one step
    const auto& tr = lane.transitions();
    for (std::size_t k = 1; k < tr.size(); ++k) {
        CHECK(tr[k - 1].at <= tr[k].at);
    }
}

TEST_CASE("targets: reset restores the configured start exactly") {
    DuckLane lane(makeLaneConfig(LanePreset::Sections), boxes());
    const Snapshot start = snap(lane);
    lane.knockDown(1);
    lane.knockDown(3);
    runTo(lane, 7.3, [] { return 1.0 / 60.0; });
    lane.reset();
    const Snapshot again = snap(lane);
    CHECK_NEAR(lane.time(), 0.0, 0.0);
    CHECK(lane.transitions().empty());
    for (std::size_t i = 0; i < start.left.size(); ++i) {
        CHECK_NEAR(start.left[i], again.left[i], 0.0);
        CHECK(again.state[i] == DuckState::Upright);
    }
}

// ---------------------------------------------------------------------------
// Poses and hit regions (one transform for drawing and hitting, R-3)
// ---------------------------------------------------------------------------

TEST_CASE("targets: an upright pose maps the image exactly onto the duck rect") {
    DuckLane lane({{single()}}, {box()});
    const Quad q = lane.quad(0);
    const Rect r = duckRect(lane.motion(0).left, kStandInWidth);
    CHECK_NEAR(q.p[0].x, r.x, 1e-3);
    CHECK_NEAR(q.p[0].y, r.y, 1e-3);
    CHECK_NEAR(q.p[2].x, r.x + r.width, 1e-3);
    CHECK_NEAR(q.p[2].y, r.y + r.height, 1e-3);
}

TEST_CASE("targets: hit region is the silhouette, not the box (AC-6)") {
    DuckConfig c = single();
    DuckLane lane({{c}}, {disc(100.0f, 100.0f)});
    const Quad q = lane.quad(0);
    const Vec2 centre{(q.p[0].x + q.p[2].x) / 2.0f, (q.p[0].y + q.p[2].y) / 2.0f};
    CHECK(lane.hitTest(centre) == std::optional<std::size_t>{0});
    CHECK(!lane.hitTest({q.p[0].x + 3.0f, q.p[0].y + 3.0f}).has_value());   // transparent corner
    CHECK(!lane.hitTest({centre.x, centre.y - 60.0f}).has_value());         // above the duck
}

TEST_CASE("targets: overlapping targets — only the front-most is hit (AC-7)") {
    DuckConfig a = single();
    DuckConfig b = single();
    b.boundsMin += 30.0f;   // overlaps a (validation would reject this; the lane still copes)
    b.boundsMax += 30.0f;
    DuckLane lane({{a, b}}, {box()});
    const Quad qa = lane.quad(0);
    const Vec2 both{qa.p[2].x - 5.0f, qa.p[2].y - 20.0f};   // inside both boxes
    CHECK(lane.hitTest(both) == std::optional<std::size_t>{1});   // later = drawn in front
    lane.knockDown(1);
    CHECK(lane.hitTest(both) == std::optional<std::size_t>{0});   // a downed duck never blocks
}

TEST_CASE("targets: falling, down and rising targets are never hittable") {
    for (const HitReaction r : {HitReaction::FlipBack, HitReaction::TiltDrop}) {
        DuckConfig c = single();
        c.reaction = r;
        c.rise = RiseTrigger::AfterDownTime;
        DuckLane lane({{c}}, {box()});
        lane.knockDown(0);
        for (int f = 0; f < 60 * 3; ++f) {
            const DuckPose p = lane.pose(0);
            CHECK(p.hittable == (lane.state(0) == DuckState::Upright));
            if (lane.state(0) != DuckState::Upright) {
                const Quad q = lane.quad(0);
                CHECK(!lane.hitTest({(q.p[0].x + q.p[2].x) / 2.0f, (q.p[0].y + q.p[2].y) / 2.0f})
                           .has_value());
            }
            lane.update(1.0 / 60.0);
        }
    }
}

TEST_CASE("targets: flip-back folds the silhouette down onto the hinge line") {
    DuckConfig c = single();
    DuckLane lane({{c}}, {box()});
    const float upright = maxY(lane.quad(0)) - minY(lane.quad(0));
    lane.knockDown(0);
    lane.update(0.1);
    const float mid = maxY(lane.quad(0)) - minY(lane.quad(0));
    CHECK(mid < upright && mid > 0.0f);
    lane.update(0.1 + 0.2);   // landed, rebound over
    CHECK(lane.state(0) == DuckState::Down);
    CHECK(maxY(lane.quad(0)) - minY(lane.quad(0)) < 0.5f);   // flat: nothing left to see
    CHECK_NEAR(maxY(lane.quad(0)), L::kTrackY, 1e-3);        // still on the hinge
}

TEST_CASE("targets: tilt-drop sinks fully behind the solid wave cutout, never below it") {
    LaneConfig cfg = makeLaneConfig(LanePreset::Sections);
    setReaction(cfg, HitReaction::TiltDrop);
    DuckLane lane(cfg, boxes());
    lane.knockDown(0);
    lane.update(0.5);
    CHECK(lane.state(0) == DuckState::Down);
    const Quad q = lane.quad(0);
    CHECK(minY(q) >= L::kWaveSolidTop);                       // hidden behind the waves
    CHECK(maxY(q) <= L::kWaveStrip.y + L::kWaveStrip.height); // not showing under them
}

TEST_CASE("targets: drawn transform and hit transform agree point for point (R-3)") {
    DuckLane lane(makeLaneConfig(LanePreset::Sections), {disc(128.0f, 105.0f), box(), box()});
    lane.update(1.234);
    const DuckPose p = lane.pose(0);
    const Affine2 xf = duckImageToLogical(p, lane.shape(0));
    for (int y = 0; y < 20; ++y) {
        for (int x = 0; x < 20; ++x) {
            const Vec2 logical = xf.apply({static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f});
            const bool hit = lane.hitTest(logical) == std::optional<std::size_t>{0};
            CHECK_EQ(hit, lane.shape(0).mask.solidAt(x, y));
        }
    }
}
