// Engineering verification pass (2026-11-12): regression tests for the
// defects found, and gameplay checks that were not yet covered.
//
//   - Foreground occlusion: a shot on scenery drawn in front of a target
//     (booth post, wave cutout) hits the scenery, not the hidden target.
//   - Frame-time statistics used for the AV-7 performance log.
//   - A whole session (shots, pause, round end) is identical at 30/60/144/240 Hz.
//   - Pause freezes the rifle cooldown; the Ending phase cannot be paused or shot.
//   - Holding the trigger never exceeds the 4 shots/s fire rate at any frame rate.

#include "support/test_framework.hpp"
#include "support/test_round.hpp"

#include "gallery/core/FrameStats.hpp"
#include "gallery/flow/Game.hpp"
#include "gallery/game/Occluder.hpp"
#include "gallery/view/SceneLayout.hpp"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <string>
#include <vector>

using namespace gallery;
namespace L = gallery::layout;
using gallery::test::centreOf;
using gallery::test::ellipseShape;
using gallery::test::kOpenBackboard;
using gallery::test::makeTimedRound;

namespace {

// A w x h mask over the 1920x1080 canvas; texels whose centre lies in any of
// `rects` (logical units) are opaque.
AlphaMask rectsMask(int w, int h, std::initializer_list<Rect> rects) {
    std::vector<std::uint8_t> px(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4, 0);
    const float sx = static_cast<float>(w) / kLogicalSize.width;
    const float sy = static_cast<float>(h) / kLogicalSize.height;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const Vec2 p{(static_cast<float>(x) + 0.5f) / sx, (static_cast<float>(y) + 0.5f) / sy};
            for (const Rect& r : rects) {
                if (r.contains(p)) {
                    px[(static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
                        static_cast<std::size_t>(x)) * 4 + 3] = 255;
                    break;
                }
            }
        }
    }
    return AlphaMask::fromRgba(px, w, h);
}

Round liveRound(LanePreset preset, Occluder foreground) {
    const DuckShape s = ellipseShape();
    return Round(DuckLane(makeLaneConfig(preset), {s, s, s}), ShotTuning{},
                 RoundTuning{.countdown = 0.0}, std::move(foreground));
}

} // namespace

// ---------------------------------------------------------------------------
// Occluder
// ---------------------------------------------------------------------------

TEST_CASE("occluder: empty covers nothing; texels map onto the logical canvas") {
    const Occluder none;
    CHECK(none.empty());
    CHECK(!none.covers({960.0f, 540.0f}));

    // 4x2 texels over 1920x1080: texel (3, 1) is the bottom-right quarter-half.
    std::vector<std::uint8_t> px(4 * 2 * 4, 0);
    px[(1 * 4 + 3) * 4 + 3] = 255;
    const Occluder o(AlphaMask::fromRgba(px, 4, 2));
    CHECK(!o.empty());
    CHECK(o.covers({1440.0f, 540.0f}));           // first logical point of that texel
    CHECK(o.covers({1919.9f, 1079.9f}));
    CHECK(!o.covers({1439.9f, 540.0f}));          // texel (2, 1)
    CHECK(!o.covers({1440.0f, 539.9f}));          // texel (3, 0)
}

TEST_CASE("occluder: off-canvas, non-finite points and bad sizes never cover") {
    const Occluder o(rectsMask(96, 54, {L::kCanvas}));
    CHECK(o.covers({0.0f, 0.0f}));
    CHECK(!o.covers({-0.5f, 500.0f}));
    CHECK(!o.covers({1920.0f, 500.0f}));
    CHECK(!o.covers({500.0f, 1080.0f}));
    CHECK(!o.covers({std::numeric_limits<float>::quiet_NaN(), 500.0f}));
    CHECK(!o.covers({std::numeric_limits<float>::infinity(), 500.0f}));
    const Occluder bad(rectsMask(8, 8, {L::kCanvas}), Size{0.0f, 1080.0f});
    CHECK(bad.empty());
    CHECK(!bad.covers({10.0f, 10.0f}));
}

