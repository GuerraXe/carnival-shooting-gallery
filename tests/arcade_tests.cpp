// D-035 (owner request): magazine and reload, dry fire, decoy targets that
// cost points, and the golden power-up target (free ammo).

#include "support/test_framework.hpp"

#include "gallery/audio/Soundscape.hpp"
#include "gallery/flow/Game.hpp"
#include "gallery/game/Shooting.hpp"
#include "gallery/view/SceneLayout.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

using namespace gallery;
namespace L = gallery::layout;

namespace {

// Fully solid box silhouette: every point inside the posed quad is a hit.
DuckShape boxShape() {
    const float lw = L::kDuckHeight * L::kDuckStandInAspect;
    return DuckShape{{lw, L::kDuckHeight}, {lw, L::kDuckHeight}, {}};
}

Round makeRound(ShotTuning tuning, TargetMix mix = {}, bool quickRise = false) {
    LaneConfig cfg = makeLaneConfig(LanePreset::Sections);
    cfg.mix = mix;
    if (quickRise) {
        for (DuckConfig& d : cfg.ducks) {
            d.rise = RiseTrigger::AfterDownTime;
        }
    }
    const DuckShape s = boxShape();
    return Round(DuckLane(cfg, {s, s, s}), tuning, RoundTuning{.countdown = 0.0});
}

Vec2 centreOf(const DuckLane& lane, std::size_t id) {
    const Quad q = lane.quad(id);
    return {(q.p[0].x + q.p[2].x) / 2.0f, (q.p[0].y + q.p[2].y) / 2.0f};
}

const Vec2 kBackboard{960.0f, 380.0f};

template <class E>
std::size_t countEvents(const Round& r) {
    std::size_t n = 0;
    for (const auto& e : r.events()) {
        n += std::holds_alternative<E>(e) ? 1 : 0;
    }
    return n;
}

// Fire at the backboard after waiting out the cooldown.
ShotResult missShot(Round& r) {
    r.update(0.26);
    return r.fire(kBackboard);
}

TargetMix allDecoys() {
    TargetMix m;
    m.decoyChance = 1.0f;
    m.maxDecoys = 5;
    return m;
}

TargetMix goldenOnRise() {
    TargetMix m;
    m.goldenChance = 1.0f;
    m.maxGolden = 5;
    return m;
}

} // namespace

// ---------------------------------------------------------------------------
// Magazine and reload
// ---------------------------------------------------------------------------

TEST_CASE("ammo: the classic tuning has no magazine; the game's has six rounds") {
    CHECK_EQ(ShotTuning{}.magazine, 0);
    CHECK_EQ(ShotTuning::standard().magazine, 6);
    Round classic = makeRound(ShotTuning{});
    for (int i = 0; i < 20; ++i) {
        CHECK(missShot(classic).outcome == ShotOutcome::Miss);   // never runs dry
    }
    CHECK(!classic.reload());                                     // nothing to reload
}

TEST_CASE("ammo: each shot uses a round; the seventh pull is a dry fire, not a shot") {
    Round r = makeRound(ShotTuning::standard());
    CHECK_EQ(r.ammo(), 6);
    for (int i = 0; i < 6; ++i) {
        CHECK(missShot(r).outcome == ShotOutcome::Miss);
    }
    CHECK_EQ(r.ammo(), 0);
    r.clearEvents();
    const ShotResult dry = missShot(r);
    CHECK(dry.outcome == ShotOutcome::Rejected);
    CHECK(dry.reason == RejectReason::Empty);
    CHECK_EQ(r.stats().shots(), 6);                                // not counted
    CHECK_EQ(countEvents<event::DryFire>(r), std::size_t{1});
    CHECK_EQ(countEvents<event::ShotFired>(r), std::size_t{0});
    // Clicking an empty rifle cannot spam clicks: the action still cycles.
    r.clearEvents();
    CHECK(r.fire(kBackboard).reason == RejectReason::Cooldown);
    CHECK_EQ(countEvents<event::DryFire>(r), std::size_t{0});
}

