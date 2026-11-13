// Shooting: rifle cooldown, scoring, accuracy, hits/misses, repeated clicks,
// target state transitions, and mouse-pixel -> logical aim conversion.

#include "support/test_framework.hpp"

#include "gallery/game/Shooting.hpp"
#include "gallery/view/SceneLayout.hpp"
#include "gallery/view/Viewport.hpp"

#include <cmath>
#include <cstdint>
#include <vector>

using namespace gallery;
namespace L = gallery::layout;

namespace {

// Silhouette like a real cutout: a solid ellipse in a 64x52 image, so the
// corners of the bounding box are transparent.
DuckShape ellipseShape() {
    const int w = 64, h = 52;
    std::vector<std::uint8_t> px(static_cast<std::size_t>(w * h * 4), 255);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const float dx = (static_cast<float>(x) + 0.5f - w / 2.0f) / (w / 2.0f);
            const float dy = (static_cast<float>(y) + 0.5f - h / 2.0f) / (h / 2.0f);
            px[static_cast<std::size_t>((y * w + x) * 4 + 3)] = dx * dx + dy * dy <= 1.0f ? 255 : 0;
        }
    }
    const float lw = L::kDuckHeight * L::kDuckStandInAspect;
    return DuckShape{{lw, L::kDuckHeight}, {64.0f, 52.0f}, AlphaMask::fromRgba(px, w, h)};
}

Round makeRound(ShotTuning tuning = {}) {
    const DuckShape s = ellipseShape();
    // No countdown: these tests are about shooting, not the round clock.
    return Round(DuckLane(makeLaneConfig(LanePreset::Sections), {s, s, s}), tuning,
                 RoundTuning{.countdown = 0.0});
}

// Centre of target `id`'s silhouette as currently posed.
Vec2 centreOf(const DuckLane& lane, std::size_t id) {
    const Quad q = lane.quad(id);
    return {(q.p[0].x + q.p[2].x) / 2.0f, (q.p[0].y + q.p[2].y) / 2.0f};
}

const Vec2 kBackboard{960.0f, 380.0f};   // open backboard, well above the lane

template <class E>
std::size_t countEvents(const Round& r) {
    std::size_t n = 0;
    for (const auto& e : r.events()) {
        n += std::holds_alternative<E>(e) ? 1 : 0;
    }
    return n;
}

} // namespace

// ---------------------------------------------------------------------------
// Rifle (G-8, AC-4)
// ---------------------------------------------------------------------------

TEST_CASE("rifle: 249 ms after a shot is rejected, 250 ms is accepted") {
    Rifle r(0.25);
    CHECK(r.tryFire(1.0));
    CHECK(!r.tryFire(1.249));
    CHECK(r.tryFire(1.250));
    CHECK(!r.tryFire(1.250));             // same instant again: rejected
}

TEST_CASE("rifle: cooldown summed from 60 Hz frame deltas still opens at 250 ms") {
    Rifle r(0.25);
    double t = 0.0;
    CHECK(r.tryFire(t));
    for (int i = 0; i < 14; ++i) {
        t += 1.0 / 60.0;
    }
    CHECK(!r.tryFire(t));                 // 233 ms
    t += 1.0 / 60.0;                      // 15 frames = 250 ms (with rounding error)
    CHECK(r.tryFire(t));
}

TEST_CASE("rifle: rejected clicks do not restart the cooldown; reset makes it ready") {
    Rifle r(0.25);
    CHECK(r.tryFire(0.0));
    CHECK(!r.tryFire(0.1));
    CHECK(!r.tryFire(0.2));
    CHECK(r.tryFire(0.25));               // measured from the accepted shot, not the clicks
    CHECK_NEAR(r.remaining(0.30), 0.20, 1e-9);
    r.reset();
    CHECK(r.ready(0.26));
}

// ---------------------------------------------------------------------------
// Stats and accuracy (G-22, G-23, AC-9, AC-10)
// ---------------------------------------------------------------------------

TEST_CASE("accuracy: no shots is '—' (nullopt), rounding is half up") {
    RoundStats s;
    CHECK(!s.accuracyPercent().has_value());
    auto acc = [](int hits, int misses) {
        RoundStats r;
        for (int i = 0; i < hits; ++i) r.recordHit(10);
        for (int i = 0; i < misses; ++i) r.recordMiss();
        return *r.accuracyPercent();
    };
    CHECK_EQ(acc(1, 2), 33);
    CHECK_EQ(acc(2, 1), 67);
    CHECK_EQ(acc(1, 7), 13);    // 12.5 -> 13
    CHECK_EQ(acc(5, 0), 100);
    CHECK_EQ(acc(0, 4), 0);
    CHECK_EQ(acc(1, 199), 1);   // 0.5 -> 1
    CHECK_EQ(acc(199, 1), 100); // 99.5 -> 100
}