// Regression for D1: in the chain preset a duck wrapping behind the left
// booth post was hit by a click on the post itself (found by a scripted run:
// "--targets chain --aim 160,562" scored +10 on a hidden target).
TEST_CASE("occlusion: a shot at a booth post never hits the duck hidden behind it") {
    const Occluder posts(rectsMask(960, 540, {L::kLeftPost, L::kRightPost}));
    Round round = liveRound(LanePreset::Chain, posts);
    Round bare = liveRound(LanePreset::Chain, Occluder{});

    // Run until some duck's centre is behind the left post.
    std::optional<std::size_t> hidden;
    for (int f = 0; f < 600 && !hidden; ++f) {
        round.update(1.0 / 60.0);
        bare.update(1.0 / 60.0);
        for (std::size_t i = 0; i < round.lane().size(); ++i) {
            const Vec2 c = centreOf(round.lane(), i);
            if (c.x > L::kLeftPost.x + 20.0f && c.x < L::kLeftPost.x + L::kLeftPost.width - 20.0f) {
                hidden = i;
                break;
            }
        }
    }
    CHECK(hidden.has_value());
    const Vec2 onPost = centreOf(round.lane(), *hidden);
    CHECK(round.lane().hitTest(onPost) == hidden);    // the silhouette IS there, behind the post
    CHECK(bare.fire(onPost).outcome == ShotOutcome::Hit);   // the old behaviour: a hit

    const ShotResult r = round.fire(onPost);
    CHECK(r.outcome == ShotOutcome::Miss);
    CHECK(!round.targetUnder(onPost).has_value());
    CHECK_EQ(round.stats().score(), 0);
    CHECK_EQ(round.stats().misses(), 1);
    CHECK(round.lane().state(*hidden) == DuckState::Upright);   // not knocked down
}

TEST_CASE("occlusion: the wave cutout covers the duck's base; the visible body still hits") {
    // Worst case for the wave: opaque from the crest line down, everywhere.
    const Rect waves{L::kWaveStrip.x, L::kWaveStrip.y, L::kWaveStrip.width,
                     kLogicalSize.height - L::kWaveStrip.y};
    Round round = liveRound(LanePreset::Sections, Occluder(rectsMask(1920, 1080, {waves})));
    const Vec2 body = centreOf(round.lane(), 0);
    const Vec2 base{body.x, L::kTrackY - 5.0f};                  // behind the crest line
    CHECK(base.y > L::kWaveStrip.y);
    CHECK(round.lane().hitTest(base) == std::optional<std::size_t>{0});   // silhouette there
    CHECK(!round.targetUnder(base).has_value());

    CHECK(round.fire(base).outcome == ShotOutcome::Miss);
    round.update(0.25);   // rifle ready again; the shuttle carriage moved a little
    const Vec2 now = centreOf(round.lane(), 0);
    CHECK(now.y < L::kWaveStrip.y);
    const ShotResult hit = round.fire(now);
    CHECK(hit.outcome == ShotOutcome::Hit);
    CHECK(hit.duck == std::optional<std::size_t>{0});
    CHECK_EQ(round.stats().shots(), 2);
    CHECK_EQ(round.stats().hits(), 1);
}

TEST_CASE("occlusion: with nothing in front, behaviour is exactly the lane's hit test") {
    Round round = liveRound(LanePreset::Sections, Occluder{});
    for (std::size_t i = 0; i < round.lane().size(); ++i) {
        const Vec2 c = centreOf(round.lane(), i);
        CHECK(round.targetUnder(c) == round.lane().hitTest(c));
    }
    CHECK(!round.targetUnder(kOpenBackboard).has_value());
}

// ---------------------------------------------------------------------------
// Frame-time statistics (AV-7 log)
// ---------------------------------------------------------------------------