TEST_CASE("reload: takes reloadTime of round time; no shots meanwhile; refills the magazine") {
    Round r = makeRound(ShotTuning::standard());
    CHECK(!r.reload());                       // full: nothing to do
    missShot(r);
    missShot(r);
    CHECK_EQ(r.ammo(), 4);
    r.clearEvents();
    CHECK(r.reload());
    CHECK(r.reloading());
    CHECK_EQ(countEvents<event::ReloadStarted>(r), std::size_t{1});
    CHECK(!r.reload());                       // already reloading
    r.update(0.6);
    CHECK_NEAR(r.reloadProgress(), 0.5f, 1e-4);
    CHECK(r.fire(kBackboard).reason == RejectReason::Reloading);
    CHECK_EQ(r.stats().shots(), 2);
    r.clearEvents();
    r.update(0.6);
    CHECK(!r.reloading());
    CHECK_EQ(r.ammo(), 6);
    CHECK_EQ(countEvents<event::Reloaded>(r), std::size_t{1});
    CHECK(r.fire(kBackboard).outcome == ShotOutcome::Miss);
}

TEST_CASE("reload: frame-rate independent and stopped at time up") {
    for (const double hz : {30.0, 60.0, 144.0}) {
        Round r = makeRound(ShotTuning::standard());
        missShot(r);
        CHECK(r.reload());
        double t = 0.0;
        while (r.reloading()) {
            r.update(1.0 / hz);
            t += 1.0 / hz;
        }
        CHECK(t >= 1.2 - 1e-9);
        CHECK(t < 1.2 + 1.0 / hz + 1e-9);
    }
    // A reload still running when time is up never completes.
    Round late = Round(DuckLane(makeLaneConfig(LanePreset::Sections), {boxShape(), boxShape(), boxShape()}),
                       ShotTuning::standard(), RoundTuning{.countdown = 0.0, .length = 1.0});
    late.update(0.3);
    late.fire(kBackboard);
    late.update(0.6);   // t = 0.9
    CHECK(late.reload());
    late.update(5.0);
    CHECK(late.reloading());
    CHECK_EQ(late.ammo(), 5);
}

TEST_CASE("reload: R and right-click reload in a round, are never shots, and do nothing on boards") {
    Game g(makeRound(ShotTuning::standard()));
    g.handle(input::KeyPressed{Key::Enter});
    CHECK(g.state() == AppState::Playing);
    g.update(0.3);
    g.handle(input::PrimaryPressed{kBackboard});
    CHECK_EQ(g.round().ammo(), 5);
    CHECK(!g.handle(input::KeyPressed{Key::R}).has_value());
    CHECK(g.round().reloading());
    CHECK(g.state() == AppState::Playing);   // R in a round is no longer a no-op, and never a restart
    g.update(1.3);
    CHECK_EQ(g.round().ammo(), 6);
    g.update(0.3);
    g.handle(input::PrimaryPressed{kBackboard});
    CHECK(!g.handle(input::SecondaryPressed{kBackboard}).has_value());
    CHECK(g.round().reloading());
    CHECK_EQ(g.round().stats().shots(), 2);
    // On the pause board R still restarts; right-click does nothing.
    g.handle(input::KeyPressed{Key::Escape});
    CHECK(g.state() == AppState::Paused);
    g.handle(input::SecondaryPressed{kBackboard});
    CHECK(g.state() == AppState::Paused);
    g.handle(input::KeyPressed{Key::R});
    CHECK(g.state() == AppState::Playing);
    CHECK_EQ(g.round().ammo(), 6);            // a fresh round starts loaded
    CHECK(!g.round().reloading());
}

// ---------------------------------------------------------------------------
// Decoys
// ---------------------------------------------------------------------------

