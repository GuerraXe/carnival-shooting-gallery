// Application flow (ARCHITECTURE §4.1, §7.2 "Flow", "Restart", "Pause"):
// every transition, keys that must do nothing, clicks that must never be
// shots, frozen time while paused, and clean restarts.

#include "support/test_framework.hpp"
#include "support/test_round.hpp"

#include "gallery/flow/Game.hpp"
#include "gallery/flow/UiLayout.hpp"

#include <cmath>
#include <vector>

using namespace gallery;
using gallery::test::centreOf;
using gallery::test::countEvents;
using gallery::test::kOpenBackboard;
using gallery::test::makeTimedRound;

namespace {

constexpr Seconds kFrame = 1.0 / 60.0;

Game makeGame(ShotTuning shot = {}) {
    return Game(makeTimedRound(RoundTuning{}, shot));
}

Vec2 centre(Rect r) {
    return {r.x + r.width / 2.0f, r.y + r.height / 2.0f};
}

Vec2 buttonAt(AppState s, ButtonId id) {
    for (const Button& b : ui::buttons(s)) {
        if (b.id == id) {
            return centre(b.rect);
        }
    }
    throw gallery::test::Failure{"no such button on this screen"};
}

void press(Game& g, Key k) {
    g.handle(input::KeyPressed{k});
}

std::optional<ShotResult> click(Game& g, Vec2 at) {
    return g.handle(input::PrimaryPressed{at});
}

// Run frames, clearing events like the shell does; returns events seen.
std::vector<GameEvent> run(Game& g, Seconds seconds, Seconds dt = kFrame) {
    std::vector<GameEvent> seen;
    const long long frames = std::llround(seconds / dt);
    for (long long f = 0; f < frames; ++f) {
        g.update(dt);
        seen.insert(seen.end(), g.events().begin(), g.events().end());
        g.clearEvents();
    }
    return seen;
}

// Start a round from the title and get past "Ready... Go!".
void startLive(Game& g) {
    press(g, Key::Enter);
    g.clearEvents();
    run(g, 1.5 + kFrame);
    if (!g.round().clock().acceptsShots()) {
        throw gallery::test::Failure{"round did not go live"};
    }
}

void playToResults(Game& g) {
    run(g, 70.0);
    if (g.state() != AppState::Results) {
        throw gallery::test::Failure{"round did not reach results"};
    }
}

} // namespace

// ---------------------------------------------------------------------------
// UiLayout
// ---------------------------------------------------------------------------

TEST_CASE("ui: every screen's placards sit inside the board and do not overlap") {
    for (const AppState s : {AppState::Title, AppState::Paused, AppState::Results}) {
        const auto bs = ui::buttons(s);
        CHECK(!bs.empty());
        for (std::size_t i = 0; i < bs.size(); ++i) {
            const Rect r = bs[i].rect;
            CHECK(r.x >= ui::kPanel.x && r.x + r.width <= ui::kPanel.x + ui::kPanel.width);
            CHECK(r.y >= ui::kPanel.y && r.y + r.height <= ui::kPanel.y + ui::kPanel.height);
            CHECK(ui::hit(s, centre(r)) == std::optional<ButtonId>{bs[i].id});
            for (std::size_t j = i + 1; j < bs.size(); ++j) {
                CHECK(!overlaps(r, bs[j].rect));
            }
        }
    }
    CHECK(ui::buttons(AppState::Playing).empty());
    CHECK(!ui::hit(AppState::Playing, centre(ui::kPanel)).has_value());
}

// ---------------------------------------------------------------------------
// Title (main menu)
// ---------------------------------------------------------------------------

TEST_CASE("flow: boots to the title with a clean, frozen round") {
    Game g = makeGame();
    CHECK(g.state() == AppState::Title);
    CHECK(g.round().clock().phase() == RoundPhase::Countdown);
    const float x = g.round().lane().motion(0).left;
    run(g, 5.0);
    CHECK_NEAR(g.round().lane().motion(0).left, x, 0.0);        // nothing moves on the title
    CHECK_NEAR(g.round().clock().phaseTime(), 0.0, 0.0);
}

TEST_CASE("flow: title -> playing by Start placard, Space or Enter") {
    for (int way = 0; way < 3; ++way) {
        Game g = makeGame();
        if (way == 0) {
            CHECK(!click(g, buttonAt(AppState::Title, ButtonId::StartRound)).has_value());
        } else {
            press(g, way == 1 ? Key::Space : Key::Enter);
        }
        CHECK(g.state() == AppState::Playing);
        CHECK_EQ(g.roundsStarted(), 1);
        CHECK_EQ(countEvents<event::RoundReset>(g.events()), std::size_t{1});
    }
}