TEST_CASE("frame stats: empty, mean, max, percentiles and the 33 ms hitch count") {
    FrameStats s;
    CHECK_EQ(s.count(), 0u);
    CHECK_EQ(s.mean(), 0.0);
    CHECK_EQ(s.percentile(0.99), 0.0);

    for (int i = 0; i < 98; ++i) {
        s.add(0.016);
    }
    s.add(0.020);
    s.add(0.050);
    CHECK_EQ(s.count(), 100u);
    CHECK_NEAR(s.mean(), (98 * 0.016 + 0.020 + 0.050) / 100.0, 1e-12);
    CHECK_EQ(s.max(), 0.050);
    CHECK_EQ(s.hitches(), 1u);
    CHECK_NEAR(s.percentile(0.50), 0.016, FrameStats::kBinWidth * 1.01);
    CHECK_NEAR(s.percentile(0.99), 0.020, FrameStats::kBinWidth * 1.01);
    CHECK_EQ(s.percentile(1.0), 0.050);   // capped at the exact maximum
}

TEST_CASE("frame stats: hitch boundary, very slow frames and bad input") {
    FrameStats s;
    s.add(FrameStats::kHitch);   // exactly 33.3 ms is not over it
    CHECK_EQ(s.hitches(), 0u);
    s.add(FrameStats::kHitch + 1e-6);
    CHECK_EQ(s.hitches(), 1u);
    s.add(0.5);                  // beyond the histogram: last bin, max exact
    CHECK_EQ(s.max(), 0.5);
    CHECK_EQ(s.percentile(1.0), 0.5);
    s.add(-1.0);
    s.add(std::numeric_limits<double>::quiet_NaN());
    s.add(std::numeric_limits<double>::infinity());
    CHECK_EQ(s.count(), 3u);
}

// ---------------------------------------------------------------------------
// Whole-session determinism and timing edge cases
// ---------------------------------------------------------------------------

namespace {

struct Session {
    std::string shots;      // 'H<id>' per hit, 'm' per miss, in order
    int score = 0;
    int hits = 0;
    int misses = 0;
    long playingFrames = 0;
};

// Start a round, fire every 0.5 s of round time from 2.0 s to 41.5 s at a
// fixed aim, pause at 10 s for 3 s of frames, then play to the results.
Session playSession(int hz) {
    const Seconds dt = 1.0 / hz;
    Game g(makeTimedRound());
    g.handle(input::KeyPressed{Key::Enter});
    g.clearEvents();
    const Vec2 aim{656.0f, 550.0f};   // centre of section #1's travel, at body height
    Session s;
    bool paused = false;
    auto collect = [&] {
        for (const GameEvent& e : g.events()) {
            if (const auto* h = std::get_if<event::DuckHit>(&e)) {
                s.shots += "H" + std::to_string(h->id);
            } else if (std::holds_alternative<event::ShotMissed>(e)) {
                s.shots += "m";
            }
        }
        g.clearEvents();
    };
    for (long guard = 0; guard < 100L * hz && g.state() != AppState::Results; ++guard) {
        const long f = s.playingFrames;
        if (!paused && f == 10L * hz) {
            g.handle(input::KeyPressed{Key::P});
            const Seconds frozen = g.round().time();
            for (int k = 0; k < 3 * hz; ++k) {
                g.update(dt);
            }
            if (g.round().time() != frozen || g.state() != AppState::Paused) {
                throw gallery::test::Failure{"pause did not freeze the round"};
            }
            g.handle(input::KeyPressed{Key::P});
            paused = true;
        }
        if (f >= 2L * hz && f <= 83L * hz / 2 && f % (hz / 2) == 0) {
            g.handle(input::PrimaryPressed{aim});
        }
        g.update(dt);
        ++s.playingFrames;
        collect();
    }
    s.score = g.round().stats().score();
    s.hits = g.round().stats().hits();
    s.misses = g.round().stats().misses();
    return s;
}

} // namespace