TEST_CASE("decoy: hitting one costs 20 points, counts as a shot but not a hit") {
    Round r = makeRound(ShotTuning::standard(), allDecoys());
    CHECK(r.lane().kind(2) == TargetKind::Decoy);
    const ShotResult res = r.fire(centreOf(r.lane(), 2));
    CHECK(res.outcome == ShotOutcome::Decoy);
    CHECK_EQ(r.stats().score(), -20);         // the score may go below zero
    CHECK_EQ(r.stats().decoys(), 1);
    CHECK_EQ(r.stats().hits(), 0);
    CHECK_EQ(r.stats().shots(), 1);
    CHECK_EQ(*r.stats().accuracyPercent(), 0);
    CHECK(r.lane().state(2) == DuckState::Falling);   // it still goes down
    bool sawHit = false;
    for (const auto& e : r.events()) {
        if (const auto* h = std::get_if<event::DuckHit>(&e)) {
            CHECK_EQ(h->points, -20);
            CHECK(h->kind == TargetKind::Decoy);
            sawHit = true;
        }
    }
    CHECK(sawHit);
}

TEST_CASE("decoy: the mix limits decoys on the lane, and the classic mix has none") {
    Round plain = makeRound(ShotTuning{});
    for (std::size_t i = 0; i < plain.lane().size(); ++i) {
        CHECK(plain.lane().kind(i) == TargetKind::Duck);
    }
    TargetMix m = allDecoys();
    m.maxDecoys = 2;
    Round r = makeRound(ShotTuning{}, m);
    int decoys = 0;
    for (std::size_t i = 0; i < r.lane().size(); ++i) {
        decoys += r.lane().kind(i) == TargetKind::Decoy ? 1 : 0;
    }
    CHECK_EQ(decoys, 2);
}

TEST_CASE("mix: the standard mix never starts with a golden target and always leaves ducks") {
    LaneConfig cfg = makeLaneConfig(LanePreset::Sections);
    cfg.mix = standardMix();
    const DuckShape s = boxShape();
    DuckLane lane(cfg, {s, s, s});
    for (int round = 0; round < 200; ++round) {
        lane.reset();
        int ducks = 0;
        for (std::size_t i = 0; i < lane.size(); ++i) {
            CHECK(lane.kind(i) != TargetKind::Golden);
            ducks += lane.kind(i) == TargetKind::Duck ? 1 : 0;
        }
        CHECK(ducks >= 3);
    }
}

// ---------------------------------------------------------------------------
// Golden power-up
// ---------------------------------------------------------------------------

TEST_CASE("golden: a target that rises again draws a new kind; golden gives free ammo") {
    ShotTuning tuning = ShotTuning::standard();
    Round r = makeRound(tuning, goldenOnRise(), true);
    CHECK(r.lane().kind(0) == TargetKind::Duck);       // never golden at the start
    CHECK(r.fire(centreOf(r.lane(), 0)).outcome == ShotOutcome::Hit);
    r.update(2.5);                                      // down 2 s, then rises
    CHECK(r.lane().active(0));
    CHECK(r.lane().kind(0) == TargetKind::Golden);
    // Spend some rounds first, then hit the golden target.
    missShot(r);
    missShot(r);
    CHECK_EQ(r.ammo(), 3);
    r.update(0.3);
    r.clearEvents();
    CHECK(r.fire(centreOf(r.lane(), 0)).outcome == ShotOutcome::Hit);
    CHECK_EQ(r.stats().score(), 20);                    // golden still scores
    CHECK_EQ(countEvents<event::PowerUp>(r), std::size_t{1});
    CHECK_NEAR(r.freeAmmoLeft(), tuning.powerUpTime, 1e-9);
    CHECK_NEAR(tuning.powerUpTime, 5.0, 1e-12);          // D-036: was 8 s
    CHECK_EQ(r.ammo(), 6);                              // full magazine waiting afterwards
    // Free ammo: shots do not use rounds and reload is pointless.
    for (int i = 0; i < 10; ++i) {
        CHECK(missShot(r).outcome == ShotOutcome::Miss);
    }
    CHECK_EQ(r.ammo(), 6);
    CHECK(!r.reload());
    r.clearEvents();
    r.update(8.0);
    CHECK_NEAR(r.freeAmmoLeft(), 0.0, 1e-9);
    CHECK_EQ(countEvents<event::PowerUpEnded>(r), std::size_t{1});
    missShot(r);
    CHECK_EQ(r.ammo(), 5);                              // back to counting rounds
}

