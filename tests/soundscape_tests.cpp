// Soundscape rules (D-030): which recording plays at which game moment, the
// mix, the beds per screen, player volume, and per-play variation. The last
// group drives a real Game through whole rounds in the same frame order as
// Application::run and checks the complete cue sequence it produces.

#include "support/test_framework.hpp"
#include "support/test_round.hpp"

#include "gallery/audio/Soundscape.hpp"
#include "gallery/flow/Game.hpp"
#include "gallery/flow/UiLayout.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace gallery;
using gallery::test::centreOf;
using gallery::test::kOpenBackboard;
using gallery::test::makeTimedRound;

namespace {

constexpr Seconds kFrame = 1.0 / 60.0;

std::vector<Cue> cuesOf(const GameEvent& e) {
    std::vector<CueRequest> r;
    cuesForEvent(e, r);
    std::vector<Cue> out;
    for (const auto& c : r) {
        out.push_back(c.cue);
    }
    return out;
}

// What the shell's AudioSystem would be asked to play, frame by frame, in
// Application::run order: input (+ UI click), update, events + transitions.
struct Recorder {
    Game game{makeTimedRound()};
    struct Played {
        Cue cue;
        Seconds delay;
        int frame;
    };
    std::vector<Played> cues;
    int frame = 0;