TEST_CASE("session: shots, pause and round end are identical at 30/60/144/240 Hz") {
    const Session ref = playSession(60);
    CHECK_EQ(ref.hits + ref.misses, 80);
    CHECK(ref.hits > 0);
    CHECK(ref.misses > 0);
    CHECK_EQ(ref.score, ref.hits * 10);
    // 1.5 s countdown + 60 s + 1 s end delay of playing time, to within a frame.
    CHECK_NEAR(static_cast<double>(ref.playingFrames) / 60.0, 62.5, 1.0 / 60.0);
    for (const int hz : {30, 144, 240}) {
        const Session s = playSession(hz);
        CHECK_EQ(s.shots, ref.shots);
        CHECK_EQ(s.score, ref.score);
        CHECK_EQ(s.hits, ref.hits);
        CHECK_EQ(s.misses, ref.misses);
        CHECK_NEAR(static_cast<double>(s.playingFrames) / hz, 62.5, 1.0 / hz);
    }
}

TEST_CASE("pause: the rifle cooldown is frozen with the round, not run down by the pause") {
    Game g(makeTimedRound(RoundTuning{.countdown = 0.0}));
    g.handle(input::KeyPressed{Key::Enter});
    const auto first = g.handle(input::PrimaryPressed{kOpenBackboard});
    CHECK(first && first->outcome == ShotOutcome::Miss);
    g.update(0.10);
    g.handle(input::KeyPressed{Key::P});
    CHECK(g.state() == AppState::Paused);
    for (int k = 0; k < 120; ++k) {
        g.update(1.0 / 60.0);   // two seconds of wall time while paused
    }
    g.handle(input::KeyPressed{Key::P});
    CHECK(g.state() == AppState::Playing);
    const auto early = g.handle(input::PrimaryPressed{kOpenBackboard});
    CHECK(early && early->outcome == ShotOutcome::Rejected);
    CHECK(early->reason == RejectReason::Cooldown);
    CHECK_NEAR(g.round().rifle().remaining(g.round().time()), 0.15, 1e-9);
    CHECK_EQ(g.round().stats().shots(), 1);
    g.update(0.15);
    const auto ready = g.handle(input::PrimaryPressed{kOpenBackboard});
    CHECK(ready && ready->outcome == ShotOutcome::Miss);
}

TEST_CASE("ending: after time is up nothing pauses or fires, and results follow 1 s later") {
    const Seconds dt = 1.0 / 60.0;
    Game g(makeTimedRound(RoundTuning{.countdown = 0.0, .length = 2.0}));
    g.handle(input::KeyPressed{Key::Enter});
    while (g.round().clock().phase() == RoundPhase::Live) {
        g.update(dt);
    }
    CHECK(g.round().clock().phase() == RoundPhase::Ending);
    const Seconds timeUpAt = g.round().time();
    g.handle(input::KeyPressed{Key::Escape});
    g.handle(input::KeyPressed{Key::P});
    g.handle(input::FocusLost{});
    CHECK(g.state() == AppState::Playing);   // the round runs out its end delay
    const auto click = g.handle(input::PrimaryPressed{kOpenBackboard});
    CHECK(click && click->outcome == ShotOutcome::Rejected);
    CHECK(click->reason == RejectReason::NotLive);
    CHECK_EQ(g.round().stats().shots(), 0);
    while (g.state() == AppState::Playing) {
        g.update(dt);
    }
    CHECK(g.state() == AppState::Results);
    CHECK_NEAR(g.round().time() - timeUpAt, 1.0, dt);
}

namespace {
int holdTrigger(int hz) {
    Game g(makeTimedRound());
    g.handle(input::KeyPressed{Key::Enter});
    for (long guard = 0; guard < 100L * hz && g.state() == AppState::Playing; ++guard) {
        g.handle(input::PrimaryPressed{kOpenBackboard});   // a click every single frame
        g.update(1.0 / hz);
    }
    return g.round().stats().shots();
}
} // namespace

TEST_CASE("fire rate: clicking every frame never beats 4 shots/s at any frame rate") {
    // 60 s live at one shot per 250 ms: shots at live time 0, 0.25 .. 59.75.
    CHECK_EQ(holdTrigger(60), 240);
    CHECK_EQ(holdTrigger(144), 240);
    CHECK_EQ(holdTrigger(240), 240);
    // At 30 Hz the 250 ms cooldown ends mid-frame, so the next shot waits for
    // the next frame (every 267 ms): frame-quantised, never faster.
    CHECK_EQ(holdTrigger(30), 225);
}
