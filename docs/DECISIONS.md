# Technical Decisions — Carnival Shooting Gallery

This is a log of significant technical decisions. Each entry records the context, the decision, the alternatives considered, and the consequences.

- **Statuses:**
  - **Accepted** means it is in effect.
  - **Provisional** means it is in effect but has an explicit measurement that could overturn it.
  - **Superseded** means it was replaced; the entry links to its replacement.
- **Changing a decision:** add a new entry. Do not rewrite old ones.

| ID | Decision | Status |
|---|---|---|
| D-001 | C++20, MSVC, CMake presets | Accepted |
| D-002 | SFML 3 as the only media library | Accepted |
| D-003 | SFML via pinned FetchContent, static link | Accepted |
| D-004 | Plain composition; no ECS, no scene graph, no singletons | Accepted |
| D-005 | Gameplay logic in an SFML-free library | Accepted |
| D-006 | 1920×1080 logical resolution with letterboxing | Accepted |
| D-007 | Variable timestep with 100 ms clamp; no interpolation | Accepted |
| D-008 | Shots resolve before update (against the last drawn frame) | Accepted |
| D-009 | Per-pixel alpha-mask hit detection | Accepted |
| D-010 | Per-frame `std::variant` event buffer instead of observers | Accepted |
| D-011 | App states as an enum inside a pure `Game` class | Accepted |
| D-012 | Compiled-in asset manifest; load everything at startup; fail fast | Accepted |
| D-013 | Exceptions only at startup boundaries; cosmetic features degrade | Accepted |
| D-014 | Post-process covers world layers only; HUD and reticle drawn after | Accepted (amends brief §6.1) |
| D-015 | Aim point clamped to the logical play area | Accepted |
| D-016 | Offscreen render target sized to the viewport's pixels | Accepted |
| D-017 | Hand-rolled header-only test framework | Accepted |
| D-018 | Software-drawn reticle, hardware cursor as fallback | Provisional (M2 latency measurement) |
| D-019 | Fixed SFX voice pool; streamed ambience | Accepted |
| D-020 | Asset manifest at `assets/ASSET_MANIFEST.md` replaces `docs/ASSET_SOURCES.md` | Accepted |
| D-021 | Originals and masters in `asset-src/`, separate from runtime `assets/` | Accepted |
| D-022 | Film grain from a scanned plate, not procedural noise | Accepted |
| D-023 | Release builds blocked by placeholder, missing, or unverified MVP assets | Accepted |
| D-024 | SFML pinned to 3.0.2 by hashed source archive; raylib re-checked and not adopted | Accepted |
| D-025 | M0 structure refinements: public logic headers, Logger owned by `main`, DPI-sized window | Accepted |
| D-026 | M1 booth composition: eight layer groups, labelled grey stand-ins for missing art, Release refuses to start without assets | Accepted |
| D-027 | Duck target system: independent shuttle sections by default (amends brief G-14/AC-5, **pending owner approval**), chain preset kept, exact event-timed state machine, masks from the drawn image | Proposed |
| D-028 | Shooting: hitscan `Round::fire` in logic (rifle cooldown, alpha-mask hit, exactly-once scoring, stats), event-driven effects in the shell | Accepted |
| D-029 | Round clock (countdown, 60 s, ending), app state machine in `Game`, painted-board screens and one UI theme | Accepted |
| D-030 | Audio: CC0 field recordings cut by a recorded tool, soundscape rules in logic, `AudioSystem` in the shell, player volume | Accepted (assets pending listening review) |
| D-031 | Atmosphere: booth light level (generator sag), shot flash as an exposure lift, bunting sway; `--no-flicker` | Accepted |
| D-032 | Engineering verification fixes: foreground occlusion, strict font decode, frame-time log | Accepted |
| D-033 | Desktop distribution: preview build (stand-ins allowed, "(Preview)" title), product-named exe, static CRT, self-contained install layout with license texts | Accepted (owner chose the preview option, 2026-11-12) |

---

## D-001 — C++20, MSVC, CMake presets

- **Status:** Accepted, 2026-11-11. Resolves brief assumption A-1 (language).
- **Context:**
  - The project is intended as a professional C++ project.
  - VS 2022 Build Tools (MSVC 14.44) bring a bundled CMake 3.31 and Ninja, a common free Windows setup, used through a `windows-msvc` preset.
  - No compiler is on `PATH`, so presets that use the Visual Studio generator avoid needing a Developer Command Prompt.
- **Decision:**
  - C++20 with MSVC.
  - CMake ≥ 3.24 with a `windows-msvc` preset.
  - Warnings at `/W4 /permissive-`, with `/WX` on project targets.
- **Alternatives:**
  - C++17, which loses `std::span` and designated initializers for little gain.
  - C++23, whose `std::expected` would be handy, but MSVC library support and the project's needs don't justify it.
  - Clang/MinGW, which is not installed.
- **Consequences:**
  - Windows-only builds in practice.
  - The code itself stays portable apart from `platform/` (Win32 message box, exe path, DPI manifest).

## D-002 — SFML 3 as the only media library

- **Status:** Accepted, 2026-11-11. Resolves brief assumption A-1 (engine/library).
- **Context:** The game needs:
  - a window and mouse/keyboard events, with cursor hide and grab
  - hardware-accelerated 2D sprites with transforms
  - an offscreen render target plus a fragment shader (grain, vignette, grade)
  - CPU access to image pixels (for alpha masks)
  - text rendering
  - SFX playback plus a streamed loop

  The brief also asks for RAII ownership and no unnecessary libraries.
- **Decision:**
  - SFML 3.0.x, using the Graphics, Window, System, and Audio modules.
  - The exact patch release is chosen and pinned at M0.
- **Alternatives:**

  | Option | Why not |
  |---|---|
  | **SDL2 + SDL_image + SDL_mixer + SDL_ttf** | Four libraries. The SDL2 2D renderer has no custom shaders, so post-processing would need raw OpenGL. C API, so we would write RAII wrappers for everything |
  | **SDL3** (GPU API) | Shaders need offline compilation per backend, which is heavy for one full-screen pass. Still several satellite libraries |
  | **raylib** | Very capable and simple, but a C API with hidden global state (window, audio device) and no RAII. Works against the brief's ownership goals. Reasonable second choice |
  | **Raw Win32 + Direct3D 11 + XAudio2** | Far more code than the game warrants |
  | **Full engine** (Unreal, Godot) | Not a C++ architecture exercise; heavy for a single screen |
- **Consequences:**
  - One dependency covers every requirement.
  - SFML 3 types are move-only RAII objects, which fits the ownership model.
  - SFML 3 is newer than most online examples (risk R-8).
  - SFML 3 needs C++17, and we use C++20.

## D-003 — SFML via pinned FetchContent, static link

- **Status:** Accepted, 2026-11-11.
- **Context:**
  - Reproducible builds need an exact dependency version.
  - Shipping should be simple: one exe and an assets folder.
  - Neither vcpkg nor Conan is installed.
- **Decision:**
  - `FetchContent_Declare(SFML …)` pinned to a release tag, with the commit hash recorded in a CMake comment.
  - `BUILD_SHARED_LIBS OFF` and `SFML_BUILD_NETWORK OFF`.
  - If the network fetch fails because of TLS interception (proxy or antivirus), point `FETCHCONTENT_SOURCE_DIR_SFML` at a local source archive.
- **Alternatives:**
  - vcpkg manifest mode, which means installing and bootstrapping another tool.
  - Prebuilt SFML binaries, which must match the MSVC version and leave DLL copying to manage.
  - Vendoring the source into git, which makes the repository heavy.
- **Consequences:**
  - The first configure is slow while SFML and its sub-dependencies build. Later builds are cached.
  - Static linking avoids DLL-shipping problems.

## D-004 — Plain composition; no ECS, scene graph, or singletons

- **Status:** Accepted, 2026-11-11.
- **Context:**
  - The game has five targets of one kind, one lane, one static scene, and four app screens.
  - The request explicitly says to avoid ECS and unnecessary patterns.
- **Decision:**
  - Concrete classes composed by value. `Application` owns everything; children borrow by `const&`.
  - No inheritance hierarchies; `std::variant` where heterogeneity is needed (events, input).
  - No service locator and no global or static mutable state.
- **Alternatives:**
  - EnTT or a hand-made ECS.
  - A `Scene`/`Node` graph.
  - A virtual `IState` State pattern.

  Each adds indirection with no second use case to justify it.
- **Consequences:**
  - The architecture is easy to read and debug.
  - Adding a second lane or a new target type means adding a value or a field, not a framework.
  - If the game ever grew to dozens of heterogeneous entity types, this decision should be revisited.

## D-005 — Gameplay logic in an SFML-free library

