# Architecture — Carnival Shooting Gallery

| Field | Value |
|---|---|
| Status | M0 + M1 (booth composition, D-026) + duck target system (D-027, proposed) + shooting (D-028) + round clock, app states and screens (D-029) implemented and verified 2026-11-11 (see [BUILD.md](BUILD.md)). audio (D-030) and atmosphere (D-031) implemented 2026-11-12; audible playback not yet checked by ear |
| Version | 0.1 |
| Date | 2026-11-11 |
| Inputs | [PROJECT_BRIEF.md](PROJECT_BRIEF.md) (approved) |
| Decisions log | [DECISIONS.md](DECISIONS.md) (`D-nnn` references below point there) |

---

## 0. Summary

This is a small, single-screen game: one scene, five targets, one round type, and four application screens. The architecture is sized to match.

- **Two layers, one rule.**
  - A **pure logic library** (`gallery_logic`) holds every gameplay rule, the app-state machine, and the coordinate math. It has **no SFML dependency**.
  - A thin **shell** (the executable) owns the window, assets, rendering, and audio, and translates between SFML and the logic library.
  - The rule is enforced by the link graph: `gallery_logic` does not link SFML, so an SFML include there fails to compile (D-005).
- **One media library.** SFML 3 handles the window, input, 2D rendering, shaders, and audio (D-002). The only other code dependency is the C++ standard library.
- **Plain composition.** There is no ECS, no scene graph, no service locator, and no singletons (D-004). `Application` owns everything by value, in a deliberate declaration order (§3).
- **Logic never sees pixels.** It works in a fixed **logical space of 1920×1080** (D-006). The shell converts window pixels into logical coordinates through one tested function.
- **Frame-rate independent by construction.** The game uses a variable timestep with a clamp, and ducks move at constant velocity, so update rate does not change outcomes (D-007). Shots resolve **before** the update, against what the player last saw (D-008).

---

## 1. Module responsibilities

### 1.1 Layout on disk

```
<project folder>/
├── CMakeLists.txt
├── CMakePresets.json
├── include/gallery/            Public headers of gallery_logic (core/, game/, flow/, view/) (D-025)
├── src/
│   ├── logic/                  ── library gallery_logic (NO SFML) ─────────
│   │   ├── core/               Vec2, Rect, Seconds, small math helpers
│   │   ├── game/               Tuning, AlphaMask, Duck, DuckLane, Rifle,
│   │   │                       RoundClock, RoundStats, Round, GameEvent
│   │   ├── flow/               AppState, InputEvent, Game (state machine),
│   │   │                       SessionBest, UiLayout (button rects)
│   │   └── view/               Viewport (letterbox + window↔logical mapping)
│   ├── shell/                  ── executable target (SFML) ────────────────
│   │   ├── app/                Application (lifecycle + main loop), FrameClock
│   │   ├── platform/           Window setup, event translation, exe path,
│   │   │                       fatal-error dialog, Logger
│   │   ├── assets/             AssetManifest, AssetStore
│   │   ├── render/             Renderer, SceneRenderer, DuckRenderer,
│   │   │                       EffectsLayer, HudRenderer, ScreenRenderer,
│   │   │                       PostProcess, ReticleRenderer
│   │   └── audio/              AudioSystem
│   └── main.cpp                Constructs Application, catches fatal errors
├── assets/                     images/, audio/, fonts/, shaders/
├── tests/                      Unit and contract tests (logic lib only)
│   └── support/test_framework.hpp
└── docs/
```

The empty root `main.cpp` was replaced by `src/main.cpp` in M0. Logic sources live in `src/logic/`, and their public headers in `include/gallery/`.

### 1.2 Module table

| Module | Responsibility | Depends on | Must NOT |
|---|---|---|---|
| **logic/core** | Value types (`Vec2`, `Rect`), `Seconds` alias, clamp, wrap, and transform helpers | std | Know about game rules |
| **logic/game** | Duck movement and state, hit testing, rifle cooldown, round timer, scoring and accuracy, event emission | core | Know about windows, pixels, textures, sounds, or wall-clock time |
| **logic/flow** | App state machine (Title / Playing / Paused / Results / Exiting), routing of logical input to the round or UI, session best, button layout | core, game | Call SFML; draw anything |
| **logic/view** | Letterbox viewport computation; window pixel ↔ logical coordinate mapping | core | Hold state beyond the current window size |
| **shell/app** | Process lifecycle, main loop, frame timing, wiring modules together | everything | Contain gameplay rules |
| **shell/platform** | Create and configure `sf::RenderWindow`; translate `sf::Event` → `InputEvent`; exe-relative paths; logging; fatal-error message box | SFML, Win32 (minimal) | Make gameplay decisions |
| **shell/assets** | Load and own every texture, sound buffer, font, shader source, and derived `AlphaMask`; report missing files | SFML, logic/game (`AlphaMask`) | Be global; load lazily mid-round |
| **shell/render** | Draw the layered scene, ducks, transient effects, HUD, screens, and reticle; post-process | SFML, assets, logic (read-only) | Mutate game state |
| **shell/audio** | Play SFX in response to `GameEvent`s; loop ambience | SFML, assets, logic (events) | Mutate game state |
| **UI** (split) | **Layout and hit-testing** of buttons lives in `logic/flow` (pure, testable). **Drawing** of title, results, and pause lives in `shell/render/ScreenRenderer`. | — | — |