TEST_CASE("golden: needs a round like any target; after a reload it still gives free ammo") {
    Round r = makeRound(ShotTuning::standard(), goldenOnRise(), true);
    r.fire(centreOf(r.lane(), 1));
    r.update(2.5);
    CHECK(r.lane().kind(1) == TargetKind::Golden);
    // shot while reloading (no shots at all), so hit it before the reload.
    for (int i = 0; i < 5; ++i) {
        missShot(r);
    }
    CHECK_EQ(r.ammo(), 0);
    r.update(0.3);
    CHECK(r.fire(centreOf(r.lane(), 1)).reason == RejectReason::Empty);   // no round for it
    CHECK(r.reload());
    r.update(1.3);
    r.update(0.3);
    CHECK(r.fire(centreOf(r.lane(), 1)).outcome == ShotOutcome::Hit);
    CHECK(r.freeAmmoLeft() > 0.0);
    CHECK(!r.reloading());
}

TEST_CASE("reset: a new round is loaded, not reloading, and without the power-up") {
    Round r = makeRound(ShotTuning::standard(), goldenOnRise(), true);
    r.fire(centreOf(r.lane(), 0));
    r.update(2.5);
    r.update(0.3);
    r.fire(centreOf(r.lane(), 0));
    CHECK(r.freeAmmoLeft() > 0.0);
    r.reset();
    CHECK_EQ(r.ammo(), 6);
    CHECK(!r.reloading());
    CHECK_NEAR(r.freeAmmoLeft(), 0.0, 1e-12);
    CHECK_EQ(r.stats().decoys(), 0);
}

// ---------------------------------------------------------------------------
// Sound
// ---------------------------------------------------------------------------

TEST_CASE("sound: dry fire, reload and power-up each have a cue from an existing recording") {
    std::vector<CueRequest> out;
    cuesForEvent(event::DryFire{{0, 0}}, out);
    cuesForEvent(event::ReloadStarted{}, out);
    cuesForEvent(event::Reloaded{}, out);
    cuesForEvent(event::PowerUp{8.0}, out);
    cuesForEvent(event::PowerUpEnded{}, out);
    CHECK_EQ(out.size(), std::size_t{4});
    CHECK(out[0].cue == Cue::DryFire);
    CHECK(out[1].cue == Cue::Reload);
    CHECK(out[2].cue == Cue::Reloaded);
    CHECK(out[3].cue == Cue::PowerUp);
    CHECK(cueSpec(Cue::DryFire).asset == AssetId::SfxUiClick);
    CHECK(cueSpec(Cue::Reload).asset == AssetId::SfxDuckReset);
    CHECK(cueSpec(Cue::PowerUp).asset == AssetId::SfxResults);
}

// ---------------------------------------------------------------------------
// D-036: harder ducks (faster, and they duck down if not hit in time)
// ---------------------------------------------------------------------------

namespace {
DuckLane laneWith(TargetMix mix) {
    LaneConfig cfg = makeLaneConfig(LanePreset::Sections);
    cfg.mix = mix;
    const DuckShape s = boxShape();
    return DuckLane(cfg, {s, s, s});
}
} // namespace

TEST_CASE("escape: an unhit duck drops by itself after 60-140 % of upTime, unscored") {
    TargetMix m;
    m.upTime = 2.0;
    DuckLane lane = laneWith(m);
    lane.update(1.19);
    for (std::size_t i = 0; i < lane.size(); ++i) {
        CHECK(lane.active(i));                    // nobody before 0.6 x upTime
    }
    lane.update(1.62);                            // t = 2.81 > 1.4 x upTime
    std::size_t escapes = 0;
    for (const DuckTransition& t : lane.transitions()) {
        if (t.from == DuckState::Upright && t.to == DuckState::Falling) {
            ++escapes;
            CHECK(t.at >= 1.2 - 1e-9);
            CHECK(t.at <= 2.8 + 1e-9);
        }
    }
    CHECK_EQ(escapes, std::size_t{5});
}