    void input(const InputEvent& e) {
        const AppState before = game.state();
        (void)game.handle(e);
        if (isUiAction(before, game.state(), e)) {
            cues.push_back({Cue::UiClick, 0.0, frame});
        }
    }
    void step(Seconds dt = kFrame) {
        game.update(dt);
        std::vector<CueRequest> r;
        for (const auto& e : game.events()) {
            cuesForEvent(e, r);
        }
        for (const auto& t : game.round().lane().transitions()) {
            cuesForTransition(t, r);
        }
        for (const auto& c : r) {
            cues.push_back({c.cue, c.delay, frame});
        }
        game.round().lane().clearTransitions();
        game.clearEvents();
        ++frame;
    }
    void run(Seconds seconds) {
        const int n = static_cast<int>(std::lround(seconds / kFrame));
        for (int i = 0; i < n; ++i) {
            step();
        }
    }
    [[nodiscard]] std::size_t count(Cue c) const {
        return static_cast<std::size_t>(
            std::count_if(cues.begin(), cues.end(), [c](const Played& p) { return p.cue == c; }));
    }
    [[nodiscard]] std::vector<Cue> since(std::size_t from) const {
        std::vector<Cue> out;
        for (std::size_t i = from; i < cues.size(); ++i) {
            out.push_back(cues[i].cue);
        }
        return out;
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Table and mapping
// ---------------------------------------------------------------------------

TEST_CASE("soundscape: cue table order matches the enum, assets are sounds, mix is sane") {
    for (std::size_t i = 0; i < kCueCount; ++i) {
        const CueSpec& s = kCueTable[i];
        CHECK_EQ(static_cast<std::size_t>(s.cue), i);
        CHECK(assetEntry(s.asset).kind == AssetKind::Sound);
        CHECK(s.gain > 0.0f && s.gain <= 1.0f);
        CHECK(s.pitchJitter >= 0.0f && s.pitchJitter <= 0.08f);   // subtle, never comic
        CHECK(s.gainJitter >= 0.0f && s.gainJitter <= 0.15f);
        CHECK(s.bus == Bus::Effects || s.bus == Bus::Interface);
    }
    // The shot leads the mix; mechanical detail sits under the impacts.
    CHECK(cueSpec(Cue::Shot).gain >= cueSpec(Cue::DuckHit).gain);
    CHECK(cueSpec(Cue::DuckHit).gain > cueSpec(Cue::DuckFlip).gain);
    CHECK(cueSpec(Cue::DuckHit).gain > cueSpec(Cue::ScoreTick).gain);
    // Clock-like cues never vary: a tick that wobbles in pitch sounds wrong.
    CHECK_EQ(cueSpec(Cue::Tick).pitchJitter, 0.0f);
    CHECK_EQ(cueSpec(Cue::BellStart).pitchJitter, 0.0f);
    CHECK_EQ(cueSpec(Cue::EndBell).pitchJitter, 0.0f);
}

TEST_CASE("soundscape: each game event maps to its sounds") {
    CHECK(cuesOf(event::RoundReset{}).empty());   // restart is silent and instant
    CHECK(cuesOf(event::Go{}) == std::vector<Cue>{Cue::BellStart});
    CHECK(cuesOf(event::ShotFired{{1, 2}}) == std::vector<Cue>{Cue::Shot});
    CHECK((cuesOf(event::DuckHit{2, {1, 2}, 10}) == std::vector<Cue>{Cue::DuckHit, Cue::ScoreTick}));
    CHECK(cuesOf(event::ShotMissed{{1, 2}}) == std::vector<Cue>{Cue::Miss});
    CHECK(cuesOf(event::LowTimeTick{7}) == std::vector<Cue>{Cue::Tick});
    CHECK((cuesOf(event::TimeUp{}) == std::vector<Cue>{Cue::EndBell, Cue::EndBell}));
    CHECK(cuesOf(event::RoundFinished{RoundStats{}}) == std::vector<Cue>{Cue::Results});
}

TEST_CASE("soundscape: delays: the score clack trails the ping, the end bell rings twice") {
    std::vector<CueRequest> r;
    cuesForEvent(event::DuckHit{0, {0, 0}, 10}, r);
    CHECK_EQ(r[0].delay, 0.0);                 // impact in the frame of the click (G-9)
    CHECK_NEAR(r[1].delay, kScoreTickDelay, 1e-12);
    CHECK(r[1].delay > 0.0 && r[1].delay < 0.2);
    r.clear();
    cuesForEvent(event::TimeUp{}, r);
    CHECK_EQ(r[0].delay, 0.0);
    CHECK_NEAR(r[1].delay, kEndBellRepeat, 1e-12);
    r.clear();
    cuesForEvent(event::ShotFired{{0, 0}}, r);
    CHECK_EQ(r[0].delay, 0.0);                 // the shot is never delayed
}

TEST_CASE("soundscape: target transitions: slam at the end of the fall, rattle as it rises") {
    std::vector<CueRequest> r;
    cuesForTransition({0, DuckState::Upright, DuckState::Falling, 1.0}, r);
    CHECK(r.empty());   // the hit itself is voiced by DuckHit
    cuesForTransition({0, DuckState::Falling, DuckState::Down, 1.2}, r);
    cuesForTransition({0, DuckState::Down, DuckState::Rising, 4.0}, r);
    cuesForTransition({0, DuckState::Rising, DuckState::Upright, 4.25}, r);
    CHECK_EQ(r.size(), 2u);
    CHECK(r[0].cue == Cue::DuckFlip);
    CHECK(r[1].cue == Cue::DuckReset);
}

TEST_CASE("soundscape: UI clicks only for presses and keys that change the screen") {
    const InputEvent press = input::PrimaryPressed{{0, 0}};
    const InputEvent key = input::KeyPressed{Key::Enter};
    CHECK(isUiAction(AppState::Title, AppState::Playing, press));
    CHECK(isUiAction(AppState::Results, AppState::Playing, key));
    CHECK(isUiAction(AppState::Playing, AppState::Paused, input::KeyPressed{Key::Escape}));
    CHECK(isUiAction(AppState::Paused, AppState::Title, press));
    CHECK(!isUiAction(AppState::Playing, AppState::Playing, press));    // a shot, not a click
    CHECK(!isUiAction(AppState::Title, AppState::Exiting, key));        // quitting: no click
    CHECK(!isUiAction(AppState::Playing, AppState::Paused, input::FocusLost{}));
    CHECK(!isUiAction(AppState::Paused, AppState::Exiting, input::CloseRequested{}));
    CHECK(!isUiAction(AppState::Title, AppState::Playing, input::PointerMoved{{0, 0}}));
}

// ---------------------------------------------------------------------------
// Beds, fader, mixer, variation
// ---------------------------------------------------------------------------

TEST_CASE("soundscape: the target mechanism runs exactly while the targets move") {
    CHECK_EQ(bedLevels(AppState::Playing, RoundPhase::Countdown, true).track, 1.0f);
    CHECK_EQ(bedLevels(AppState::Playing, RoundPhase::Live, true).track, 1.0f);
    CHECK_EQ(bedLevels(AppState::Playing, RoundPhase::Ending, true).track, 0.0f);
    CHECK_EQ(bedLevels(AppState::Playing, RoundPhase::Finished, true).track, 0.0f);
    CHECK_EQ(bedLevels(AppState::Paused, RoundPhase::Live, true).track, 0.0f);
    CHECK_EQ(bedLevels(AppState::Title, RoundPhase::Countdown, true).track, 0.0f);
    CHECK_EQ(bedLevels(AppState::Results, RoundPhase::Finished, true).track, 0.0f);
    CHECK_EQ(bedLevels(AppState::Playing, RoundPhase::Live, false).track, 0.0f);
}

TEST_CASE("soundscape: ambience never stops on a board, settles back when paused or unfocused") {
    for (const AppState s : {AppState::Title, AppState::Playing, AppState::Paused, AppState::Results}) {
        CHECK(bedLevels(s, RoundPhase::Live, true).ambience > 0.0f);
        CHECK(bedLevels(s, RoundPhase::Live, false).ambience > 0.0f);
        CHECK(bedLevels(s, RoundPhase::Live, false).ambience <= 0.35f);
    }
    CHECK(bedLevels(AppState::Paused, RoundPhase::Live, true).ambience <
          bedLevels(AppState::Playing, RoundPhase::Live, true).ambience);
    CHECK(bedLevels(AppState::Playing, RoundPhase::Live, true).music <
          bedLevels(AppState::Title, RoundPhase::Countdown, true).music);
    const BedLevels off = bedLevels(AppState::Exiting, RoundPhase::Live, true);
    CHECK_EQ(off.ambience + off.track + off.music, 0.0f);
}

TEST_CASE("soundscape: fader ramps linearly, never overshoots, ignores bad dt") {
    Fader f(0.5);
    CHECK_EQ(f.value(), 0.0f);
    f.step(1.0f, 0.25);
    CHECK_NEAR(f.value(), 0.5f, 1e-6);
    f.step(1.0f, 10.0);
    CHECK_EQ(f.value(), 1.0f);
    f.step(0.2f, 0.1);
    CHECK_NEAR(f.value(), 0.8f, 1e-6);
    f.step(0.2f, 1.0);
    CHECK_NEAR(f.value(), 0.2f, 1e-6);
    f.step(1.0f, -1.0);
    f.step(1.0f, std::nan(""));
    CHECK_NEAR(f.value(), 0.2f, 1e-6);
    f.step(7.0f, 100.0);   // targets clamp to [0, 1]
    CHECK_EQ(f.value(), 1.0f);
    f.snap(-3.0f);
    CHECK_EQ(f.value(), 0.0f);
}

TEST_CASE("soundscape: mixer volume steps, clamps, mutes, and switches music") {
    Mixer m;
    CHECK_EQ(m.masterPercent(), 80);
    CHECK(m.nudge(+1));
    CHECK(m.nudge(+1));
    CHECK(!m.nudge(+1));   // already at 100 %
    CHECK_EQ(m.masterPercent(), 100);
    CHECK_NEAR(m.gain(Bus::Effects), 1.0f, 1e-6);
    m.setMasterPercent(54);
    CHECK_EQ(m.masterPercent(), 50);   // nearest step
    CHECK_NEAR(m.gain(Bus::Effects), 0.25f, 1e-6);   // perceptual: 0.5 squared
    m.setMasterPercent(-20);
    CHECK_EQ(m.masterStep(), 0);
    CHECK_EQ(m.gain(Bus::Effects), 0.0f);
    CHECK(!m.nudge(-1));
    m.setMasterPercent(250);
    CHECK_EQ(m.masterPercent(), 100);

    m.toggleMute();
    for (std::size_t b = 0; b < kBusCount; ++b) {
        CHECK_EQ(m.gain(static_cast<Bus>(b)), 0.0f);
    }
    m.toggleMute();
    CHECK(m.gain(Bus::Music) > 0.0f);
    m.toggleMusic();
    CHECK_EQ(m.gain(Bus::Music), 0.0f);
    CHECK(m.gain(Bus::Ambience) > 0.0f);   // music off leaves the booth sound alone
    // Beds sit under the effects so a shot is always clear.
    CHECK(m.busTrim(Bus::Ambience) < m.busTrim(Bus::Effects));
    CHECK(m.busTrim(Bus::Music) < m.busTrim(Bus::Effects));
}

TEST_CASE("soundscape: variation is deterministic and stays within each cue's jitter") {
    CueVariation a, b;
    for (int i = 0; i < 2000; ++i) {
        for (const CueSpec& s : kCueTable) {
            const PlayParams p = a.next(s);
            const PlayParams q = b.next(s);
            CHECK_EQ(p.pitch, q.pitch);
            CHECK_EQ(p.gain, q.gain);
            CHECK(std::fabs(p.pitch - 1.0f) <= s.pitchJitter + 1e-6f);
            CHECK(p.gain >= s.gain * (1.0f - s.gainJitter) - 1e-6f);
            CHECK(p.gain <= std::min(1.0f, s.gain * (1.0f + s.gainJitter)) + 1e-6f);
        }
    }
    // Repeated shots really do differ.
    CueVariation c;
    const float p1 = c.next(cueSpec(Cue::Shot)).pitch;
    const float p2 = c.next(cueSpec(Cue::Shot)).pitch;
    CHECK(p1 != p2);
}

// ---------------------------------------------------------------------------
// Whole rounds, in Application frame order
// ---------------------------------------------------------------------------

TEST_CASE("soundscape: a full round produces the right sounds in the right frames") {
    Recorder r;
    r.input(input::KeyPressed{Key::Enter});        // Title -> Playing
    CHECK(r.cues.size() == 1 && r.cues[0].cue == Cue::UiClick);

    // Countdown: silent until Go, which rings the bell exactly once.
    r.run(1.4);
    CHECK_EQ(r.count(Cue::BellStart), 0u);
    r.run(0.2);
    CHECK_EQ(r.count(Cue::BellStart), 1u);

    // A miss: shot then miss, same frame as the click.
    std::size_t mark = r.cues.size();
    r.input(input::PrimaryPressed{kOpenBackboard});
    r.step();
    CHECK((r.since(mark) == std::vector<Cue>{Cue::Shot, Cue::Miss}));
    CHECK_EQ(r.cues[mark].frame, r.cues[mark + 1].frame);

    // A click inside the cooldown is not a shot: no sound at all.
    mark = r.cues.size();
    r.input(input::PrimaryPressed{kOpenBackboard});
    r.step();
    CHECK(r.since(mark).empty());

    // A hit: shot, ping, delayed score clack; then the slam 0.2 s later and,
    // once the target re-enters, the reset rattle.
    r.run(0.3);
    mark = r.cues.size();
    const Vec2 target = centreOf(r.game.round().lane(), 2);
    r.input(input::PrimaryPressed{target});
    r.step();
    CHECK((r.since(mark) == std::vector<Cue>{Cue::Shot, Cue::DuckHit, Cue::ScoreTick}));
    CHECK(r.cues[mark + 2].delay > 0.0);
    r.run(0.3);
    CHECK_EQ(r.count(Cue::DuckFlip), 1u);
    r.run(8.0);
    CHECK_EQ(r.count(Cue::DuckReset), 1u);

    // Final ten seconds: exactly ten ticks; then the double end bell, the
    // results ring-up one second later, and no more mechanism cues.
    for (int guard = 0; r.game.state() == AppState::Playing && guard < 60 * 70; ++guard) {
        r.step();
    }
    CHECK_EQ(r.count(Cue::Tick), 10u);
    // The end bell comes after the last tick, and the results after the bell.
    std::size_t lastTick = 0, firstEnd = 0, results = 0;
    for (std::size_t i = 0; i < r.cues.size(); ++i) {
        if (r.cues[i].cue == Cue::Tick) lastTick = i;
        if (r.cues[i].cue == Cue::EndBell && firstEnd == 0) firstEnd = i;
        if (r.cues[i].cue == Cue::Results) results = i;
    }
    CHECK(lastTick < firstEnd && firstEnd < results);
    // Results ring up about 1 s (60 frames) after time up (G-28).
    CHECK(r.cues[results].frame - r.cues[firstEnd].frame >= 59);
    CHECK(r.cues[results].frame - r.cues[firstEnd].frame <= 61);
    CHECK_EQ(r.count(Cue::EndBell), 2u);
    CHECK_EQ(r.count(Cue::Results), 1u);
    CHECK(r.game.state() == AppState::Results);
    CHECK_EQ(r.count(Cue::Shot), 2u);   // exactly the accepted shots
    CHECK_EQ(r.count(Cue::Miss), 1u);
    CHECK_EQ(r.count(Cue::DuckHit), 1u);
}

TEST_CASE("soundscape: pause is silent of game cues; resume and restart click once each") {
    Recorder r;
    r.input(input::KeyPressed{Key::Enter});
    r.run(3.0);
    std::size_t mark = r.cues.size();
    r.input(input::FocusLost{});                    // pauses, but focus loss is not a click
    CHECK(r.game.state() == AppState::Paused);
    r.run(20.0);                                    // frozen: no ticks, no bells, no targets
    CHECK(r.since(mark).empty());
    r.input(input::FocusGained{});
    CHECK(r.since(mark).empty());
    r.input(input::KeyPressed{Key::Space});         // resume
    CHECK((r.since(mark) == std::vector<Cue>{Cue::UiClick}));

    mark = r.cues.size();
    r.input(input::KeyPressed{Key::Escape});        // pause (a click)
    r.input(input::KeyPressed{Key::R});             // restart (a click)
    r.step();                                       // RoundReset: silent
    CHECK((r.since(mark) == std::vector<Cue>{Cue::UiClick, Cue::UiClick}));
    r.run(1.6);
    CHECK_EQ(r.count(Cue::BellStart), 2u);          // the new round rings its own Go
}

TEST_CASE("soundscape: ten back-to-back rounds give identical soundtracks") {
    Recorder r;
    std::vector<std::size_t> perRound;
    for (int round = 0; round < 10; ++round) {
        const std::size_t before = r.cues.size();
        // Start (Enter on the title) or play again (R on the results): one click each.
        r.input(input::KeyPressed{round == 0 ? Key::Enter : Key::R});
        r.run(1.6);
        r.input(input::PrimaryPressed{kOpenBackboard});
        r.run(62.0);
        CHECK(r.game.state() == AppState::Results);
        perRound.push_back(r.cues.size() - before);
    }
    CHECK_EQ(r.count(Cue::UiClick), 10u);
    for (const std::size_t n : perRound) {
        CHECK_EQ(n, perRound.front());
    }
    CHECK_EQ(r.count(Cue::Results), 10u);
    CHECK_EQ(r.count(Cue::EndBell), 20u);
    CHECK_EQ(r.count(Cue::Tick), 100u);
}