TEST_CASE("stats: hits add exactly 10 points each; misses never change the score") {
    RoundStats s;
    s.recordHit(10);
    s.recordMiss();
    s.recordMiss();
    s.recordHit(10);
    CHECK_EQ(s.score(), 20);
    CHECK_EQ(s.shots(), 4);
    CHECK_EQ(s.hits(), 2);
    CHECK_EQ(s.misses(), 2);
}

// ---------------------------------------------------------------------------
// Round::fire — hits, misses, repeated clicks
// ---------------------------------------------------------------------------

TEST_CASE("shot: a hit knocks the target down and awards points once") {
    Round r = makeRound();
    const ShotResult res = r.fire(centreOf(r.lane(), 2));
    CHECK(res.outcome == ShotOutcome::Hit);
    CHECK(res.duck == std::optional<std::size_t>{2});
    CHECK(r.lane().state(2) == DuckState::Falling);
    CHECK(!r.lane().active(2));
    CHECK_EQ(r.stats().score(), 10);
    CHECK_EQ(r.stats().hits(), 1);
    CHECK_EQ(r.stats().shots(), 1);
    CHECK_EQ(countEvents<event::ShotFired>(r), std::size_t{1});
    CHECK_EQ(countEvents<event::DuckHit>(r), std::size_t{1});
    CHECK_EQ(countEvents<event::ShotMissed>(r), std::size_t{0});
}

TEST_CASE("shot: a miss is counted and gives feedback but leaves the score alone") {
    Round r = makeRound();
    const ShotResult res = r.fire(kBackboard);
    CHECK(res.outcome == ShotOutcome::Miss);
    CHECK_EQ(r.stats().score(), 0);
    CHECK_EQ(r.stats().misses(), 1);
    CHECK_EQ(r.stats().shots(), 1);
    CHECK_EQ(countEvents<event::ShotFired>(r), std::size_t{1});
    CHECK_EQ(countEvents<event::ShotMissed>(r), std::size_t{1});
    for (std::size_t i = 0; i < r.lane().size(); ++i) {
        CHECK(r.lane().active(i));
    }
}

TEST_CASE("shot: transparent corner of a target's box is a miss (alpha-aware, AC-6)") {
    Round r = makeRound();
    const Quad q = r.lane().quad(1);
    const Vec2 corner{q.p[0].x + 4.0f, q.p[0].y + 4.0f};   // inside the box, outside the ellipse
    CHECK(r.fire(corner).outcome == ShotOutcome::Miss);
    CHECK(r.lane().active(1));
    CHECK_EQ(r.stats().score(), 0);
}

TEST_CASE("shot: many clicks in one frame fire one shot (AC-4)") {
    Round r = makeRound();
    int accepted = 0;
    for (int i = 0; i < 10; ++i) {
        accepted += r.fire(kBackboard).outcome != ShotOutcome::Rejected ? 1 : 0;
    }
    CHECK_EQ(accepted, 1);
    CHECK_EQ(r.stats().shots(), 1);
    CHECK_EQ(countEvents<event::ShotFired>(r), std::size_t{1});
}

TEST_CASE("shot: clicks during the cooldown are ignored and not counted") {
    Round r = makeRound();
    CHECK(r.fire(kBackboard).outcome == ShotOutcome::Miss);
    for (int f = 0; f < 14; ++f) {                       // 233 ms of 60 Hz frames
        r.update(1.0 / 60.0);
        CHECK(r.fire(centreOf(r.lane(), 0)).outcome == ShotOutcome::Rejected);
    }
    CHECK_EQ(r.stats().shots(), 1);
    CHECK(r.lane().active(0));                           // the rejected clicks hit nothing
    r.update(1.0 / 60.0);                                // 250 ms
    CHECK(r.fire(centreOf(r.lane(), 0)).outcome == ShotOutcome::Hit);
    CHECK_EQ(r.stats().shots(), 2);
}

TEST_CASE("shot: a second shot at the same target cannot score it twice") {
    Round r = makeRound(ShotTuning{.fireCooldown = 0.0, .pointsPerHit = 10});
    const Vec2 aim = centreOf(r.lane(), 3);
    CHECK(r.fire(aim).outcome == ShotOutcome::Hit);
    CHECK(r.fire(aim).outcome == ShotOutcome::Miss);     // now falling: not hittable
    CHECK_EQ(r.stats().score(), 10);
    CHECK_EQ(r.stats().hits(), 1);
    CHECK_EQ(r.stats().misses(), 1);
}