TEST_CASE("escape: decoys leave after decoyUpTime (D-037), stay without it; classic ducks never do") {
    TargetMix m = allDecoys();
    m.upTime = 1.0;                              // ducks only: decoys ignore it
    DuckLane staying = laneWith(m);
    staying.update(30.0);
    for (std::size_t i = 0; i < staying.size(); ++i) {
        CHECK(staying.active(i));
    }
    m.decoyUpTime = 2.0;
    DuckLane leaving = laneWith(m);
    leaving.update(1.19);
    CHECK(leaving.transitions().empty());          // none before 0.6 x decoyUpTime
    leaving.update(1.62);                          // t = 2.81 > 1.4 x decoyUpTime
    std::size_t escapes = 0;
    for (const DuckTransition& t : leaving.transitions()) {
        escapes += t.from == DuckState::Upright && t.to == DuckState::Falling ? 1 : 0;
    }
    CHECK_EQ(escapes, std::size_t{5});
    DuckLane classic = laneWith(TargetMix{});
    classic.update(120.0);
    CHECK(classic.transitions().empty());
}

TEST_CASE("escape: a round with escaping ducks scores nothing for them") {
    TargetMix m;
    m.upTime = 1.0;
    LaneConfig cfg = makeLaneConfig(LanePreset::Sections);
    cfg.mix = m;
    const DuckShape s = boxShape();
    Round r(DuckLane(cfg, {s, s, s}), ShotTuning::standard(), RoundTuning{.countdown = 0.0});
    r.update(3.0);
    CHECK_EQ(r.stats().score(), 0);
    CHECK_EQ(r.stats().shots(), 0);
}

TEST_CASE("speed: ordinary ducks run duckSpeed times faster; decoys keep the base speed") {
    TargetMix fast;
    fast.duckSpeed = 2.0f;
    DuckLane a = laneWith(fast);
    DuckLane b = laneWith(TargetMix{});
    a.update(0.7);
    b.update(1.4);
    for (std::size_t i = 0; i < a.size(); ++i) {
        CHECK_NEAR(a.motion(i).left, b.motion(i).left, 1e-3);
    }
    TargetMix slowDecoys = allDecoys();
    slowDecoys.duckSpeed = 2.0f;
    DuckLane c = laneWith(slowDecoys);
    DuckLane d = laneWith(TargetMix{});
    c.update(0.7);
    d.update(0.7);
    for (std::size_t i = 0; i < c.size(); ++i) {
        CHECK_NEAR(c.motion(i).left, d.motion(i).left, 1e-3);
    }
}

TEST_CASE("standard mix: rarer gold, 5 s of free ammo, faster ducks that escape") {
    const TargetMix m = standardMix();
    CHECK_NEAR(m.goldenChance, 0.05f, 1e-6);
    CHECK_NEAR(m.duckSpeed, 1.35f, 1e-6);
    CHECK_NEAR(m.upTime, 3.0, 1e-12);               // D-038 (was 3.5)
    CHECK_NEAR(m.goldenUpTime, 2.2, 1e-12);         // D-038
    CHECK_NEAR(m.upTimeRamp, 0.25f, 1e-6);          // D-038
    CHECK_NEAR(m.minUpTime, 1.3, 1e-12);            // D-038
    CHECK_NEAR(m.dropGap, 0.4, 1e-12);              // D-038
    CHECK_NEAR(m.decoyUpTime, 4.0, 1e-12);          // D-038 (D-037 had 5.0)
    CHECK_NEAR(m.goldenEvery, 15.0, 1e-12);         // D-037
    CHECK_NEAR(ShotTuning::standard().powerUpTime, 5.0, 1e-12);
}