### 1.3 Brief responsibility → owner

| Responsibility (from the request) | Owner |
|---|---|
| Game/application lifecycle | `Application` (shell/app), `main.cpp` |
| Scene and game-state management | `Game` + `AppState` (logic/flow); `Round` + `RoundClock` (logic/game) |
| Asset loading and resource ownership | `AssetStore` (shell/assets) |
| Photographic background/foreground rendering | `SceneRenderer`, `PostProcess` (shell/render) |
| Duck target behavior and movement | `Duck`, `DuckLane` (logic/game) |
| Mouse input and aiming | `translateEvent` (shell/platform) → `Viewport` (logic/view) → `Game` (logic/flow); `ReticleRenderer` draws it |
| Shooting and hit detection | `Rifle`, `DuckLane::hitTest`, `AlphaMask` (logic/game), orchestrated by `Round::fire` |
| Score and round management | `RoundStats`, `RoundClock`, `Round` (logic/game); `SessionBest` (logic/flow) |
| Audio playback | `AudioSystem` (shell/audio) |
| User interface | `UiLayout` (logic/flow) + `HudRenderer` / `ScreenRenderer` (shell/render) |

---

## 2. Class relationships

### 2.1 Overview

```
                         ┌──────────────────────── Application (shell) ────────────────────────┐
 main() ──owns──► Application                                                                    │
                  │  owns (by value, declaration order = construction order)                     │
                  ├── Logger                                                                     │
                  ├── sf::RenderWindow           (via platform::createWindow)                    │
                  ├── AssetStore ─────────────── owns textures, sound buffers, fonts, masks       │
                  ├── Game  (logic) ─────────── owns Round, SessionBest, AppState, UiLayout       │
                  │     └── Round ───────────── owns DuckLane, Rifle, RoundClock, RoundStats,     │
                  │           │                       event buffer                                │
                  │           └── DuckLane ──── owns 5 × Duck; borrows const DuckShape& (mask)    │
                  ├── Viewport (logic, value)                                                    │
                  ├── Renderer ──────────────── borrows const AssetStore&                         │
                  │     ├── owns SceneRenderer, DuckRenderer, EffectsLayer,                      │
                  │     │        HudRenderer, ScreenRenderer, ReticleRenderer                    │
                  │     └── owns PostProcess (sf::RenderTexture + optional sf::Shader)           │
                  ├── AudioSystem ───────────── borrows const AssetStore&; owns sf::Sound pool,   │
                  │                             sf::Music                                         │
                  └── FrameClock                                                                 │
                  └──────────────────────────────────────────────────────────────────────────────┘

  Data flow each frame:
    sf::Event ─► InputEvent ─► Viewport (px → logical) ─► Game::handle()
    Game::update(dt) ─► GameEvent buffer ─► AudioSystem::consume(), Renderer::consume()
    Renderer::draw(const Game&, Viewport, reticle position)
```

Arrows are **ownership** ("owns") or **non-owning reference** ("borrows"). There is no inheritance in the core design. The only polymorphism is `std::variant` for events.

### 2.2 Logic library (no SFML)