TEST_CASE("flow: title clicks off the placards, R and P do nothing; never a shot") {
    Game g = makeGame();
    CHECK(!click(g, centreOf(g.round().lane(), 0)).has_value());
    CHECK(!click(g, kOpenBackboard).has_value());
    press(g, Key::R);
    press(g, Key::P);
    CHECK(g.state() == AppState::Title);
    CHECK_EQ(g.round().stats().shots(), 0);
    CHECK_EQ(countEvents<event::ShotFired>(g.events()), std::size_t{0});
}

TEST_CASE("flow: title -> exiting by Esc, by the Quit placard, by closing the window") {
    Game a = makeGame();
    press(a, Key::Escape);
    CHECK(a.state() == AppState::Exiting);
    Game b = makeGame();
    click(b, buttonAt(AppState::Title, ButtonId::Quit));
    CHECK(b.state() == AppState::Exiting);
    Game c = makeGame();
    c.handle(input::CloseRequested{});
    CHECK(c.state() == AppState::Exiting);
    // Exiting is final.
    press(c, Key::Enter);
    c.update(1.0);
    CHECK(c.state() == AppState::Exiting);
}

// ---------------------------------------------------------------------------
// Playing
// ---------------------------------------------------------------------------

TEST_CASE("flow: during a round clicks are trigger pulls; Space, Enter, R do nothing") {
    Game g = makeGame();
    startLive(g);
    press(g, Key::Space);
    press(g, Key::Enter);
    press(g, Key::R);
    CHECK(g.state() == AppState::Playing);
    const auto shot = click(g, centreOf(g.round().lane(), 1));
    CHECK(shot.has_value());
    CHECK(shot->outcome == ShotOutcome::Hit);
    CHECK_EQ(g.round().stats().score(), 10);   // score updates immediately, before any update
    CHECK_EQ(g.round().stats().shots(), 1);
}

TEST_CASE("flow: the timer runs only while playing and the round ends in Results") {
    Game g = makeGame();
    press(g, Key::Enter);
    g.clearEvents();
    run(g, 1.5 + 30.0);
    CHECK_EQ(g.round().clock().displaySeconds(), 30);
    run(g, 29.0 + kFrame);
    CHECK(g.round().clock().lowTime());
    run(g, 1.0);
    CHECK(g.round().clock().phase() == RoundPhase::Ending);    // time up, ducks stopped
    CHECK(g.state() == AppState::Playing);                     // results ~1 s later (G-28)
    press(g, Key::Escape);                                     // input disabled after zero
    CHECK(g.state() == AppState::Playing);
    run(g, 1.0 + kFrame);
    CHECK(g.state() == AppState::Results);
}

// ---------------------------------------------------------------------------
// Paused
// ---------------------------------------------------------------------------

TEST_CASE("flow: Esc, P and focus loss pause; clock and targets freeze (AC-15)") {
    for (int way = 0; way < 3; ++way) {
        Game g = makeGame();
        startLive(g);
        run(g, 10.0);
        if (way == 0) {
            press(g, Key::Escape);
        } else if (way == 1) {
            press(g, Key::P);
        } else {
            g.handle(input::FocusLost{});
        }
        CHECK(g.state() == AppState::Paused);
        const Seconds left = g.round().clock().remaining();
        const float x = g.round().lane().motion(3).left;
        run(g, 20.0);
        CHECK_NEAR(g.round().clock().remaining(), left, 0.0);
        CHECK_NEAR(g.round().lane().motion(3).left, x, 0.0);
        g.handle(input::FocusGained{});
        CHECK(g.state() == AppState::Paused);                  // never auto-resumes (G-29)
    }
}

TEST_CASE("flow: the countdown can be paused too, and resumes where it was") {
    Game g = makeGame();
    press(g, Key::Enter);
    run(g, 0.5);
    press(g, Key::Escape);
    run(g, 5.0);
    CHECK(g.round().clock().phase() == RoundPhase::Countdown);
    CHECK_NEAR(g.round().clock().countdownRemaining(), 1.0, 1e-6);
    press(g, Key::Space);
    CHECK(g.state() == AppState::Playing);
}