TEST_CASE("frame rate: with the standard mix a long session is identical at 30, 60 and 144 Hz") {
    // Kinds are drawn from one shared generator, so this needs transitions
    // processed in time order across all targets (D-036).
    struct Snapshot {
        std::vector<int> kinds;
        std::vector<int> states;
        std::vector<float> lefts;
        std::size_t transitions = 0;
    };
    auto run = [](double hz) {
        DuckLane lane = laneWith(standardMix());
        std::size_t transitions = 0;
        const int frames = static_cast<int>(90.0 * hz);
        for (int f = 0; f < frames; ++f) {
            lane.update(1.0 / hz);
            // Knock down whatever is up every ~0.7 s, at the same lane times.
            const int every = static_cast<int>(std::lround(0.7 * hz));
            if (f % every == every - 1) {
                for (std::size_t i = 0; i < lane.size(); ++i) {
                    if (lane.active(i) && lane.kind(i) != TargetKind::Decoy) {
                        lane.knockDown(i);
                        break;
                    }
                }
            }
            transitions += lane.transitions().size();
            lane.clearTransitions();
        }
        Snapshot s;
        for (std::size_t i = 0; i < lane.size(); ++i) {
            s.kinds.push_back(static_cast<int>(lane.kind(i)));
            s.states.push_back(static_cast<int>(lane.state(i)));
            s.lefts.push_back(lane.motion(i).left);
        }
        s.transitions = transitions;
        return s;
    };
    // 30/60/144 Hz all divide 0.7 s windows differently, so compare without
    // knock-downs first (pure timing), then 60 vs 120 Hz with knock-downs on
    // identical lane times (every 42 vs 84 frames = exactly 0.7 s).
    auto runPlain = [](double hz) {
        DuckLane lane = laneWith(standardMix());
        for (int f = 0; f < static_cast<int>(90.0 * hz); ++f) {
            lane.update(1.0 / hz);
        }
        Snapshot s;
        for (std::size_t i = 0; i < lane.size(); ++i) {
            s.kinds.push_back(static_cast<int>(lane.kind(i)));
            s.states.push_back(static_cast<int>(lane.state(i)));
            s.lefts.push_back(lane.motion(i).left);
        }
        return s;
    };
    const Snapshot p30 = runPlain(30.0);
    for (const double hz : {60.0, 144.0}) {
        const Snapshot p = runPlain(hz);
        CHECK(p.kinds == p30.kinds);
        CHECK(p.states == p30.states);
        for (std::size_t i = 0; i < p.lefts.size(); ++i) {
            CHECK_NEAR(p.lefts[i], p30.lefts[i], 0.05);
        }
    }
    const Snapshot k60 = run(60.0);
    const Snapshot k120 = run(120.0);
    CHECK(k60.kinds == k120.kinds);
    CHECK(k60.states == k120.states);
    CHECK_EQ(k60.transitions, k120.transitions);
}

TEST_CASE("ammo duck: with no luck at all, a golden target still rises once goldenEvery has passed") {
    TargetMix m;                 // goldenChance 0: only the guarantee can make one
    m.upTime = 1.0;              // ducks keep cycling down and up
    m.goldenEvery = 10.0;
    DuckLane lane = laneWith(m);
    std::vector<Seconds> goldenAt;
    for (int f = 0; f < 60 * 45; ++f) {
        lane.update(1.0 / 60.0);
        for (const DuckTransition& t : lane.transitions()) {
            if (t.to == DuckState::Rising && lane.kind(t.id) == TargetKind::Golden) {
                goldenAt.push_back(t.at);
            }
        }
        lane.clearTransitions();
    }
    CHECK(goldenAt.size() >= 2);
    CHECK(!goldenAt.empty() && goldenAt[0] >= 10.0 - 1e-9);   // never before it is due
    for (std::size_t i = 1; i < goldenAt.size(); ++i) {
        CHECK(goldenAt[i] - goldenAt[i - 1] >= 10.0 - 1e-9);   // and then once per period
    }
    // Without the guarantee and with zero chance there is none.
    m.goldenEvery = 0.0;
    DuckLane none = laneWith(m);
    for (int f = 0; f < 60 * 45; ++f) {
        none.update(1.0 / 60.0);
        for (std::size_t i = 0; i < none.size(); ++i) {
            CHECK(none.kind(i) != TargetKind::Golden);
        }
    }
}

// ---------------------------------------------------------------------------
// D-038: pacing of how long targets stay up
// ---------------------------------------------------------------------------