| Class / type | Kind | Key members / operations | Notes |
|---|---|---|---|
| `Vec2`, `Rect` | value structs | `float x, y`; `contains()` | Separate from `sf::Vector2f` so logic stays SFML-free; converted at the shell boundary |
| `Tuning` | aggregate struct | `duckCount=5`, `laneSpeed=0.25` (lane widths/s), `fireCooldown=0.25s`, `pointsPerHit=10`, `roundLength=60s`, `countdown=1.5s`, `lowTimeThreshold=10s`, `duckMinDownTime=2s`, `endDelay=1s`, `alphaThreshold=128` | Every brief number in one place. Passed by const ref; tests construct their own |
| `AlphaMask` | value class | `static AlphaMask fromRgba(span<const uint8_t>, w, h, threshold)`; `bool solidAt(int x, int y) const` | Packed bits; built once from the duck image at load. Out-of-bounds returns `false` |
| `DuckShape` | struct | `Size size` (logical); `Size imageSize`; `AlphaMask mask` (empty = whole box) | One per paint variant, built by the shell from the image that is drawn (D-027). Copied into the lane: no borrowed references |
| `DuckConfig`, `LaneConfig` | aggregate structs | mode (Shuttle/Wrap), bounds, trackY, speed, turnTime, dwell, phase, startDirection, wobble, reaction, rise trigger, fall/rise/min-down times, dropDepth, variant | All target tuning as data; presets `sections` / `chain`; `validateLane()` (D-027) |
| `DuckMotion` | value class | `sample(clock)` → position, velocity, direction, edge count; `timeToEdge(clock)` | Pure motion profile; frame-rate independent by construction |
| `Duck` | private struct in `DuckLane` | motion clock; `DuckState {Upright, Falling, Down, Rising}`; `stateTime`; hit direction | Plain data |
| `DuckPose` | value struct | `Vec2 hinge; float fold; float tiltDeg; float sink; bool hittable` | **The single description of where a duck is.** Both hit testing and rendering derive from it (risk R-3) |
| `DuckLane` | class | `update(Seconds)`; `knockDown(id)`; `hitTest(Vec2)`; `pose(id)`; `motion(id)`; `state(id)`; `transitions()`; `reset()` | Owns the targets (count from config); exact event-timed state machine; re-entry at an end stop or wrap after min down time (D-027). `duckImageToLogical()` is the one draw/hit transform |
| `Rifle` | class | `bool tryFire(Seconds now)`; `reset()` | Cooldown only; rejected clicks are never counted (G-8) |
| `RoundClock` | class | `RoundPhase {Countdown, Live, Ending, Finished}`; `update(dt)`; `remaining()`; `acceptsShots()` | Emits `Go`, `LowTimeTick`, `TimeUp` events |
| `RoundStats` | value class | `shots`, `hits`, `score`; `std::optional<int> accuracyPercent() const` | `nullopt` when shots == 0 → UI shows "—" (G-23) |
| `GameEvent` | `std::variant<…>` | `RoundStarted`, `Go`, `ShotFired{pos}`, `DuckHit{id, pos, points}`, `ShotMissed{pos}`, `DuckRose{id}`, `LowTimeTick{secondsLeft}`, `TimeUp`, `RoundFinished{stats}` | Per-frame vector, read by audio and effects, then cleared (D-010) |
| `Round` | class | `fire(Vec2 logicalAim)`; `update(dt)`; `stats()`; `clock()`; `lane()`; `events()`; `clearEvents()` | Composes the five pieces above. The only mutator API for gameplay |
| `AppState` | enum class | `Title, Playing, Paused, Results, Exiting` | — |
| `InputEvent` | `std::variant<…>` | `PointerMoved{logical}`, `PrimaryPressed{logical}`, `KeyPressed{Key}`, `FocusLost`, `FocusGained`, `CloseRequested` | Already in logical coordinates when it reaches `Game` |
| `UiLayout` | struct | Button rects for Start, Play Again, and Resume, in logical space; `hit(Vec2) → optional<ButtonId>` | Pure; drawn by `ScreenRenderer` |
| `SessionBest` | value class | `submit(score) → bool isNewBest`; `best()` | Memory only (G-31) |
| `Game` | class | `handle(const InputEvent&)`; `update(dt)`; `state()`; `round()`; `aim()`; `best()`; `events()` | Owns the app state machine (§4). The top of the logic layer and the only object `Application` talks to for game behavior |
| `Viewport` | value class | `static Viewport fit(Size window, Size logical)`; `Rect pixelRect()`; `Vec2 toLogical(Vec2 px)`; `Vec2 toPixel(Vec2 logical)`; `float scale()` | Pure math, exhaustively tested (§2.4) |

### 2.3 Shell (SFML)

| Class | Responsibility | Owns | Borrows |
|---|---|---|---|
| `Application` | Builds everything, runs the loop, tears down in order | All subsystems (by value) | — |
| `FrameClock` | `sf::Clock` wrapper; returns clamped `dt` (§5.2) | `sf::Clock` | — |
| `platform::createWindow()` | Free function that configures `sf::RenderWindow` (title, vsync, icon, min size) | — | — |
| `platform::translateEvent()` | Free function: `sf::Event` → `std::optional<RawInput>` (still in window pixels) | — | — |
| `platform::exeDirectory()` | Locates assets relative to the executable, not the working directory | — | — |
| `platform::showFatalError()` | Win32 `MessageBoxW` with the error text | — | — |
| `Logger` | Timestamped lines to `stderr` and `carnival.log`; redirects `sf::err()` | `std::ofstream` | — |
| `AssetStore` | Loads everything from the manifest at startup; typed accessors | `sf::Texture`, `sf::SoundBuffer`, `sf::Font`, shader source strings, `AlphaMask`, `DuckShape` | — |
| `Renderer` | Per-frame draw orchestration; reacts to resize | `PostProcess`, the sub-renderers | `AssetStore` |
| `SceneRenderer` | Static photographic layers 1–4 and 6–8 (brief §6.1) | `sf::Sprite`s | textures |
| `DuckRenderer` | Draws each duck from its `DuckPose`, using the same transform as hit testing | — | textures, `DuckLane` |
| `EffectsLayer` | Visual-only transients: muzzle flash, recoil offset, miss marks (≤20, 2 s fade), "+10" pop-ups, rifle swivel | small fixed-capacity arrays | textures |
| `HudRenderer` | Score board and timer (diegetic) | `sf::Text` | font, `Round` |
| `ScreenRenderer` | Title, Pause, and Results boards using `UiLayout` rects | `sf::Text` | font, `Game` |
| `PostProcess` | Offscreen target + grade/vignette/grain shader; degrades to a plain blit | `sf::RenderTexture`, `std::optional<sf::Shader>` | shader source |
| `ReticleRenderer` | Draws the reticle after post-processing, at the exact aim point | `sf::Sprite` | texture |
| `AudioSystem` | Maps `GameEvent` → sound; fixed voice pool; ambience loop | `std::vector<sf::Sound>` (16), `sf::Music` | sound buffers |