TEST_CASE("flow: the resume click is not a shot and loses no time (AC-15)") {
    Game g = makeGame();
    startLive(g);
    run(g, 5.0);
    const int shotsBefore = g.round().stats().shots();
    const Seconds left = g.round().clock().remaining();
    press(g, Key::Escape);
    // A click on the open booth (not on a placard) resumes.
    const auto r = click(g, centreOf(g.round().lane(), 0));
    CHECK(!r.has_value());
    CHECK(g.state() == AppState::Playing);
    CHECK_EQ(g.round().stats().shots(), shotsBefore);
    CHECK_EQ(countEvents<event::ShotFired>(g.events()), std::size_t{0});
    CHECK_NEAR(g.round().clock().remaining(), left, 0.0);
    // The very next click is a real shot.
    const auto s = click(g, kOpenBackboard);
    CHECK(s.has_value());
    CHECK(s->outcome == ShotOutcome::Miss);
}

TEST_CASE("flow: paused placards and keys: Resume, Restart Round, Main Menu, Space/Enter/P, R, Esc") {
    struct Way {
        bool isClick;
        ButtonId button;
        Key key;
        AppState expect;
        bool fresh;   // a new round started
    };
    const Way ways[] = {
        {true, ButtonId::Resume, Key::Space, AppState::Playing, false},
        {true, ButtonId::RestartRound, Key::Space, AppState::Playing, true},
        {true, ButtonId::MainMenu, Key::Space, AppState::Title, false},
        {false, ButtonId::Resume, Key::Space, AppState::Playing, false},
        {false, ButtonId::Resume, Key::Enter, AppState::Playing, false},
        {false, ButtonId::Resume, Key::P, AppState::Playing, false},
        {false, ButtonId::Resume, Key::R, AppState::Playing, true},
        {false, ButtonId::Resume, Key::Escape, AppState::Title, false},
    };
    for (const Way& w : ways) {
        Game g = makeGame();
        startLive(g);
        click(g, kOpenBackboard);                              // one shot on the books
        run(g, 3.0);
        press(g, Key::Escape);
        g.clearEvents();
        if (w.isClick) {
            CHECK(!click(g, buttonAt(AppState::Paused, w.button)).has_value());
        } else {
            press(g, w.key);
        }
        CHECK(g.state() == w.expect);
        if (w.expect == AppState::Playing) {
            CHECK_EQ(g.round().stats().shots(), w.fresh ? 0 : 1);
            CHECK_EQ(g.roundsStarted(), w.fresh ? 2 : 1);
            CHECK(g.round().clock().phase() ==
                  (w.fresh ? RoundPhase::Countdown : RoundPhase::Live));
        } else {
            // Main menu: the abandoned round is discarded, not scored.
            CHECK_EQ(g.round().stats().shots(), 0);
            CHECK(!g.best().best().has_value());
        }
    }
}

// ---------------------------------------------------------------------------
// Results
// ---------------------------------------------------------------------------

TEST_CASE("flow: results hold the round's actual statistics; clicks there are not shots") {
    Game g = makeGame(ShotTuning{.fireCooldown = 0.0});
    startLive(g);
    click(g, centreOf(g.round().lane(), 0));                   // hit
    click(g, centreOf(g.round().lane(), 4));                   // hit
    click(g, kOpenBackboard);                                  // miss
    playToResults(g);
    const RoundStats& s = g.round().stats();
    CHECK_EQ(s.score(), 20);
    CHECK_EQ(s.shots(), 3);
    CHECK_EQ(s.hits(), 2);
    CHECK_EQ(s.misses(), 1);
    CHECK_EQ(*s.accuracyPercent(), 67);
    CHECK_EQ(*g.best().best(), 20);
    CHECK(!g.newBest());                                       // first round: nothing to beat
    CHECK(!click(g, kOpenBackboard).has_value());
    CHECK(!click(g, centreOf(g.round().lane(), 1)).has_value());
    CHECK_EQ(g.round().stats().shots(), 3);
    run(g, 5.0);
    CHECK(g.state() == AppState::Results);                     // results stay until asked
}

TEST_CASE("flow: results -> playing by Play Again, R, Enter, Space; -> title; Esc quits") {
    struct Way {
        bool isClick;
        ButtonId button;
        Key key;
        AppState expect;
    };
    const Way ways[] = {
        {true, ButtonId::PlayAgain, Key::Space, AppState::Playing},
        {false, ButtonId::PlayAgain, Key::R, AppState::Playing},
        {false, ButtonId::PlayAgain, Key::Enter, AppState::Playing},
        {false, ButtonId::PlayAgain, Key::Space, AppState::Playing},
        {true, ButtonId::MainMenu, Key::Space, AppState::Title},
        {false, ButtonId::PlayAgain, Key::Escape, AppState::Exiting},
        {false, ButtonId::PlayAgain, Key::P, AppState::Results},  // P does nothing here
    };
    for (const Way& w : ways) {
        Game g = makeGame();
        startLive(g);
        playToResults(g);
        if (w.isClick) {
            click(g, buttonAt(AppState::Results, w.button));
        } else {
            press(g, w.key);
        }
        CHECK(g.state() == w.expect);
    }
}