- **Status:** Accepted, 2026-11-11.
- **Context:**
  - Gameplay rules (timer, cooldown, hit testing, scoring, state transitions) must be testable without a window, a GPU, or real time.
  - The brief's acceptance criteria are mostly about these rules.
- **Decision:**
  - The `gallery_logic` static library contains `core/`, `game/`, `flow/`, and `view/`.
  - It does not link SFML and uses its own `Vec2`/`Rect`, which are converted at the shell boundary.
- **Alternatives:**
  - Use `sf::Vector2f` in logic for convenience. This couples tests to SFML, and the convenience is small.
- **Consequences:**
  - The compiler enforces the boundary.
  - Conversion helpers (`toSf`, `fromSf`) live in the shell.
  - All unit and scenario tests run headless in milliseconds.

## D-006 — 1920×1080 logical resolution with letterboxing

- **Status:** Accepted, 2026-11-11. Implements brief §2.
- **Context:**
  - Art is composed for one framing.
  - Gameplay must behave identically at any window size.
  - The brief accepts letterboxing for non-16:9 windows in the MVP.
- **Decision:**
  - All logic and layout are in a 1920×1080 logical space.
  - `Viewport::fit` computes a uniform scale and centered offset. Bars fill the remaining space.
  - `Viewport` is the only window↔logical conversion. SFML's `mapPixelToCoords` is not used for gameplay, so there is a single tested source of truth.
- **Alternatives:**
  - Stretch, which distorts the photos.
  - Expand the visible area on wide screens, which needs extra art (brief enhancement, tier 1).
  - Integer scaling only, which wastes space at common resolutions.
- **Consequences:**
  - Black bars appear on non-16:9 displays.
  - Hit testing and layout never deal with physical pixels.

## D-007 — Variable timestep with 100 ms clamp; no interpolation

- **Status:** Accepted, 2026-11-11.
- **Context:**
  - Movement must be frame-rate independent.
  - What the player sees must match exactly what they can hit.
  - All motion is constant-velocity along a line, and all timers are linear.
- **Decision:**
  - One `update(dt)` per frame, with `dt` from a monotonic clock clamped to 100 ms.
  - No fixed-step accumulator and no render interpolation.
- **Alternatives:**
  - **Fixed timestep with accumulator and interpolation.** The standard choice when integration error or physics stability matter, and neither applies here. Interpolation would draw ducks where the simulation is not, which breaks "hit what you see" (D-008).
  - **Fixed timestep without interpolation.** Visible stutter when the display rate doesn't match the step rate.
  - **Unclamped `dt`.** Ducks teleport and the timer burns after a window drag.
- **Consequences:**
  - A test asserts identical outcomes at 30/60/144/240 Hz and with jittered `dt`.
  - If non-linear motion is ever added (bobbing, acceleration), use closed-form functions of time, or revisit this decision.
  - Hitches over 100 ms slow game time slightly, which is accepted.

## D-008 — Shots resolve before update

- **Status:** Accepted, 2026-11-11.
- **Context:**
  - A click is a reaction to the frame the player saw.
  - Resolving after `update` would test against positions the player hasn't seen yet, which is up to one frame of drift (≈4 px at 60 FPS for the planned speed).
- **Decision:**
  - The loop polls and handles input first, against the state that was last rendered. Then it updates, then renders.
  - The shot's audio and visual feedback events come out of the same frame.
- **Consequences:**
  - "What you see is what you hit."
  - The G-28 boundary works naturally: a click in the frame that crosses zero is handled while the phase is still `Live`.

## D-009 — Per-pixel alpha-mask hit detection

- **Status:** Accepted, 2026-11-11. Implements brief G-16.
- **Context:**
  - Hits must match the painted duck silhouette, not a rectangle.
  - The duck art is a photographic PNG with alpha.
- **Decision:**
  - At load time, build an `AlphaMask` (packed bits, alpha ≥ threshold, default 128) from the same `sf::Image` used to create the texture.
  - Hit testing transforms the logical aim point into image space using the duck's pose, then samples the mask.
- **Alternatives:**
  - Hand-authored polygons, which drift whenever the art changes.
  - Bounding boxes, which the brief rules out.
  - GPU picking, which is over-engineered and adds a readback stall.
- **Consequences:**
  - Art changes update hitboxes automatically.
  - Masks are tiny (about 64 KB for 512×1024).
  - Pixel-perfect hits may feel harsh on thin parts such as the bill; a dilation radius can be added to `fromRgba` if playtests say so.

## D-010 — Per-frame event buffer of `std::variant`

- **Status:** Accepted, 2026-11-11.
- **Context:** Audio and visual effects must react to gameplay moments (shot, hit, miss, go, low time, time up) without logic depending on them.
- **Decision:**
  - `Round`/`Game` append `GameEvent` values to a `std::vector` that is reserved once.
  - The shell reads `events()` after `update`, dispatches them to `AudioSystem` and `Renderer` with `std::visit`, then calls `clearEvents()`.
- **Alternatives:**
  - Observer/callback registration, which brings lifetime hazards, hidden control flow, and harder testing.
  - Presentation polling for state changes, which is fragile because a hit and a re-rise within one frame could be missed.
- **Consequences:**
  - Tests assert on emitted events directly.
  - Ordering within a frame is explicit.

## D-011 — App state machine as an enum inside a pure `Game`

- **Status:** Accepted, 2026-11-11.
- **Context:** There are five states (Title, Playing, Paused, Results, Exiting) with fewer than 20 transitions.
- **Decision:**
  - `enum class AppState` plus a `switch` in `Game::handle` and `Game::update`.
  - `Game` lives in the logic library, so every transition is unit-testable.
  - The shell applies side effects (cursor grab and visibility, window close) based on `game.state()`.
- **Alternatives:**
  - A State pattern with `IState` subclasses, which is more files and indirection for five states.
  - A `std::variant` of state structs, which would be useful if states carried much data, and they don't.
- **Consequences:**
  - The whole flow fits on one screen of code.
  - If it grows past about 8 states or acquires per-state data, revisit.

## D-012 — Compiled-in asset manifest; load all at startup; fail fast

- **Status:** Accepted, 2026-11-11.
- **Context:**
  - There are roughly 15 images, 10 sounds, 1 font, and 1 shader, with an estimated load time under 1 s.
  - Restart must never load anything (G-32).
  - Missing art should be obvious immediately, not mid-round.
- **Decision:**
  - A `constexpr` table of `{AssetId, path, kind, required}`.
  - `AssetStore` loads everything in its constructor, collects every failure, and throws once with the full list.
  - Paths are relative to the executable directory.
  - A unit test checks that every manifest path exists in the source tree.
- **Alternatives:**
  - A JSON manifest, which needs a parser dependency and has no runtime benefit.
  - Lazy loading, which can stall mid-round.
  - Embedding assets in the exe as resources, which is awkward while art changes frequently and is possible later.
- **Consequences:**
  - Adding an asset means editing the enum and the table.
  - The game cannot start with partial required assets, which is intended.

## D-013 — Exceptions only at startup boundaries; cosmetic features degrade

- **Status:** Accepted, 2026-11-11.
- **Context:**
  - Startup failures need a clear message for a non-technical player.
  - The frame loop must be predictable.
  - Some features (shader, audio) are cosmetic.
- **Decision:**
  - Constructors throw `StartupError`, which aggregates all problems found. `main` catches it, logs it, shows a Win32 message box, and exits with code 1.
  - Per-frame functions are designed not to throw.
  - A missing shader support or audio device gives a warning, and the game continues without post-processing or sound.
  - Logic expresses normal negative outcomes (rejected shot, miss) as return values and events, never as errors.
- **Alternatives:**
  - `Result<T>` everywhere, as in the System Resource Monitor project. Viable, but SFML constructors and RAII make exceptions the natural fit at load time, and the hot path has no fallible operations to wrap.
- **Consequences:** Error paths are concentrated in `main.cpp`, `AssetStore`, `PostProcess`, and `AudioSystem`.

## D-014 — Post-process covers world layers only; HUD and reticle drawn after