### 2.4 Coordinate spaces

There are three spaces. Only `Viewport` converts between them.

| Space | Units | Used by |
|---|---|---|
| **Window pixels** | integer px, origin at the window's top-left | SFML events, the window |
| **Logical** | float, 1920×1080, origin top-left, y down | All of logic, all layout, all render positions |
| **Image** | texel of a specific texture | `AlphaMask` lookup only (`DuckShape` maps logical → image) |

`Viewport::fit(window, logical)` picks `scale = min(w/1920, h/1080)` and centers the scaled rectangle, which leaves black bars on the remaining axis. Then:

- `toLogical(px) = (px − offset) / scale`
- `toPixel(l) = l · scale + offset`

Tests cover exact 16:9 sizes, 4:3, 21:9, very small windows, odd pixel sizes, round-trip accuracy within 0.5 px, and points inside the bars.

**Aim outside the play area.** The cursor is confined to the window, but the window may include letterbox bars. The aim point is **clamped to the logical rectangle**, and the reticle is drawn at the clamped point. A click therefore always lands where the reticle is (D-015).

---

## 3. Resource ownership and lifecycle

### 3.1 Principles

1. **Every resource has exactly one owner, which is an object** (RAII). No `new` or `delete` appears in project code, and no raw owning pointers.
2. SFML types are RAII already (`sf::Texture`, `sf::SoundBuffer`, `sf::RenderTexture`, `sf::Music`). We hold them **by value** inside owners whose lifetime is the whole process.
3. **Borrowing is by `const&`**, and only from an owner declared *earlier* in `Application`. C++ destroys members in reverse declaration order, so every borrower dies before what it borrows.
4. **No globals and no singletons.** The one unavoidable process-wide object is SFML's internal audio device and GL context management, which we do not control.
5. Containers that hand out references (`AssetStore` maps) are **fully populated before any reference is taken and never modified afterward**. This rules out reference invalidation by rehash.

### 3.2 `Application` member order (normative)

```
// main() owns the Logger and outlives Application, so a throwing constructor
// can still be logged (D-025). Application holds `Logger& log_`.
class Application {
    Logger&           log_;         // 1  borrowed from main()
    AssetPaths        assetPaths_;  //    validated before the window: fail fast (D-025)
    sf::RenderWindow  window_;      // 2  creates the GL context textures need
    AssetStore        assets_;      // 3  textures, buffers, fonts, masks
    DuckVisuals       duckVisuals_; //    target images + hit shapes built from them (D-027)
    gallery::Game     game_;        // 4  logic: state machine + Round (copies its DuckShapes)
    gallery::Viewport viewport_;    // 5  value
    Renderer          renderer_;    // 6  borrows assets_
    AudioSystem       audio_;       // 7  borrows assets_  (sf::Sound must die before its SoundBuffer)
    FrameClock        clock_;       // 8
};
```

Reordering these members is a bug. A comment in the header will say so, and a review checklist item covers it.

### 3.3 Lifecycle phases

| Phase | What happens | Failure behavior |
|---|---|---|
| **Boot** | `main` → `Application` constructor. Logger opens; window created; `AssetStore` loads **every** asset in the manifest, collecting all failures before throwing once; masks built from duck images; `PostProcess` compiles its shader; `AudioSystem` creates voices and opens ambience | Missing required assets or no window → `StartupError` thrown → caught in `main` → message box + log → exit code 1. Shader or audio failure → logged warning, feature disabled, game continues (§6) |
| **Run** | Main loop (§5) until `AppState::Exiting` or the window is closed | No allocation-heavy work per frame; no I/O except logging |
| **Round restart** | `Round::reset()` resets plain data in place: lane, rifle, clock, stats, events. `EffectsLayer::clear()` runs on the `RoundStarted` event | Cannot fail; no loading (AC-13, G-32 ≤ 0.5 s) |
| **Resize** | `Viewport` recomputed; `PostProcess` render texture recreated at the new viewport pixel size **once per frame at most**, lazily at the start of `draw` | Zero or minimized size → skip rendering; the game is paused via focus rules |
| **Shutdown** | Loop exits; `Application` destructor runs members in reverse order; `main` returns 0 | Destructors do not throw |

### 3.4 Asset manifest

- The asset list is a **compiled-in table** of `{AssetId, relative path, kind, required}`, using enum-indexed IDs. There is no runtime JSON or XML parser, because there is nothing to configure at runtime (D-012).
- Paths resolve against `exeDirectory()/assets/`. CMake copies `assets/` next to the executable after each build.
- A test checks that every manifest path exists in the source `assets/` folder (§7), so a missing file fails CI rather than a playtest.
- **Formats:**
  - JPEG for opaque photographic plates.
  - PNG for anything with alpha (ducks, booth frame, counter, rifle).
  - WAV for short SFX, so decoding never happens on the hot path.
  - OGG for the streamed ambience loop.
  - One TTF display font under the OFL license.
  - GLSL `.frag` for the post-process shader.

---

## 4. Application and gameplay state transitions