namespace {
// Every unhit stay on the lane: when it stood up, how long until it left by
// itself, and as what. Nothing is ever shot.
struct Stay {
    Seconds up;
    Seconds length;
    TargetKind kind;
};
std::vector<Stay> stays(DuckLane& lane, Seconds total, double hz = 60.0) {
    std::vector<Seconds> upAt(lane.size(), 0.0);   // everyone stands at t = 0
    std::vector<TargetKind> kindOf(lane.size());
    for (std::size_t i = 0; i < lane.size(); ++i) {
        kindOf[i] = lane.kind(i);
    }
    std::vector<Stay> out;
    for (int f = 0; f < static_cast<int>(total * hz); ++f) {
        lane.update(1.0 / hz);
        for (const DuckTransition& t : lane.transitions()) {
            if (t.to == DuckState::Rising) {
                kindOf[t.id] = lane.kind(t.id);
            } else if (t.to == DuckState::Upright) {
                upAt[t.id] = t.at;
            } else if (t.from == DuckState::Upright && t.to == DuckState::Falling) {
                out.push_back({upAt[t.id], t.at - upAt[t.id], kindOf[t.id]});
            }
        }
        lane.clearTransitions();
    }
    return out;
}
} // namespace

TEST_CASE("pacing: with the standard mix no two targets leave within 0.4 s of each other") {
    DuckLane lane = laneWith(standardMix());
    std::vector<Seconds> leaves;
    for (const Stay& s : stays(lane, 120.0)) {
        leaves.push_back(s.up + s.length);
    }
    CHECK(leaves.size() > 40);
    std::size_t close = 0;
    for (std::size_t i = 1; i < leaves.size(); ++i) {
        close += leaves[i] - leaves[i - 1] < 0.4 - 1e-9 ? 1 : 0;
    }
    // A clash is allowed only when the whole window is crowded (16 tries all
    // fail); with five targets and windows of seconds that should be rare.
    CHECK(close * 20 <= leaves.size());
}

TEST_CASE("pacing: durations differ, stay inside the window, respect the floor, and shrink late") {
    TargetMix m = standardMix();
    m.decoyChance = 0.0f;   // ducks and gold only
    DuckLane lane = laneWith(m);
    const std::vector<Stay> all = stays(lane, 60.0);
    CHECK(all.size() > 25);
    double earlyMax = 0.0;
    double lateMax = 0.0;
    std::vector<long long> distinct;
    for (const Stay& s : all) {
        const double base = s.kind == TargetKind::Golden ? 2.2 : 3.0;
        const double ramp = std::min(0.4, 0.25 * std::max(0.0, s.up - 0.25) / 60.0);
        CHECK(s.length >= 1.3 - 1e-6);                                   // fair floor
        CHECK(s.length <= std::max(1.3, base * 1.4 * (1.0 - ramp)) + 1e-6);
        if (s.kind == TargetKind::Duck && s.up < 15.0) {
            earlyMax = std::max(earlyMax, s.length);
        } else if (s.kind == TargetKind::Duck && s.up > 45.0) {
            lateMax = std::max(lateMax, s.length);
        }
        distinct.push_back(std::llround(s.length * 100.0));
    }
    std::sort(distinct.begin(), distinct.end());
    const auto unique = static_cast<std::size_t>(std::unique(distinct.begin(), distinct.end()) - distinct.begin());
    CHECK(unique * 10 >= all.size() * 7);    // at least 70 % different to the hundredth of a second
    CHECK(lateMax <= 3.0 * 1.4 * (1.0 - 0.25 * 45.0 / 60.0) + 1e-6);   // late windows are shorter
    CHECK(earlyMax > 0.0);
}

TEST_CASE("pacing: gold ammo ducks are fleeting, decoys linger longer than ducks") {
    DuckLane lane = laneWith(standardMix());
    double goldMax = 0.0;
    double decoyMin = 1e9;
    for (const Stay& s : stays(lane, 240.0)) {
        if (s.kind == TargetKind::Golden) {
            goldMax = std::max(goldMax, s.length);
        } else if (s.kind == TargetKind::Decoy) {
            decoyMin = std::min(decoyMin, s.length);
        }
    }
    CHECK(goldMax > 0.0);
    CHECK(goldMax <= 2.2 * 1.4 + 1e-6);   // 3.08 s at most
    CHECK(decoyMin >= 4.0 * 0.6 - 1e-6);  // 2.4 s at least
}