TEST_CASE("shot: falling, down and rising targets cannot be hit; upright again can") {
    Round r = makeRound(ShotTuning{.fireCooldown = 0.0, .pointsPerHit = 10});
    CHECK(r.fire(centreOf(r.lane(), 0)).outcome == ShotOutcome::Hit);
    int scored = 0;
    bool sawEachState[4] = {};
    for (int f = 0; f < 60 * 8; ++f) {
        r.update(1.0 / 60.0);
        const DuckState st = r.lane().state(0);
        sawEachState[static_cast<int>(st)] = true;
        // Aim at where the box would be, even while folded or sunk.
        const Vec2 aim{r.lane().pose(0).hinge.x, L::kTrackY - L::kDuckHeight / 2.0f};
        const ShotResult res = r.fire(aim);
        if (st != DuckState::Upright) {
            CHECK(res.outcome != ShotOutcome::Hit);
        } else if (res.outcome == ShotOutcome::Hit) {
            ++scored;
            break;   // back up and hittable again
        }
    }
    CHECK(sawEachState[1] && sawEachState[2] && sawEachState[3]);
    CHECK_EQ(scored, 1);
    CHECK_EQ(r.stats().hits(), 2);
    CHECK_EQ(r.stats().score(), 20);
}

TEST_CASE("shot: resolves against the targets as last drawn, before the frame update (D-008)") {
    Round r = makeRound();
    r.update(0.8);   // targets mid-pass
    const Quad q = r.lane().quad(4);
    // Right at the trailing edge band of the silhouette's middle row.
    const MotionSample m = r.lane().motion(4);
    const float trailing = m.velocity > 0 ? q.p[0].x + 6.0f : q.p[1].x - 6.0f;
    const Vec2 aim{trailing, (q.p[0].y + q.p[2].y) / 2.0f};
    CHECK(r.fire(aim).outcome == ShotOutcome::Hit);      // what the player saw is what is hit
}

TEST_CASE("shot: a non-finite aim is ignored") {
    Round r = makeRound();
    r.clearEvents();
    CHECK(r.fire({std::nanf(""), 500.0f}).outcome == ShotOutcome::Rejected);
    CHECK_EQ(r.stats().shots(), 0);
    CHECK(r.events().empty());
}

TEST_CASE("shot: reset clears score, shots, events and the cooldown") {
    Round r = makeRound();
    r.fire(centreOf(r.lane(), 1));
    r.update(0.1);
    r.reset();
    CHECK_EQ(r.stats().shots(), 0);
    CHECK_EQ(r.stats().score(), 0);
    CHECK(!r.stats().accuracyPercent().has_value());
    CHECK_EQ(r.events().size(), std::size_t{2});   // only the reset (and Go: no countdown)
    CHECK_EQ(countEvents<event::RoundReset>(r), std::size_t{1});
    CHECK(r.lane().active(1));
    CHECK(r.fire(kBackboard).outcome == ShotOutcome::Miss);   // ready immediately
}

// ---------------------------------------------------------------------------
// Coordinate conversion: mouse pixels -> logical aim -> hit
// ---------------------------------------------------------------------------

TEST_CASE("aim: clicking a target's pixel centre hits it at any window size and aspect") {
    const Size windows[] = {{1920, 1080}, {1280, 720}, {1600, 900}, {1200, 900},
                            {2520, 1080}, {3840, 2400}, {641, 359},  {800, 1000}};
    for (const Size w : windows) {
        const Viewport vp = Viewport::fit(w);
        Round r = makeRound();
        r.update(1.37);
        for (std::size_t id = 0; id < r.lane().size(); ++id) {
            const Vec2 logical = centreOf(r.lane(), id);
            const Vec2 px = vp.toPixel(logical);
            // The OS reports whole pixels.
            const Vec2 mouse{std::floor(px.x), std::floor(px.y)};
            const Vec2 aim = vp.clampToLogical(vp.toLogical(mouse));
            // One window pixel is at most 1/scale logical units off.
            CHECK(std::fabs(aim.x - logical.x) <= 1.0f / vp.scale() + 1e-3f);
            CHECK(std::fabs(aim.y - logical.y) <= 1.0f / vp.scale() + 1e-3f);
            Round shot = makeRound();
            shot.update(1.37);
            const ShotResult res = shot.fire(aim);
            CHECK(res.outcome == ShotOutcome::Hit);
            CHECK(res.duck == std::optional<std::size_t>{id});
        }
    }
}

TEST_CASE("aim: a click in the letterbox bars clamps to the canvas edge and never hits") {
    const Viewport vp = Viewport::fit({2520, 1080});   // bars left and right
    const Rect canvas = vp.pixelRect();
    CHECK(canvas.x > 0.0f);
    Round r = makeRound();
    const Vec2 aim = vp.clampToLogical(vp.toLogical({canvas.x / 2.0f, 548.0f}));
    CHECK_NEAR(aim.x, 0.0, 1e-4);
    CHECK(r.fire(aim).outcome == ShotOutcome::Miss);   // a shot, but nothing is there
}