### 4.1 Application states (`Game`, logic/flow)

> **D-029 (implemented):** as drawn below, plus: P also pauses and resumes; Results has a Main Menu placard; Title has a Quit placard. Pausing is possible only while the round is running (Countdown or Live): once time is up, input is disabled. Going to the title resets the round (`RoundReset`), so an abandoned round is never scored. Button rects live in `gallery::ui` (`UiLayout.hpp`).

```
           ┌───────────┐  Start button / Space / Enter      ┌──────────────┐
  boot ───►│   Title   │───────────────────────────────────►│   Playing    │
           └───────────┘                                    │ (Round runs) │
             │  Esc                                         └──────────────┘
             ▼                                     Esc / FocusLost │    ▲ click / Space / Enter
         Exiting ◄──────────────┐                               ▼    │ (resume click is NOT a shot)
             ▲                  │                        ┌──────────────┐
             │ Esc              │ CloseRequested          │    Paused    │── Esc ──► Title
             │                  │ (any state)             └──────────────┘── R ───► Playing (fresh round)
     ┌───────────────┐          │
     │    Results    │◄─────────┴──── Round reaches Finished (TimeUp + endDelay)
     └───────────────┘
        │ Play Again button / R / Enter
        └──────────────────────────────────────────────► Playing (fresh round, skips Title)
```

Notes:

- `FocusLost` matters only in `Playing`, where it moves to `Paused`. In other states it only releases the cursor, which the shell does.
- `FocusGained` **does not** auto-resume. The player must click or press a key (G-29).
- Entering `Playing` from Title, Results, or Paused+R always calls `Round::reset()` → emits `RoundStarted`.
- Entering `Results` calls `SessionBest::submit()` once.
- `Game::update(dt)` advances the round **only** in `Playing`. Paused, Title, and Results freeze it, so the timer cannot drift (AC-15).
- The cursor is hidden and grabbed in `Playing`, and visible and free in every other state. The shell reads `game.state()` after handling input and applies this. The logic layer does not call SFML.

### 4.2 Round phases (`RoundClock`, logic/game)

```
 Countdown (1.5 s) ──► Live (60 s) ──► Ending (1.0 s) ──► Finished
  ducks move              shots accepted         ducks stop,          Game → Results
  shots rejected          LowTimeTick at ≤10 s   input ignored
  emits RoundStarted      emits Go               emits TimeUp         emits RoundFinished
```

- **Rejected-shot policy.** During Countdown and Ending, primary clicks are dropped *before* `Rifle::tryFire`. They are not shots and not counted (G-26, G-28, AC-16).
- **Timer boundary.** Input is processed before `update` (§5). A click in the same frame that the timer would cross zero is therefore resolved while the phase is still `Live`, and counts (G-28).
- **Time precision.** Timer and duck time are `double` seconds, accumulated from clamped frame deltas.

### 4.3 Duck states (`Duck`, logic/game)

> **D-027 (proposed):** the default target preset is now `sections`: five independent shuttle carriages, one rail section each, rising at an end stop. The chain loop described below is the `chain` preset (`--targets chain`). Note that the posts are only 12 units wider than a duck, so a chain duck is still rising as it emerges.

```
 Upright ──hit──► Falling (0.2 s) ──► Down ──[wrapped past lane end AND down ≥ 2 s]──► Rising (0.2 s) ──► Upright
 (hittable)       (not hittable)      (not hittable, keeps riding the chain)          (not hittable)
```

- **Position is a function of chain position only.** `x = laneStart + wrap(chainPos, loopLength)`, where `loopLength = laneWidth + hiddenMargin`. The hidden margin lies behind the booth's side posts, so wrap-around is never visible.
- **Spacing.** Ducks are spaced `loopLength / 5` apart and all move together. Spacing therefore cannot drift, and G-19 (the lane is never empty) holds whenever at most four ducks are down at once.
- **Rising runs behind the posts.** The `Rising` transition happens in the hidden margin, so ducks come back into view already upright.

### 4.4 Shot resolution (`Round::fire`)

```
 PrimaryPressed(logical)
   └─► Game: state == Playing?        no → UI hit-test (buttons) or ignore
         └─► Round: clock.acceptsShots()?    no → ignore (not counted)
               └─► Rifle::tryFire(now)?      no → ignore (cooldown, not counted)
                     ├─ stats.shots++ ; emit ShotFired{aim}
                     └─► DuckLane::hitTest(aim)
                           ├─ hit id → knockDown(id); stats.hits++; score += 10; emit DuckHit
                           └─ none   → emit ShotMissed{aim}
```

`DuckLane::hitTest` works as follows:

1. Iterate ducks in **front-to-back draw order**, considering only `Upright` ones.
2. Transform the aim point into each duck's image space using its `DuckPose` + `DuckShape`.
3. Sample `AlphaMask::solidAt`.
4. Return the first hit. One shot hits at most one duck (G-16, AC-7).

---

## 5. Main loop

### 5.1 Structure