- **Status:** Accepted, 2026-11-11. **Amends brief §6.1** (which needs the owner's approval to update).
- **Context:** The brief is inconsistent on this point.
  - §6.1 says post-processing applies to layers 1–9, which includes the HUD.
  - §7.5 says grain must not be visible on the reticle **or HUD numbers**.
  - AV-4 requires that grain not affect the reticle.
- **Decision:**
  - World layers (sky through rifle and muzzle effects) render into the post-processed target.
  - The HUD, overlays, and reticle are drawn after, at full clarity.
  - To keep the HUD looking physical, its art (painted boards) is pre-graded, so it carries the warm look without live grain.
- **Alternatives:**
  - Apply grade but not grain to the HUD, which needs two passes or a mask channel and costs more for a subtle difference.
- **Consequences:**
  - The HUD and score are always perfectly legible (AV-6).
  - The brief should be updated to say "layers 1–8".

## D-015 — Aim point clamped to the logical play area

- **Status:** Accepted, 2026-11-11.
- **Context:**
  - Cursor grab confines the mouse to the *window*, but a letterboxed window has bars outside the play area.
  - The brief requires that the shot land at the reticle.
- **Decision:**
  - `Game` clamps the incoming logical aim to `[0,1920)×[0,1080)`.
  - The reticle is drawn at the clamped point, and clicks fire at it.
- **Alternatives:**
  - Ignore clicks in the bars. The reticle would sit at the edge while clicks do nothing, which is confusing.
  - Hide the reticle in the bars, which loses the aim indicator.
- **Consequences:** Clicks in the bars are counted as shots and almost always miss. This matches the reticle, so it is honest.

## D-016 — Offscreen target sized to the viewport's pixels

- **Status:** Accepted, 2026-11-11.
- **Context:**
  - Post-processing needs the world rendered offscreen.
  - Rendering at a fixed 1920×1080 and scaling would blur at 1440p/4K and waste effort at 720p.
- **Decision:**
  - The `RenderTexture` is the size of `viewport.pixelRect()`, with an `sf::View` of logical 1920×1080. It is recreated lazily, at most once per frame, when the size changes.
  - The result is blitted 1:1 at the viewport offset.
- **Consequences:**
  - Output is crisp at any size.
  - Grain is specified per output pixel, so its size is independent of resolution.
  - Some VRAM churn occurs during interactive resizing, which is acceptable.

## D-017 — Hand-rolled header-only test framework

- **Status:** Accepted, 2026-11-11.
- **Context:**
  - Logic tests need assertions, test registration, and a nonzero exit code on failure.
  - A small hand-rolled `tests/support/test_framework.hpp` is enough and adds no dependency.
  - The brief says to avoid additional libraries without a demonstrated need.
- **Decision:**
  - Reuse the same style: `TEST_CASE`, `CHECK`, `CHECK_EQ`, `CHECK_NEAR`, and a runner.
  - One `gallery_tests` executable, registered with CTest.
- **Alternatives:** Catch2 or doctest. Nicer output, but an extra fetched dependency for a test suite of about 100 cases.
- **Consequences:** Swapping to doctest later is a mechanical change if reporting needs grow.

## D-018 — Software-drawn reticle, hardware cursor as fallback

- **Status:** **Provisional.** To be confirmed by the M2 latency measurement (AC-3, AC-14).
- **Context:**
  - The reticle must track the mouse 1:1 without perceptible lag.
  - A sprite drawn by the game lags the OS cursor by about one to two frames under vsync.
  - A hardware cursor (`sf::Cursor` from an image) has the lowest latency but cannot animate the recoil "kick" (G-9) and is limited in size on some systems.
- **Decision:**
  - The reticle is drawn by the game, last in the frame, at the latest polled mouse position, with no smoothing.
  - If the M2 measurement fails, first try vsync off with a frame limit. If that still fails, switch to a hardware cursor and express recoil through rifle motion only.
- **Consequences:** The decision may flip after measurement. `ReticleRenderer` is isolated so the swap is local.

## D-019 — Fixed SFX voice pool; streamed ambience

- **Status:** Accepted, 2026-11-11.
- **Context:**
  - Rapid fire can overlap gunshot, hit, and miss sounds.
  - Allocating sound objects per event is wasteful.
  - The ambience loop is long.
- **Decision:**
  - `AudioSystem` holds 16 `sf::Sound` voices and picks a free one round-robin. If none is free, it steals the oldest.
  - SFX are preloaded `sf::SoundBuffer`s (WAV).
  - The ambience is an `sf::Music` stream (OGG, looping).
  - Sound buffers live in `AssetStore`, which is declared before `AudioSystem` so the buffers outlive the voices.
- **Consequences:**
  - No allocation on the event path.
  - Voice stealing is inaudible at a 250 ms fire cooldown.

## D-020 — Asset manifest at `assets/ASSET_MANIFEST.md`

- **Status:** Accepted, 2026-11-11. Supersedes the `docs/ASSET_SOURCES.md` named in brief §7.7 and AS-2.
- **Context:** Provenance, license, modifications, and status must be recorded per asset. The manifest should travel with the assets it describes.
- **Decision:**
  - One Markdown manifest lives in `assets/`, with per-asset evidence files in `assets/licenses/`.
  - Asset IDs match the compiled-in asset table (D-012).
  - Rules and specifications live in `docs/ASSET_GUIDE.md`.
- **Alternatives:**
  - A CSV or JSON manifest. It is machine-friendlier but harder to review by eye. Revisit if the contract test needs to parse more than the status column.
- **Consequences:** The brief's AS-2 criterion now refers to this file.

## D-021 — Originals and masters in `asset-src/`, separate from runtime `assets/`

- **Status:** Accepted, 2026-11-11.
- **Context:**
  - Originals must be preserved byte-for-byte.
  - Editing happens in large layered files.
  - Only optimized runtime files should ship.
- **Decision:**
  - `asset-src/originals/` holds untouched downloads and RAW files, each with its SHA-256 recorded in the manifest.
  - `asset-src/work/` holds layered masters.
  - `asset-src/reference/` holds mood references.
  - Only `assets/` is copied next to the executable.
- **Consequences:**
  - `asset-src/` may reach several GB.
  - The storage method (Git LFS, external storage, or don't keep) is an open owner question (Asset Guide Q-4).

## D-022 — Film grain from a scanned plate, not procedural noise

- **Status:** Accepted, 2026-11-11. Amends ARCHITECTURE R-7 mitigation.
- **Context:** The asset rules forbid procedural textures standing in for photographic material. Real film grain has structure that hash noise lacks.
- **Decision:**
  - The post-process shader samples a tileable grayscale scan of real film grain (`OV_GRAIN`), offset randomly each frame.
  - The vignette remains an analytic optical falloff (a lens property, not an image).
- **Consequences:**
  - One extra 512×512 texture.
  - Tiling must be checked for visible repetition. Mitigate by changing the offset and scale every frame.

## D-023 — Release builds blocked by placeholder, missing, or unverified MVP assets

- **Status:** Accepted, 2026-11-11.
- **Context:** Placeholders must never ship by accident (Asset Guide §5.4).
- **Decision:** The asset contract test fails in the Release configuration when either of these is true:
  - Any MVP-priority manifest entry is not `READY`.
  - Any file under `assets/` has the `TEMP_` prefix.

  Debug builds only warn.
- **Consequences:**
  - The contract test reads the manifest's status column, so its table format becomes a lightweight interface.
  - Changes to the column layout must update the test.

## D-024 — SFML pinned to 3.0.2 by hashed source archive; raylib re-checked and not adopted

- **Status:** Accepted, 2026-11-11 (M0). Completes D-002 ("exact patch release chosen at M0") and D-003.
- **Context:**
  - The M0 implementation request named raylib, conditional on it being confirmed compatible during repository inspection.
  - The approved architecture had already chosen SFML 3 (D-002) and listed raylib as the second choice. Its C API with hidden global state conflicts with the ownership model (§3), and adopting it would invalidate D-002, D-003, D-005, D-016, D-019 and most of ARCHITECTURE §2.3.
  - SFML tags available on 2026-11-11: 3.0.0, 3.0.1, 3.0.2, 3.1.0.
- **Decision:**
  - Keep SFML 3. raylib is not adopted.
  - Pin **3.0.2** (the latest patch of the 3.0 line that D-002 approved), commit `0fa201c9…`.
  - Fetch it as the GitHub archive of that commit with `URL_HASH SHA256`, not as a git tag. A tag can be moved; a hashed archive cannot change without the build failing.
  - The archive is declared `SYSTEM`, so `/WX` never applies to SFML headers.
- **Alternatives:**
  - SFML 3.1.0: newer minor release, outside the approved 3.0.x line. Revisit if a 3.1 feature is needed.
  - `GIT_TAG` with a commit hash: also immutable, but clones the full repository history.
- **Consequences:**
  - Risk R-4 resolved for now: CMake's download and the sub-dependency git clones both worked. The documented fallback remains (docs/BUILD.md).
  - Sub-dependencies are pinned transitively by SFML 3.0.2's own release tags.

## D-025 — M0 structure refinements

- **Status:** Accepted, 2026-11-11 (M0). Refines ARCHITECTURE §1.1, §3.2 and risk R-6.
- **Decisions:**
  1. **Public headers for `gallery_logic` live in `include/gallery/<module>/`.** Sources stay in `src/logic/`. Shell headers remain private next to their sources in `src/shell/`, because the executable has no public interface.
  2. **`main` owns the `Logger`; `Application` borrows it.** If `Logger` were `Application`'s first member, it would already be destroyed when `main` catches a `StartupError` thrown by the constructor, and the failure could not be logged.
  3. **`AssetPaths` is validated before the window is created.** A missing asset folder fails fast without flashing a window. M1's `AssetStore`, which needs the GL context, goes after the window as §3.2 specifies.
  4. **The initial window size scales with display DPI** (`gallery::initialWindowSize`, unit-tested): 1280×720 at 100 % scaling, multiplied by the DPI scale, and fitted within 85 % of the desktop. Found during M0 verification: on a 250 % display a fixed 1280×720 physical-pixel window was tiny.
  5. **The exe manifest also sets the UTF-8 active code page**, so `argv` and narrow paths are UTF-8.
- **Consequences:** R-6 is resolved for startup sizing and rendering. Interactive moves between monitors with different DPI are still untested.

## D-026 — M1 booth composition: layer groups, stand-ins, startup policy

- **Status:** Accepted, 2026-11-11 (M1). Refines ARCHITECTURE §5.3, D-012, and Asset Guide §5.4.
- **Context:**
  - The M1 request asked for eight visual layers: background and booth interior, decorations and signage, target track, ducks, counter, rifle, crosshair and UI, and shot effects.
  - Correct occlusion needs more draw depths than eight. The rail sits behind the ducks, the wave cutout in front of them, and the booth posts in front of both, so ducks wrap out of sight.
  - Every photographic asset is still `MISSING`. D-012 would refuse to start, so the composition could not be inspected.
- **Decisions:**
  1. **Draw passes and layer groups are separate.** `gallery::kDrawPasses` (in `SceneLayout.hpp`, unit-tested) lists 17 passes back to front. Each pass belongs to one of the eight `LayerGroup`s, and keys 1–8 toggle the groups. Draw order:
     - plates → backboard → miss marks → rail → ducks → wave cutout → booth frame → sign and bulbs → counter → rifle → muzzle puff
     - then, after post-processing (D-014): score board → timer board → crosshair
  2. **Composition lives in pure logic.** Every slot is in logical units in `gallery::layout`. Tests enforce the brief's layout rules:
     - the lane band is 40–60 % of screen height
     - the rifle never covers the lane at any aim or recoil
     - HUD boards stay off the lane
     - ducks wrap only while hidden behind a post
     - the counter fills about the bottom 20 %
  3. **Photographs are never distorted.** `placeImage` offers only uniform-scale modes: cover, contain, fit-width, and fit-height.
  4. **Missing MVP images render as labelled flat-grey layout cards** of the slot's size and rough silhouette. This is Asset Guide §5.4's "flat mid-grey card", drawn at runtime rather than stored as `TEMP_` files. Every card names the missing ID and its path.
     - Glows, film grain, and other photographic effects get **no** synthetic substitute: no gradient glow, no procedural grain (D-022).
     - E1 assets are simply not drawn when missing.
     - The reticle uses a TEMP functional graphic until Q-2 is answered.
  5. **Startup policy.**
     - Stand-ins are on by default in Debug, and in any build with `--stand-ins`.
     - Otherwise D-012 applies: Release refuses to start, listing every missing MVP asset.
     - The asset contract test enforces the D-023 release gate in the Release CTest configuration, so `ctest --preset release` **fails until the MVP assets are READY**. This is intended.
  6. **Effects are visual-only in M1.** A click fires a cosmetic shot: muzzle puff at the rifle's muzzle, recoil, and a miss mark when the aim is on the backboard. Duck positions are a temporary pure function of time. `DuckLane` and hit testing replace them in M3.
- **Consequences:** Dropping a correctly named photograph into `assets/` replaces its stand-in with no code change. Updating the manifest status keeps the contract test green.

---

## D-027 — Duck target system

- **Status:** Proposed, 2026-11-11. Implemented and verified. Item 1 amends brief G-12/G-14/AC-5 and ARCHITECTURE §4.3, so it **needs the owner's approval**. The brief's behaviour stays available as `--targets chain`.
- **Context:**
  - The target-system request asks for five *independently* moving targets. Each has its own position, speed, direction, bounds, hit region, active state, hit reaction and re-entry. Speeds and timing vary modestly so the targets are "mechanical but not completely synchronized", and they never leave the visible gallery.
  - The approved brief instead has one chain loop at one speed (G-14). On a shared rail, different speeds mean collisions, which would be physically implausible.
  - No approved target photograph exists. `TGT_DUCK_A/B/C` are all `MISSING`, and generated substitutes are ruled out.
- **Decisions:**
  1. **Default preset `sections`.** The rail across the opening is split into five equal sections with a 20-unit gap between them. Each target rides its own reciprocating carriage:
     - It cruises at constant speed, brakes and accelerates at constant rate over `turnTime` (no instant reversal), and dwells briefly at each end stop.
     - Speeds are 112–131 u/s (±8 %). Dwell, turn time, phase, start direction and minimum down time differ slightly per target. All values are fixed tables, with no runtime randomness.
     - Sections never overlap, so targets cannot collide. `validateLane` enforces this.
  2. **Preset `chain`** is the brief verbatim: one loop, 0.25 lane widths/s, even spacing, wrap behind the posts.
  3. **Configuration is data.** `DuckConfig` / `LaneConfig` in `gallery/game/DuckConfig.hpp` hold every number. `validateLane()` checks a configuration against the booth (bounds inside the opening, no collisions, sane numbers), and the game refuses to start on an invalid one. Presets are compiled in (D-012); `--targets` and `--reaction` select them.
  4. **Motion is a pure function of a per-target clock** (`DuckMotion`). State transitions are resolved at their exact instant inside a frame, with the leftover time carried into the next state. Results are therefore independent of the frame rate, which the tests and the `--sim-dt` runs at 30/60/144/240 Hz confirm.
  5. **State machine:** Upright (active) → Falling (`fallTime` 0.2 s) → Down → Rising (`riseTime` 0.25 s) → Upright. The carriage keeps moving throughout.
     - Re-entry (`RiseTrigger::AtLaneEnd`): the target rises at the first end stop (sections) or wrap (chain) after `minDownTime` (≥ 2 s).
  6. **Hit reactions:**
     - `FlipBack` (default): folds back on its hinge (projected height × cos), slams flat and bounces once.
     - `TiltDrop`: jolts 4° askew and sinks behind the solid part of the wave cutout (`layout::kWaveSolidTop`), without showing below it.
  7. **One transform for drawing and hitting (R-3).** `duckImageToLogical(DuckPose, DuckShape)` is drawn by the renderer and inverted by `DuckLane::hitTest`. The alpha mask (D-009) is built from the exact image that is drawn:
     - the photograph when present;
     - otherwise the stand-in silhouette, rendered once into a texture.
     So a photo dropped into `assets/images/targets/` changes both the look and the hit region, with no code change.
  8. **Debug/diagnostics:**
     - F3 cycles off → layout view → target view (bounds brackets, posed hit masks, aim-over highlight, state table).
     - K knocks down the next upright target to preview reactions. It is not a shot, and shooting is not implemented.
     - `--knock N` does the same at frame N; `--inspect-targets` starts in the target view; `--sim-dt S` feeds fixed frame times; `--fps N` caps the frame rate.
     - Transitions and an exit snapshot go to `carnival.log`.
- **Known limits:**
  - **Rise visibility in the chain preset.** The booth posts are only 12 units wider than a stand-in duck. A chain duck that starts rising at the hidden wrap point is still rising (about 95 units of travel) as it emerges. ARCHITECTURE §4.3's "rising runs behind the posts" would need wider posts or a slower chain.
  - **Section travel is short.** About 156 units per target with the stand-in width; a wider photograph shortens it. If playtests find this cramped, the options are fewer sections, a second rail at another depth (tier-2, brief §9), or the chain preset.

---

## D-028 — Shooting interaction

- **Status:** Accepted, 2026-11-11. Implements ARCHITECTURE §4.4 ahead of the M4 round clock.
- **Decisions:**
  1. **Rules live in logic.** `gallery/game/Shooting.hpp` holds them:
     - `Rifle`: 250 ms cooldown (G-8). It is checked with a 1 ns tolerance so cooldowns summed from frame deltas open on time.
     - `RoundStats`: hits, misses, score, and accuracy with integer half-up rounding (`nullopt` → "—", G-23).
     - `Round::fire(aim)`: the only mutator API for shooting. `Round` owns the `DuckLane`.
     - The shell holds no shooting rule; the renderer's old cosmetic cooldown is gone.
  2. **Hitscan.** A shot resolves on the frame its click arrives, before `update()` (D-008), against the alpha mask of the front-most **upright** target (D-009, D-027).
     - `DuckLane::knockDown` succeeds only on an upright target. A second shot at a falling, down or rising target is therefore a miss, and points are awarded exactly once.
     - There is no projectile.
  3. **Rejected clicks** (cooldown) are not shots. They change no stats and emit no events (AC-4). One press is one shot: SFML delivers presses, not held state, and nothing auto-fires.
  4. **Aim.** The press position goes through the one conversion: `Viewport::toLogical`, then `clampToLogical` (D-006, D-015). The reticle is drawn at that same point; its kick widens it, but its centre never moves. Only the rifle prop swivels, a few degrees as in M1; no background layer moves with the aim.
  5. **Feedback is event-driven.** `ShotFired`, `DuckHit` and `ShotMissed` go to `Renderer::consume` in the same frame:
     - every shot: muzzle puff, rifle recoil, reticle kick;
     - hit: "+10" tag rising from the impact point, reticle warms to a hit colour, and the target's D-027 reaction;
     - miss: pellet mark on the backboard that pops in and fades over 2 s (only where there is backboard), with the score untouched.
  6. **HUD.** The score board shows the live score, plus "HITS · MISSES · ACC" on one line.
- **Not done here:**
  - Gunshot and hit sounds (G-9): there is no `AudioSystem` yet, and every SFX asset is `MISSING`.
  - A muzzle *flash*: no photographed flash asset exists, and D-026 forbids synthetic glows. The muzzle effect is `OV_MUZZLE_PUFF`, currently a grey stand-in.
  - Round phases, the timer, and shot gating outside a round (G-11, G-26, G-28) are M4.

---

## D-029 — Round clock, application states, and the painted-board interface

- **Status:** Accepted, 2026-11-11. Implements ARCHITECTURE §4.1, §4.2 and brief G-11, G-24..G-33 (M4 scope, without audio). Refines D-011 and D-026.
- **Context:** The game needs a main menu, an active round, a pause, and a results screen with restart, plus a 60-second timer, with no scattered flags, no leaks, no duplicated input and no timer drift across transitions.
- **Decisions:**
  1. **One enum, one owner.** `gallery::Game` (logic/flow) holds `AppState {Title, Playing, Paused, Results, Exiting}`, the `Round`, and the session best. Every input goes through `Game::handle`, which acts on it for the *current* state only, once. The round's own progress is `RoundClock`'s phase. The old debug "freeze targets" flag (`Round::setTargetsPaused`, key P) is gone; P now pauses the game.
  2. **`RoundClock`** (logic/game): Countdown 1.5 s → Live 60 s → Ending 1 s → Finished. A frame that crosses a boundary is split at the exact instant, so the targets stop at zero to the nanosecond at any frame rate. The board shows whole seconds rounded up (60 … 1, then 0). Only `Playing` advances it; Title, Paused and Results freeze clock, targets and rifle cooldown.
  3. **Shots are gated twice.** Clicks on boards never reach `Round::fire` (they press placards or resume). Inside `Playing`, `fire` rejects clicks outside Live (`RejectReason::NotLive`) before touching the rifle. Input is handled before the update, so a click in the frame that crosses zero still counts (G-28).
  4. **Restart = `Round::reset()`** in place: targets home, rifle ready, stats zero, clock to Countdown, pending events dropped, then one `RoundReset` event, which the renderer uses to clear effects. No allocation, no loading. Going to the main menu also resets, so an abandoned round is discarded and never scored. (The architecture's `RoundStarted` event is named `RoundReset` because it also fires on the way to the title.)
  5. **Events:** `RoundReset`, `Go`, `ShotFired`, `DuckHit`, `ShotMissed`, `LowTimeTick{10..1}`, `TimeUp`, `RoundFinished{stats}`, in the order they happen within a frame.
  6. **Buttons are data in logic.** `gallery::ui` holds the board rect and each screen's placard rects; `Game` hit-tests them and `ScreenRenderer` draws exactly those rects.
     - Title: Start Round (Enter/Space), Quit (Esc).
     - Paused: Resume (Space/Enter/P or a click anywhere off the placards), Restart Round (R), Main Menu (Esc).
     - Results: Play Again (Enter/Space/R), Main Menu; Esc quits (G-33).
  7. **Cursor policy from state.** The OS pointer is hidden and confined only in `Playing` with focus; it is visible and free on every board and on focus loss (G-3, G-4). Focus loss pauses a running round; focus gain never auto-resumes (G-29). The reticle is drawn only in `Playing`.
  8. **Interface look: physical objects, one theme** (`shell/render/Theme.hpp`).
     - Every overlay is the `UI_PANEL` board photograph with runtime sign-writer lettering; while it is missing, the D-026 labelled grey card. The booth stays visible behind it under a flat warm dimming wash.
     - Palette from brief §7.3: ink brown-black lettering, cream, faded carnival red and mustard gold as accents only. No gradients, glows, bevels or drop shadows.
     - One display face, one type scale (numerals 72, headings 58, rows 30, body 25, captions 19 letter-spaced, footnotes 16).
     - Placards are ink-lined panels at sign size (380×52), not full-width slabs; the one under the pointer is painted solid ink with cream lettering and its indicator lamp lit. Key equivalents are painted small at the right end.
     - Results is a painted tally with dot leaders; a new session best gets a small red ribbon.
     - Illuminated indicators: placard lamps and a timer-board lamp (steady while the clock runs, flashing each second in the final ten, dark otherwise). They use `DEC_BULB_LIT` tinted on/off; while it is missing, flat grey discs (dark off, light on). No glow is ever drawn (D-026).
     - Low time turns the timer numerals red (G-27). "Ready…", "Go!" and "Time!" are painted letters over the backboard.
  9. **Fonts.** While `FNT_DISPLAY` is missing, the display face falls back to an installed slab serif (Rockwell Bold, then Georgia Bold, then Segoe UI), read from the local Windows font folder at run time and never copied into `assets/`. Stand-in labels and the inspection overlay use a separate utility face (Segoe UI) so a "MISSING" card can never pass for lettering.
  10. **Automated runs.** `--frames` or any scripted input (`--fire/--click`, `--key N:NAME`, `--focus-lost`, `--focus-gained`, `--close`) makes a run deterministic: real mouse, keyboard and focus events are ignored, and the pointer is never confined. Script steps are keyed to loop iterations, so each fires exactly once, even while the window is minimised.
- **Consequences:**
  - Every transition is unit-tested in `tests/game_flow_tests.cpp`, the clock in `tests/round_clock_tests.cpp` (134 tests in total).
  - A scripted 12-round session ran every transition. All 12 rounds ended at t = 61.500 s with identical statistics. Every pause resumed at exactly the remaining time it froze at. Private memory stayed flat (71–77 MB), and the Debug CRT leak report was empty.
  - Not done: audio cues (Go bell, low-time ticks, end buzzer). The events exist; there is no `AudioSystem`, and every SFX asset is `MISSING` (M5).
  - `Rockwell` lacks the middle-dot glyph, so separators in display lettering are spaces or em dashes. The real `FNT_DISPLAY` must be checked for the glyphs the UI uses: — … ! % : and digits.

---

## D-030 — Audio: sourcing, soundscape rules, `AudioSystem`

- **Status:** Accepted, 2026-11-12. Implements brief §8, G-9, G-17, G-26..G-28 (audio parts), ARCHITECTURE §2.3 `AudioSystem`, D-019. Every sound asset is `IN_PROGRESS` until the owner's listening review.
- **Context:**
  - The atmosphere request asks for authentic carnival sound, not generic arcade effects: the mounted rifle, target mechanism, impacts, resets, score confirmation, vintage ambience, optional music. Assets must be licensed and documented; audio must be centrally managed with volume controls and safe cleanup.
  - Asset Guide §4.2 requires recordings of real sounds, per-sound licences (CC0 or CC BY), and §3.2 provenance checks.
- **Decisions:**
  1. **Sources.** Thirteen runtime sounds come from twelve Freesound recordings, all CC0, each recorded by its uploader (equipment or own making stated on the page). Evaluated and not used: Commons fairground recordings under CC BY-SA (tier C; the Vienna Prater shooting-range air rifle, EV-A01, is the best match and awaits an owner decision), MuseScore renderings (synthesised), Craig Smith's digitised studio libraries (uploader ≠ creator, as EV-05), and ambiences with intelligible speech or recognisable music. Manifest §2.1 records all ten.
  2. **Originals and edits are reproducible.** The files obtained are Freesound's public HQ MP3 previews (the lossless originals need a login), stored byte-for-byte in `asset-src/originals/audio/` with SHA-256. Every runtime file is cut by one command line in `asset-src/work/audio/prep.sh`, using the project tool `tools/audio_prep.cpp` (target `audio_prep`, not built by default, never shipped). Its only operations: segment, leading-silence trim, short fades, mono downmix, peak normalisation, equal-power loop crossfade. Segments were chosen by level and transient analysis (`env`, `onsets`, `quiet`), **not by ear**, which is why nothing is `READY`.
  3. **Music stays out for now.** The only fully cleared candidate (EV-A10: a CC0 carousel band-organ recording whose recordist names one of five songs as "Over the Waves", 1888) could not be matched to a segment without listening, and the preview is truncated. Asset Guide §3.6 says: when in doubt, no music. `MUS_CALLIOPE` becomes **E1 (optional)**, its code path is complete, and audition cuts are in `asset-src/work/audio/audition/`.
  4. **Catalog.** Sounds join the compiled-in table (D-012) as `AssetKind::Sound` (decoded once at startup into `sf::SoundBuffer`s owned by `AssetStore`) or `AssetKind::Stream` (ambience, music: path verified to decode at startup, streamed by `sf::Music`). New E1 entries: `SFX_DUCK_RESET`, `SFX_SCORE_TICK`, `SFX_RESULTS`, `OV_MUZZLE_FLASH`. Missing sounds are silent, never substituted; under the Release policy missing MVP sounds refuse startup like images.
  5. **Rules in logic** (`gallery/audio/Soundscape.hpp`, unit-tested):
     - events → cues: Go → start bell; ShotFired → shot; DuckHit → metal ping + score-board clack 90 ms later; ShotMissed → wood knock; LowTimeTick → clock tick; TimeUp → end bell twice, 0.4 s apart; RoundFinished → register bell; RoundReset → silence;
     - target transitions → cues: Falling→Down → the hinged slam; Down→Rising → the hinge rattle;
     - a board action (a press or key that changed the app state) → wooden click; focus changes and window close are not clicks;
     - beds: the mechanism loop runs exactly while targets move; ambience never stops on a board but settles to 50 % paused and 35 % unfocused; music (if installed) sits under it;
     - mix table (one place) and per-play pitch/level variation from a seeded xorshift (±3–6 %; clock and bells never vary).
  6. **`AudioSystem`** (shell): 16 voices round-robin, oldest stolen (D-019); a fixed 16-slot delayed-cue queue; three faded beds; no allocation on the event path. It runs in the same frame as the event (G-9). Member order (after `AssetStore` in `Application`) plus an explicit ordered teardown stop every source before its buffer goes. It degrades to silence with one warning on `--no-audio`, no device, or **a device that refuses to open**: SFML opens the device with the first sound object, so `AudioSystem` creates one at startup and checks `sf::PlaybackDevice::getDevice()`.
  7. **Player controls.** M mute, − / = master volume in 10 % steps (perceptual: slider squared), N music on/off; a painted note confirms each change for ~1.6 s. Switches `--volume N`, `--mute`, `--no-music`, `--no-audio`, `--log-audio` (one log line per cue). Settings are per session (A-11).
- **Verification:** 13 soundscape tests, including a whole round in Application frame order and ten identical back-to-back rounds. The asset contract now checks the licence evidence files and the WAV/Ogg format. A scripted, muted 80 s session logged exactly the expected cues (73 shots = 14 hits + 59 misses, 14 slams, 10 ticks, 2 end bells, 1 results, 6 board clicks).
- **Known limit:** in the original test environment, Windows refused to open the output device (`WASAPI: Invalid argument`), even in a minimal SFML program with no game code. So **audible playback has not been verified.** The game now handles that case cleanly. Check with `audio_prep play <file>` (BUILD.md).

## D-031 — Atmosphere: light level, shot flash, environmental motion

- **Status:** Accepted, 2026-11-12. Refines D-026 (no synthetic glows), D-022, brief §7.2, §7.5, §9.2.
- **Context:** The request asks for restrained polish: warm incandescent light, grain, vignette, natural target motion, recoil and muzzle flash, gentle environmental animation, optional flicker, without overloading the scene or making it look like a modern 3D-rendered arcade.
- **Already in place, kept as is:** warm grade and analytic vignette (post-process); scanned-grain hook (D-022, off until `OV_GRAIN` exists, still no procedural grain); targets that brake, dwell and reverse, fold back and bounce (D-027); recoil and reticle kick (D-028); two tired bulbs (M1).
- **Decisions:**
  1. **Booth light level** `ambientLight(t)` (pure, tested): about every 25 s (10–40 s apart) the generator sags for 0.26 s, dimming everything 2.5–5.5 % with a fast drop and slower thermal recovery. The tired bulbs take at most 0.4 % each with them. It is applied as an exposure uniform in the post-process and to every bulb sprite. The first version let the tired bulbs pull the whole scene down ≥ 1 % for 18 % of the time; the test caught it as constant "breathing" and the share was cut by two thirds.
  2. **Shot flash** = a warm exposure lift of the world for 55 ms (+7 % red, +5 % green, +2.5 % blue at the instant), not a drawn glow. An air rifle shows little flame; D-026 forbids synthetic glows. A photographed flash (`OV_MUZZLE_FLASH`, E1) is added on top only if one is ever supplied. The flash ends well inside the 250 ms cooldown, so shots never stack it, and it never touches the reticle or HUD (D-014).
  3. **Bunting sway** (E1 photograph only): < 0.5° about its top edge plus ≤ 2 units of drift, from two slow incommensurate sines.
  4. **`--no-flicker`** gives steady light: no bulb dips, sags or sway, for anyone who finds flicker uncomfortable.
  5. **Not added, on purpose:** extra target judder (motion is already mechanical, and a pose change would touch the hit-mask transform, R-3), procedural smoke, haze, dust, light leaks, lens flares, parallax. Each would be synthetic or busy.

## D-032 — Engineering verification fixes: foreground occlusion, strict font decode, frame-time log

- **Status:** Accepted, 2026-11-12 (engineering verification pass). Refines D-009, D-012, D-013, D-028.
- **Context:** A structured verification (BUILD.md, "Engineering verification 2026-11-12") found three defects and one missing measurement.
- **Decisions:**
  1. **Scenery in front of a target stops the shot (D1).** Hit testing used only the duck's full silhouette, so a click on a booth post scored the `chain` duck hidden behind it (reproduced: `--targets chain --aim 160,562 --fire 111` → "HIT target #3"), and a click on a wave crest scored the duck base it covers (both presets). Now `Renderer::buildForegroundOccluder()` renders the static passes drawn after the ducks (wave cutout, booth frame, header sign, counter) once at startup into a 1920×1080 target; its alpha (≥ 128, as D-009) becomes a `gallery::Occluder`. `Round::targetUnder()` = not occluded AND front-most upright silhouette; `fire()` and the F3 overlay both use it. Rifle, muzzle puff, bunting, bulbs and HUD boards are excluded: they cannot reach the lane (scene_layout_tests). Built from the drawn images, so it stays correct when the photographs arrive.
  2. **A font file that exists but does not open is a startup error (D2),** like every other undecodable asset. It was logged as "MISSING" and the system stand-in font was substituted silently.
  3. **`--no-dialog` holds on an invalid command line (D3).** The flag is pre-scanned; before, a typo in an automated run blocked on a modal message box.
  4. **Frame-time log for AV-7.** `gallery::FrameStats` (0.1 ms histogram, exact max, count over 33 ms) records the unclamped wall time of every rendered frame except the first; `carnival.log` gets one summary line at exit. The frame clock is also restarted when the main loop starts, so boot time is no longer the first frame's delta.
- **Verification:** 11 new tests in `tests/verification_tests.cpp` (163 total).

---
## D-033 — Desktop distribution: preview build, naming, static runtime, self-contained layout

- **Status:** Accepted, 2026-11-12. The owner chose the preview option when asked. Leaves D-023 and D-026 unchanged for the strict Release build.
- **Context:** The game was to be prepared for normal desktop use, but all 20 MVP assets (19 images and the display font) are still MISSING. Under D-023/D-026 a strict Release build refuses to start without them, so it cannot be handed to a player.
- **Decisions:**
  1. **A separate preview build.** CMake option `GALLERY_PREVIEW` (default OFF), set only by the `windows-msvc-preview` configure preset in its own tree `build/windows-msvc-preview/`. It is Release-optimized, starts with the labelled stand-ins without `--stand-ins`, has "(Preview)" in its window title, version `0.1.0-preview`, and the PRERELEASE flag in its version resource. `asset_contract` runs without `--release-gate` in that tree only. The development tree (`windows-msvc`: Debug + strict Release) and its release gate are untouched.
  2. **Product naming.** The exe is `CarnivalShootingGallery.exe` (`OUTPUT_NAME`; the CMake target stays `carnival_gallery`). A version resource fills Explorer's Details tab.
  3. **Static MSVC runtime in the preview tree** (`SFML_USE_STATIC_STD_LIBS`, `CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`), so the exe imports only Windows system DLLs and needs no VC++ Redistributable. The development tree keeps the default DLL runtime.
  4. **Self-contained layout** via `install(... COMPONENT Game)` and CPack ZIP: exe, `assets/` (without `.gitkeep`), `README.md`, `THIRD_PARTY_NOTICES.md`, `licenses/third-party/`. License texts are copied verbatim from the pinned sources at configure time (`cmake/ThirdPartyNotices.cmake`). The component keeps SFML's own install rules out of the package.
  5. **Log fallback.** `carnival.log` stays next to the exe. If that folder is not writable (for example under Program Files), the log goes to `%LOCALAPPDATA%\CarnivalShootingGallery\carnival.log`.
  6. **One reproducible command:** `cmake --workflow --preset preview` (configure → build → test → package into `dist/`).
- **Known exception:** the TEMP display-font fallback (D-029) still reads Rockwell / Georgia / Segoe UI from the Windows font folder. These fonts are not redistributable, and no licensed font has been sourced yet (FNT_DISPLAY MISSING), so they are not bundled.

---
## D-034 — Development keys are Debug-only (independent audit)

- **Status:** Accepted, 2026-11-12 (independent audit, see `docs/AUDIT_REPORT.md`). Restores ARCHITECTURE §7 ("a Debug-only overlay (F3)"); refines D-027 item 8.
- **Context:** The player-facing builds (strict Release and the D-033 preview zip) still acted on F3, 1–8 and K. A stray digit key hid a whole layer group (key 4: every duck) with no on-screen sign, while the hidden ducks stayed hittable; K knocked targets down without a shot during a real round.
- **Decision:** In `Application::handle`, F3, 1–8 and K act only when `NDEBUG` is not defined (`if constexpr (kDebugBuild)`). F4 (post-process on/off), M, N, −/= stay in every build. The command-line diagnostics `--inspect`, `--inspect-targets` and `--knock N` are unchanged, because a switch cannot be pressed by accident.
- **Verification:** Debug, Release and Preview build with `/W4 /WX`, zero warnings; 163/163 unit tests; preview workflow re-packaged (BUILD.md, Desktop distribution). Not covered by an automated test: automated runs ignore real keys by design (D-029 item 10).

---
## D-035 — Decoys, magazine and reload, golden power-up; unlabelled stand-ins (owner request)

- **Status:** Accepted, 2026-11-12, at the owner's request. **Amends the brief:** G-21 (misses were the only bad outcome; decoys now cost points), §9.3 tier 2 (the "limited ammo with reload" variant becomes the default rules, and a power-up target is added), and G-22/G-23 (decoys count as shots, not hits). It also amends D-026: stand-ins no longer carry "MISSING …" lettering.
- **Decisions:**
  1. **Target kinds** (`TargetKind` in `DuckConfig.hpp`): Duck +10; **Decoy** −20 (the score may go below zero; counted as a shot, not a hit, so it lowers accuracy); **Golden** +10 plus free ammo. Each target draws a kind at round start (never Golden then) and again each time it rises (`DuckLane::drawKind`). The draw uses a seeded xorshift inside the lane, which is not re-seeded per round, so rounds differ but a session can be reproduced.
     - `standardMix()`: 25 % decoy, 8 % golden. At most 2 decoys and 1 golden on the lane at once, so at least three of five are always ducks.
     - The default `TargetMix` is all ducks, so tests and plain configurations keep the classic rules.
  2. **Magazine** (`ShotTuning::standard()`): 6 rounds. **R or right click** reloads in 1.2 s of round time. Reload time is frozen by a pause and stopped at time up; no shots are possible while it runs. A trigger pull on an empty magazine is a **dry fire**: a click, not a shot, still rate-limited by the 250 ms action. The default `ShotTuning{}` has no magazine (classic rules, used by the older tests).
  3. **Golden power-up:** 8 s of free ammo. Shots use no rounds, reload is refused, a running reload is cancelled, and the magazine is full when it ends.
  4. **Events:** `DryFire`, `ReloadStarted`, `Reloaded`, `PowerUp`, `PowerUpEnded`; `DuckHit` carries the kind and signed points. `ShotOutcome::Decoy`; `RejectReason::Empty`, `Reloading`. `input::SecondaryPressed` for the right button. R in a round reloads, and stays restart on the pause board.
  5. **Sound:** no reload or dry-fire recordings exist, so the new cues reuse cleared CC0 recordings: dry fire and "reloaded" use the wooden click, reload uses the hinge rattle, power-up uses the register bell. Nothing is synthesised.
  6. **Look:**
     - Decoys are drawn with a red tint and golden ducks with a gold tint, over whatever duck image is drawn. Dedicated decoy and golden photographs would be new asset entries; none exist.
     - The ammo board (`layout::kAmmoSlot`, on the counter beside the timer) shows brass rounds, spent outlines, refilling while reloading, "FREE AMMO n", and a steady red "RELOAD" prompt when empty.
     - Hit tags are signed: "−20" in red, "+10  FREE AMMO" in gold.
     - The results board adds a "Decoys hit" row.
     - The title board explains the rules.
  7. **Unlabelled stand-ins:** the grey cards, boards, panel, reticle and stand-in duck no longer show "MISSING <ID>", paths or "TEMP". The Debug F3 overlay and `carnival.log` still list every missing asset. This also removes the lettering from the foreground hit mask (D-032), which is more accurate.
- **Verification:** 12 new tests in `tests/arcade_tests.cpp` plus an ammo-board layout check (175 total). A scripted 60 s session logged decoy hits, power-ups and reloads, and the results board shows the six rows above the placards.

---
## D-036 — Harder ducks, rarer and shorter power-up; time-ordered lane stepping (owner request)

- **Status:** Accepted, 2026-11-12, at the owner's request. Tunes D-035.
- **Decisions:**
  1. **Faster ducks.** While a target is an ordinary Duck, its motion clock runs `TargetMix::duckSpeed` = **1.35×** faster. Decoys and golden targets keep the base speed. The rate changes only when a target rises with a new kind, so position stays continuous; the whole motion profile (cruise, turns, dwell) scales together.
  2. **Ducks escape.** A Duck or Golden target that stays upright unhit for `upTime` × (0.6…1.4) drops by itself, unscored and unpenalised, through the same Falling → Down → Rising path (so it re-enters at an end stop after ≥ 2 s). `upTime` = **3.5 s**, so 2.1–4.9 s, drawn per rise from the lane's generator. Decoys never escape: they are obstacles. Brief G-19 ("lane never empty") is weaker still than noted in the audit (M-2); playtest.
  3. **Power-up:** golden chance **5 %** per rise (was 8 %); free ammo **5 s** (was 8 s).
  4. **Time-ordered lane stepping.** `DuckLane::update` now advances every target together from one transition to the next in time order (simultaneous ones in id order). Before, each target was advanced through the whole frame in turn, so draws from the shared kind generator, and the per-lane decoy/golden limits, could come out in a different order at a different frame rate. Transition times stay exact.
  5. The defaults (`TargetMix{}`: speed 1, no escape) keep the classic behaviour for the older tests and for `--targets`-only configurations.
- **Verification:** 6 new tests (escape window and count, decoys and classic never escape, no score for escapes, speed scaling for ducks only, standard values, a 90 s standard-mix lane identical at 30/60/144 Hz and with knock-downs at 60/120 Hz); 181 total. A scripted round showed 32 Upright → Falling transitions, 14 of them hits and 18 escapes.

---
## D-037 — Decoys leave too; a guaranteed ammo duck (owner request)

- **Status:** Accepted, 2026-11-12, at the owner's request. Tunes D-035/D-036.
- **Decisions:**
  1. **Decoys leave.** `TargetMix::decoyUpTime` = **5 s** (× 0.6…1.4, so 3–7 s). After it, an unhit decoy drops through the same Falling → Down → Rising path and may come back as anything. Ducks keep `upTime` 3.5 s.
  2. **Guaranteed ammo duck.** `TargetMix::goldenEvery` = **15 s**. If no golden target has risen for 15 s of lane time (the round start counts as one), the next target to rise is golden, still subject to `maxGolden`. The 5 % chance per rise stays on top. The check uses the exact transition time, so it is frame-rate independent (D-036 stepping).
  3. The log names the kind a target rises as ("Down -> Rising as golden").
- **Verification:** decoy-escape test (window 1.2–2.8 s for decoyUpTime 2, none without it); guarantee test (zero chance, goldenEvery 10 s: golden rises at ≥ 10 s and then ≥ 10 s apart; none without the guarantee); 182 tests. A scripted round logged golden rises at 15.4, 31.3, 40.8 (by chance) and 57.5 s.

---
## D-038 — Pacing of how long targets stay up (owner request)

- **Status:** Accepted, 2026-11-12, at the owner's request ("don't last too long, not all the same duration, still challenging and fun"). Tunes D-036/D-037.
- **Decisions** (all in `TargetMix`, applied in `DuckLane::drawKind`; the defaults are off, so the classic behaviour and older tests are unchanged):
  1. **Shorter windows, by kind:**
     - Ducks: `upTime` 3.0 s → 1.8–4.2 s.
     - Golden ammo duck: `goldenUpTime` 2.2 s → 1.3–3.1 s; a reward should be fleeting.
     - Decoys: `decoyUpTime` 4.0 s → 2.4–5.6 s; obstacles linger.
  2. **Gets harder:** `upTimeRamp` 0.25. Duck and golden windows shrink by 25 % per minute of lane time, at most 40 %; by the end of a round, ducks get about 1.35–3.15 s. Decoys do not ramp.
  3. **Fair floor:** `minUpTime` 1.3 s. With the 1.35× duck speed, that is still more than a full pass across a section.
  4. **Staggered:** `dropGap` 0.4 s. Each target's scheduled leave time (lane time when it stands up + its window) is placed at least 0.4 s from every other upright or rising target's.
     - The random draw is the first candidate. Further candidates walk the window in golden-ratio steps (16 tries; the first that fits wins, otherwise the original draw).
     - No extra generator draws, so the sequence and the frame-rate independence (D-036) are unchanged.
- **Measured** in one 60 s unshot round (Debug, scripted): ducks n=35, 1.65–3.67 s (mean 2.63); golden n=4, 1.39–2.36 s; decoys n=6, 3.50–5.38 s; first wave 2.0–5.0 s; 50 departures, none within 0.4 s of another.
- **Verification:** 3 new tests (stagger over 120 s, window/floor/ramp/variety over 60 s, golden fleeting vs decoys lingering over 240 s) and updated standard-mix values; 185 tests.

---
## D-039 — First-person rifle: points at the shot, perspective, stiff swivel (owner request)

- **Status:** Accepted, 2026-11-12, at the owner's request ("stiffer, like the real deal; point in the direction of the shot; more first-person, looking down it"). Supersedes the small swivel of D-026/D-028 and refines brief G-5. The rifle is still decorative: the shot lands at the reticle (A-7).
- **Decisions:**
  1. **Points at the aim.** `RiflePose` is now {heading, recoil}. The barrel heading runs from the swivel (`kRiflePivot` 1170,1085, at the bottom edge just right of centre) straight at the aim, clamped to −150°…−30° so it never points down. The swivel, the muzzle and the aim are collinear (tested).
  2. **First-person perspective.** `rifleAxisToLogical(along, across)` is a projective map:
     - the barrel recedes toward the aim (`kRifleReach` 340, `kRifleDepth` 0.9: the muzzle is drawn at about half the width of the receiver);
     - the stock widens toward the eye and runs off the bottom of the screen.
     - A photo (`FG_RIFLE`) is drawn as a 48×12 textured mesh through `rifleImageToLogical`.
  3. **Stand-in redrawn as seen from behind:** same flat parts and greys (barrel with muzzle band and bead, forend, receiver with rear-sight notch, stock), symmetric about the barrel line. The side-profile silhouette, rotated and put in perspective, made the stock jut sideways.
  4. **Stiff swivel.** `RifleMount` is a damped spring (natural frequency 11 rad/s, damping 0.9, fixed 2 ms substeps). The barrel swings after the reticle and settles in about 0.3 s without visible overshoot, identically at any frame rate. Visual only.
  5. **Recoil** pulls the gun 26 units back toward the eye and climbs the muzzle 2.5°. The muzzle puff spawns at the actual muzzle.
  6. **Bounds.** Asset rule: the silhouette lies within ±70 image units of the swivel→muzzle line (ASSET_GUIDE, manifest). `rifleBounds` bounds that corridor clipped to the screen, so the off-screen stock does not count. The layout checks are unchanged: never over the lane, the timer board or the ammo board, at any aim or recoil.
- **Verification:**
  - 3 new or rewritten layout tests: barrel collinear with the aim, plus clamps; perspective narrowing and the off-screen stock; spring lag, settle, no overshoot, and frame-rate independence. 186 tests in total.
  - Screenshots at left, centre and right aims and on a shot.
  - Not verifiable in scripted runs: the swing itself, because scripted runs fix the aim.

---
## D-040 — The wave cutout hides the duck bases and the rail (owner request)

- **Status:** Accepted, 2026-11-12, at the owner's request. The duck bases and the track line showed through the wave troughs, which broke the illusion.
- **Before:** crests at y 585, troughs 26 deep (to 611), duck hinge line at 600, rail 574–596. Every trough showed the bottom of the duck (its hinge bracket and flat base) and the rail.
- **Decisions:**
  - Wave cutout raised: `kWaveStrip` y 585 → **570**, height 140 → 155, so its bottom edge is unchanged.
  - Troughs made shallower: `kWaveTroughDepth` 26 → **15**, so the deepest trough is at 585.
  - Rail moved wholly behind the solid board: `kTrackRail` 587–605.
  - The duck track (`kTrackY` 600) is unchanged.
  - Result: even in a trough, the bottom 15 units of every duck are covered, which is more than its hinge bracket (the lowest 12 % of its height). About 71 % of the duck shows above the crests and 86 % in a trough.
  - The TiltDrop depth, the foreground hit mask (D-032) and the miss-mark region follow automatically from these constants.
- **Layout rule:** the rail lies behind the solid board, the deepest trough sits at least 2 units above the duck's base band, and at least 70 % of the duck is above the crests (was 80 %). The first two are compile-time `static_assert`s; the third is a test.
- **For the photographs:** `FG_WAVE_STRIP` should have its crests at the top of its frame and troughs no deeper than about 15 units (≈ 10 % of the strip's height).

---
## D-041 — Project folder renamed to `carnival-shooting-gallery` (owner request)

- **Status:** Accepted, 2026-11-12. The folder was `p` since the brief.
- **Name:** lower-case kebab-case, matching the repository name.
- **What it took:**
  - An editor background process held the downloaded SFML sources in `build/*/_deps/sfml-src` open, which blocked the rename, so it was stopped first.
  - The generated `build/` was deleted, because CMake caches absolute paths, and then rebuilt from scratch: configure, Debug, Release and the preview workflow.
- **References:** the docs no longer depend on the folder name ("run from the project folder"). The dated notes in ASSET_GUIDE, AUDIT_REPORT and PROJECT_BRIEF now give the new path, with the old one alongside.
- **README:** "Installing and running" rewritten:
  - Option A, play from the zip: the extracted folder layout, the SmartScreen "Run anyway" step, a shortcut, how to uninstall.
  - Option B, run straight from a build.
  - Also: how a round goes, how to quit, a troubleshooting table, and the Build Tools install step with the exact `cd` line.
- **Verification:** 186/186 tests in the new location (Debug and preview). The re-packaged zip was extracted outside the project and run from `C:\Windows`: asset root found next to the exe, shots logged, exit 0.

---
## D-042 — No stand-in for the header sign (owner request)

- **Status:** Accepted, 2026-11-12. Amends D-026 for this one slot.
- **Context:** Without its photograph, `DEC_SIGN_HEADER` was drawn as a plain grey card left of the score board. Since D-035 removed the stand-in lettering, it was an unexplained empty box with no gameplay role.
- **Decision:** the header sign is drawn only when its photograph exists, like the other decorative E1 assets (bunting, bulb glow). The header beam and awning show in its place. The slot (`layout::kSignSlot`), its layout checks and the F3 outline are unchanged, so a supplied photo appears with no code change. The now-unused `StandInPainter::card` and `standin::kSign` were removed.
- **Hit mask:** the sign was part of the foreground mask (D-032) but lies far above the lane, so no target or shot is affected.
- **Verification:** 186/186 tests; screenshot shows the header without the box; preview re-packaged.

---
## D-043 — Playable copy in `game/`; machine details removed from the docs (owner request)

- **Status:** Accepted, 2026-11-13.
- **Context:** It was unclear which `CarnivalShootingGallery.exe` opens the game: the playable one sat four folders deep in `build/windows-msvc-preview/Release/`, next to development and strict-release exes with the same name. The docs also described the development computer (hardware, display, OS build, security software, tooling environment), which the owner did not want published.
- **Decision:**
  1. Every preview build installs the `Game` component (the same layout as the zip) to `<project>/game/` as a post-build step. `game/CarnivalShootingGallery.exe` is the one file to open. `game/` is generated and gitignored, like `build/` and `dist/`.
  2. The README opens with a "Where is the game?" box: Releases zip for players, `game\CarnivalShootingGallery.exe` for people who build it. The other exes are listed as developer outputs.
  3. Records now say "one Windows 11 x64 PC with a high-DPI display" instead of naming the hardware, and describe the blocked-download risk (R-4) as generic TLS interception.
- **Verification:** `cmake --workflow --preset preview`: build, 2/2 ctest, zip; `game\CarnivalShootingGallery.exe --frames 60` found its assets in `game\assets`, exit 0.