TEST_CASE("flow: session best and 'new best' across rounds") {
    Game g = makeGame(ShotTuning{.fireCooldown = 0.0});
    startLive(g);
    click(g, centreOf(g.round().lane(), 0));
    playToResults(g);                                          // 10
    CHECK(!g.newBest());
    press(g, Key::R);
    run(g, 1.5 + kFrame);
    playToResults(g);                                          // 0
    CHECK(!g.newBest());
    CHECK_EQ(*g.best().best(), 10);
    press(g, Key::R);
    run(g, 1.5 + kFrame);
    click(g, centreOf(g.round().lane(), 1));
    click(g, centreOf(g.round().lane(), 2));
    playToResults(g);                                          // 20
    CHECK(g.newBest());
    CHECK_EQ(*g.best().best(), 20);
    press(g, Key::R);
    CHECK(!g.newBest());                                       // cleared with the new round
}

// ---------------------------------------------------------------------------
// Restart: ten back-to-back rounds, no carry-over (AC-13)
// ---------------------------------------------------------------------------

TEST_CASE("restart: ten rounds in a row each start identical and clean") {
    Game fresh = makeGame();
    press(fresh, Key::Enter);
    std::vector<float> startX;
    for (std::size_t i = 0; i < fresh.round().lane().size(); ++i) {
        startX.push_back(fresh.round().lane().motion(i).left);
    }

    Game g = makeGame(ShotTuning{.fireCooldown = 0.0});
    for (int round = 1; round <= 10; ++round) {
        if (round == 1) {
            press(g, Key::Enter);
        } else if (round % 3 == 0) {
            click(g, buttonAt(AppState::Results, ButtonId::PlayAgain));
        } else {
            press(g, round % 2 ? Key::R : Key::Enter);
        }
        CHECK(g.state() == AppState::Playing);
        CHECK_EQ(g.roundsStarted(), round);
        // Exactly the reset event is pending: nothing stale from last round.
        CHECK_EQ(g.events().size(), std::size_t{1});
        CHECK_EQ(countEvents<event::RoundReset>(g.events()), std::size_t{1});
        const Round& r = g.round();
        CHECK_EQ(r.stats().shots(), 0);
        CHECK_EQ(r.stats().hits(), 0);
        CHECK_EQ(r.stats().score(), 0);
        CHECK(!r.stats().accuracyPercent().has_value());
        CHECK(r.clock().phase() == RoundPhase::Countdown);
        CHECK_EQ(r.clock().displaySeconds(), 60);
        CHECK_NEAR(r.time(), 0.0, 0.0);
        CHECK(r.rifle().ready(0.0));
        for (std::size_t i = 0; i < r.lane().size(); ++i) {
            CHECK(r.lane().state(i) == DuckState::Upright);
            CHECK_NEAR(r.lane().motion(i).left, startX[i], 0.0);
        }
        CHECK(r.lane().transitions().empty());
        g.clearEvents();

        // Play it: knock some down, miss some, then let it run out.
        run(g, 1.5 + kFrame);
        for (int k = 0; k < round % 5; ++k) {
            click(g, centreOf(g.round().lane(), static_cast<std::size_t>(k)));
            click(g, kOpenBackboard);
        }
        playToResults(g);
        CHECK_EQ(g.round().stats().shots(), 2 * (round % 5));
    }
}

TEST_CASE("restart: from pause mid-round discards the old round entirely") {
    Game g = makeGame(ShotTuning{.fireCooldown = 0.0});
    startLive(g);
    click(g, centreOf(g.round().lane(), 2));
    run(g, 20.0);
    press(g, Key::Escape);
    press(g, Key::R);
    CHECK(g.state() == AppState::Playing);
    CHECK_EQ(g.round().stats().score(), 0);
    CHECK(g.round().lane().state(2) == DuckState::Upright);
    CHECK(g.round().clock().phase() == RoundPhase::Countdown);
    // The timer starts from the top: Results only after a full 61.5 s more.
    run(g, 61.0);
    CHECK(g.state() == AppState::Playing);
    run(g, 1.6);
    CHECK(g.state() == AppState::Results);
    CHECK(!g.best().best().has_value() || *g.best().best() == 0);
}

TEST_CASE("flow: input after window close is ignored") {
    Game g = makeGame();
    startLive(g);
    g.handle(input::CloseRequested{});
    CHECK(g.state() == AppState::Exiting);
    CHECK(!click(g, kOpenBackboard).has_value());
    CHECK_EQ(g.round().stats().shots(), 0);
}