```
run():
  while state != Exiting and window open:
      dt = clock.frameDelta()                        // clamped, §5.2

      // 1. INPUT. Resolve against what was last drawn (D-008).
      while (event = window.pollEvent()):
          raw = platform::translateEvent(event)      // window pixels
          if Resized:  viewport = Viewport::fit(newSize, kLogical); continue
          game.handle(toLogical(raw, viewport))      // clamps aim into logical rect
      applyCursorPolicy(game.state())                // hide/grab vs show/release

      // 2. UPDATE. Pure logic, frame-rate independent.
      game.update(dt)

      // 3. REACT. Presentation consumes this frame's events.
      audio.consume(game.events())
      renderer.consume(game.events())                // spawns effects, clears on RoundStarted
      game.clearEvents()

      // 4. RENDER. Read-only view of logic.
      renderer.update(dt)                            // effect fades, rifle swivel easing
      renderer.draw(window, game, viewport)
      window.display()
```

- `game.handle` and `game.update` push into the same event buffer, so effects and sounds for a shot appear **in the same frame as the click** (G-9, AC-14).
- **Update and render are separate types.** The renderer receives `const Game&` and cannot mutate gameplay. Its own `update(dt)` advances visual-only state such as fades.

### 5.2 Frame time

- `dt` comes from `sf::Clock` (monotonic) and is **clamped to 100 ms**.
- Why clamp: on Windows, dragging or resizing the window stalls the message loop, and a debugger pause can take seconds. Without the clamp, one huge `dt` would teleport ducks and burn the timer.
- Cost: during a hitch longer than 100 ms, game time runs slower than wall time. This is accepted. AC-11's ±100 ms bound applies to normal play, and window drags pause play in practice (R-5).
- **Variable timestep, no fixed-step accumulator, no interpolation (D-007).**
  - All motion is constant-velocity and all timers are linear, so `pos += v·dt` is exact for any `dt`.
  - A fixed step would add an accumulator plus interpolation. Interpolation would make the drawn duck differ from the simulated one, which breaks the "hit what you see" guarantee.
  - A frame-rate-independence test (§7) enforces this.
- **Vsync** is on by default, giving a 60 Hz cap on typical displays. Its latency is measured in M2 (R-2). If it fails AC-14, fall back to `setFramerateLimit` with vsync off.

### 5.3 Render order

These are the brief's §6.1 layers. They are drawn into the `PostProcess` render texture with a logical `sf::View` of 1920×1080.

1. Sky / far background
2. Mid background
3. Booth frame and header
4. Backboard
5. Ducks
6. Wave strip
7. Counter
8. Rifle, plus muzzle flash and miss marks

Effects are placed at the correct depth: miss marks on the backboard (layer 4), muzzle flash with the rifle (layer 8).

The post-process shader then applies **grade + vignette + grain**, and the result is blitted to the window at `viewport.pixelRect()`.

After that, these are drawn directly on the window with **no post-process**:

9. HUD: score board and timer
10. Overlays: title, pause, results
11. Reticle

This moves the HUD out of the post-processed group, which differs from brief §6.1. See §9 and D-014.

The render texture is sized to the **viewport's pixel size**, not fixed at 1920×1080. Output is therefore crisp at any window size, and logical coordinates are unchanged (D-016).

---

## 6. Error handling

| Category | Examples | Policy |
|---|---|---|
| **Fatal startup** | Window/GL context creation fails; any *required* asset missing or undecodable; asset folder not found | Throw `StartupError` (derives from `std::runtime_error`) carrying **all** problems found, one per line. `main` catches it → logs → `showFatalError()` → exit code 1. The message names the expected `assets/` path |
| **Degradable startup** | `sf::Shader::isAvailable()` is false or the shader fails to compile; no audio device; ambience file missing (marked optional) | Log `WARN`, disable the feature, continue. Post-process becomes a plain blit; audio becomes no-op voices |
| **Runtime, recoverable** | `RenderTexture` recreation fails on resize (e.g. out of VRAM at an extreme size) | Log, keep the previous texture, retry next resize |
| **Programming errors** | Invalid duck ID, negative `dt`, a state transition that should be impossible | `assert` in Debug. In Release, a defined safe fallback (ignore) plus a log line. Logic code is written so that these are unrepresentable where cheap (enums, `std::optional`, fixed arrays) |
| **Unexpected exceptions** | `std::bad_alloc` and similar | `main` catches `std::exception` and `...` as a last resort → same dialog path |

Rules:

- **Exceptions occur only at boundaries.** Constructors in boot and `main`. The per-frame path (`handle`, `update`, `consume`, `draw`) is designed not to throw: no allocation-dependent logic beyond reserved buffers, and no I/O except logging.
- **The logic library never logs and never throws for game conditions.** "Shot rejected" and "no duck hit" are normal outcomes expressed in return values and events.
- **`Logger`** writes timestamped lines to `stderr` and to `carnival.log` next to the executable. If the log file cannot be opened, the game continues with `stderr` only. `sf::err()` is redirected into it, so SFML warnings, such as a failed texture decode, are captured.

---

## 7. Testing strategy

### 7.1 Tiers

| Tier | Scope | How | Covers (brief IDs) |
|---|---|---|---|
| **Unit** | `gallery_logic` classes in isolation | Hand-rolled header framework (`tests/support/test_framework.hpp`), so no new dependency (D-017). One test exe, registered with CTest | AC-4–AC-11, AC-16 |
| **Scenario** | Full `Game` driven by a scripted list of `(t, InputEvent)` with synthetic `DuckShape`s | Same framework. Uses no SFML, no window, and no real time | AC-12, AC-13, AC-15, G-19 |
| **Contract** | Assets and build | Every manifest path exists. Duck PNGs have alpha. Shader file present | AS-2 (partly), startup robustness |
| **Manual / measured** | Feel, visuals, latency, performance | Checklist mapped to AC/AV IDs, run at each milestone. A Debug-only overlay (F3) shows hit masks, viewport rect, `dt`, and frame-time history | AC-1–AC-3, AC-14, AV-1–AV-7 |

### 7.2 Key test cases (non-exhaustive)

- **Viewport:** see §2.4. Includes `toPixel(toLogical(p)) ≈ p`, and that bar points clamp to the edge.
- **AlphaMask:**
  - Opaque, transparent, and threshold-boundary texels.
  - Out-of-bounds returns false.
  - A non-multiple-of-8 width (bit packing).
- **Hit test:**
  - Center hit.
  - Transparent corner of the bounding box → miss (AC-6).
  - Overlapping ducks → front-most only (AC-7).
  - Falling, Down, and Rising ducks never hit.
- **Rifle:**
  - Second click at 249 ms rejected, at 250 ms accepted.
  - Rejected clicks don't change `shots` (AC-4).
- **Accuracy:**
  - 0 shots → `nullopt`.
  - 1/3 → 33, 2/3 → 67, 1/8 → 13 (12.5 rounds half up).
  - 5/5 → 100 (AC-10).
- **Duck lifecycle:**
  - Hit → not hittable within one update.
  - Rises only after wrap **and** ≥ 2 s down.
  - Hit just before the wrap point → stays down for another lap (AC-8).
- **Timer:**
  - Countdown rejects shots.
  - A click in the frame that crosses zero counts.
  - `Finished` is reached after 60 s + `endDelay` of summed `dt` (AC-11, simulated clock).
- **Frame-rate independence:** run the same scripted 62.5 s session at fixed 30, 60, 144, and 240 Hz, plus a seeded jittery `dt` sequence. Duck positions at checkpoints must match within 1e-3 logical px, and the final stats must be identical.
- **Restart:** play to Results, restart ten times, and assert all stats are zero, positions match initial, phase is Countdown, and no stale events remain (AC-13).
- **Pause:**
  - `FocusLost` freezes clock and lane.
  - The resume click produces no `ShotFired` (AC-15).
- **Flow:** every arrow in §4.1, including keys that should do nothing in a state.

### 7.3 Untested by design

Pixel output, shader appearance, and audio are verified manually against AV-1–AV-7. Screenshot-diff testing is not worth its cost for a single static scene.

---

## 8. Build and dependency strategy

| Aspect | Choice |
|---|---|
| Language | C++20 (`std::span`, designated initializers, `<numbers>`). No modules. |
| Compiler | MSVC 14.44 (VS 2022 Build Tools) |
| Build system | CMake ≥ 3.24 (the bundled 3.31 works) with `CMakePresets.json`: `windows-msvc` (VS 17 2022 generator). Configs: Debug, Release |
| Warnings | `/W4 /permissive- /utf-8`, and `/WX` on project targets only (not on SFML) |
| Targets | `gallery_logic` (static lib, **does not link SFML**); `carnival_gallery` (WIN32 exe: logic + SFML); `gallery_tests` (console exe: logic only) |
| Media dependency | **SFML 3.0.x**, modules Graphics, Window, System, Audio. Network is turned off (`SFML_BUILD_NETWORK OFF`) (D-002) |
| How fetched | CMake `FetchContent`, pinned to an exact release tag and recorded commit hash, built **statically** (`BUILD_SHARED_LIBS OFF`). The game ships as one exe plus `assets/` (D-003) |
| Transitive deps | SFML's own fetched/bundled deps: FreeType, Ogg/Vorbis/FLAC, and miniaudio (internal to SFML 3). They are not used directly |
| Offline/MITM fallback | If `FetchContent` fails behind TLS interception, such as a corporate proxy or antivirus HTTPS scanning (R-4), set `FETCHCONTENT_SOURCE_DIR_SFML` to a locally downloaded source archive. The preset documents the variable |
| Test framework | Hand-rolled header, so no dependency |
| Assets | `assets/` copied next to the exe by a post-build step. Provenance logged in `assets/ASSET_MANIFEST.md` (brief AS-2; see [ASSET_GUIDE.md](ASSET_GUIDE.md) and D-020). Masters and originals live in `asset-src/`, which is never shipped (D-021) |
| Windows specifics | App manifest declaring per-monitor-v2 DPI awareness (R-6). `WIN32` subsystem in Release, console subsystem in Debug for visible logs |
| CI | Not in MVP. The `cmake --preset` + `ctest` commands are the local gate before each milestone commit |

**No other libraries.** Every candidate was checked against an actual requirement:

| Candidate | Verdict |
|---|---|
| JSON/TOML parser | No runtime config exists |
| Math library | `Vec2` and an affine transform are about 100 lines |
| ECS (EnTT etc.) | Five ducks |
| Logging library | Writing to a file is enough |
| Dear ImGui | The F3 overlay is a few `sf::Text`s |
| Catch2/doctest | The house test framework is sufficient |

Any of these can be revisited if a concrete requirement appears.

---

## 9. Architectural risks (pre-implementation)

| ID | Risk | Likelihood / impact | Mitigation | Check by |
|---|---|---|---|---|
| **R-1** | **Photographic art is unavailable, or inconsistent in perspective and lighting** (brief A-3). The look depends on it more than on any code | High / High | The layer stack and manifest let placeholders be swapped without code changes. Duck masks are derived from whatever PNG is supplied. Lock the camera height and horizon (brief §7.6) as an art spec before sourcing | M1 composited mock frame |
| **R-2** | **Input-to-photon latency** of the software reticle under vsync feels laggy (AC-3, AC-14) | Medium / High | Measure in M2 with the F3 overlay and a frame-capture method. Fallbacks, in order: (a) vsync off + `setFramerateLimit(display Hz)`; (b) use an `sf::Cursor` image as a **hardware cursor** for the reticle, which has the lowest possible latency but cannot be post-processed or animated | M2 |
| **R-3** | **Drawn duck ≠ hit duck.** Any mismatch between the render transform and the hit-test transform, or between mask resolution and texture, makes hits feel unfair | Medium / High | Single `DuckPose` → transform function in logic, used by both; mask built from the *same* image as the texture; F3 overlay draws masks over sprites; unit tests on the transform | M3 |
| **R-4** | **Dependency fetch blocked** by TLS interception (a corporate proxy or antivirus HTTPS scanning). SFML also fetches its own sub-deps | Medium / Medium | Try at M0 first thing. Fallback: local source archive via `FETCHCONTENT_SOURCE_DIR_SFML`, plus SFML's `SFML_USE_SYSTEM_DEPS`-off bundled deps; or vendor into `third_party/` (gitignored, documented) | M0 |
| **R-5** | **Windows modal loop** during window drag or resize stalls frames, then delivers one big `dt` | High / Low | 100 ms `dt` clamp; lazy render-texture recreation; drags pause play in practice | M0/M4 |
| **R-6** | **DPI scaling** (125–150% common on laptops) causes blurry output or coordinate mismatch | Medium / Medium | DPI-aware manifest; all mapping through `Viewport` using the actual framebuffer size reported by SFML; test at 100/150% scaling | M0 |
| **R-7** | **Full-screen post-process cost** at high resolutions on integrated GPUs (brief A-12, AV-7) | Low–Medium / Medium | One cheap single-pass shader; grain sampled from a small tiled **scanned film-grain** texture, not procedural noise (D-022); shader toggle in F3 to measure delta | M5 |
| **R-8** | **SFML 3 API unfamiliarity.** Most online material is for SFML 2 (event API, `sf::Sound` construction, `Rect` changes) | Medium / Low | Pin one version; isolate SFML to the shell; consult SFML 3 docs/migration guide, not SFML 2 examples | M0–M2 |
| **R-9** | **Gunshot audio latency** from the backend's default buffer, so the sound arrives noticeably after the flash | Low / Medium | Preloaded WAV buffers, sounds started in the same frame; measure in M5; trim leading silence in assets | M5 |
| **R-10** | **Brief inconsistency:** §6.1 applies post-processing to the HUD (layer 9), while §7.5 says grain must not be visible on HUD numbers | Certain / Low | Resolved here: HUD drawn after post-process (D-014). Brief should be amended with the owner's approval | Now |
| **R-11** | **Scope creep into an engine:** generic scenes, component systems, data-driven everything | Medium / Medium | This document is the scope fence. New abstractions need a concrete second use case. Tier-2 features (second lane, golden duck) fit the current classes: `DuckLane` per lane, `Duck::variant` for value | Ongoing |

---

## 10. Implementation sequencing (maps to brief §12)

| Milestone | Architectural deliverable |
|---|---|
| M0 | CMake presets; SFML fetched and statically linked; window opens; `Logger`, `exeDirectory`, fatal dialog; `gallery_logic` + `gallery_tests` skeleton with the `Viewport` tests passing. **Resolve R-4, R-6** |
| M1 | `AssetStore` + manifest + contract test; `SceneRenderer` layer stack with placeholders; `PostProcess` with fallback. **Address R-1** |
| M2 | `InputEvent` translation, aim clamping, `Rifle`, `ReticleRenderer`, `EffectsLayer` (flash, recoil, miss marks), `AudioSystem` gunshot. **Measure R-2** |
| M3 | `AlphaMask`, `Duck`, `DuckLane`, `DuckRenderer`, F3 mask overlay. **Close R-3** |
| M4 | `RoundClock`, `RoundStats`, `Round`, `Game` state machine, `UiLayout`, `HudRenderer`, `ScreenRenderer`; scenario tests |
| M5 | Ambience, remaining SFX, shader tuning, performance pass. **Measure R-7, R-9**; full AC/AV checklist |
