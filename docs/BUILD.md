# Build and Run — Carnival Shooting Gallery

Status: **M1 booth composition** (D-026) **+ duck target system** (D-027) **+ shooting** (D-028) **+ rounds and screens** (D-029) **+ audio and atmosphere** (D-030, D-031) **+ preview distribution** (D-033): main menu, a 60-second round with a "Ready… Go!" countdown, pause, results with session best, and restart. Five moving targets, mouse aiming, hitscan shots with alpha-mask hits, live score, CC0 sound effects and ambience. All photographic assets are still MISSING and render as plain grey stand-ins (unlabelled since D-035).

## Requirements

| Tool | Version used | Notes |
|---|---|---|
| Windows | 10 1607+ / 11 | `GetDpiForWindow` and the per-monitor-v2 DPI manifest need 1607+ |
| Visual Studio 2022 Build Tools | MSVC 14.44 | The "Desktop development with C++" workload |
| CMake | ≥ 3.26 (VS-bundled 3.31.6 works) | Not on `PATH` by default; see below |
| Network | First configure only | Downloads SFML and its sub-dependencies |

The VS-bundled CMake lives here. The commands below use these variables:

```powershell
# PowerShell
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$ctest = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe"
```

No Developer Command Prompt is needed: the preset uses the Visual Studio generator, which finds MSVC itself.

## Commands

Run from the project folder (the one containing `CMakeLists.txt`; any name works, nothing depends on it).

```powershell
# 1. Configure (first run ~1 min: fetches and configures SFML 3.0.2 + deps)
& $cmake --preset windows-msvc

# 2. Build
& $cmake --build --preset debug       # console subsystem: logs also on stderr
& $cmake --build --preset release     # WIN32 subsystem: logs go to carnival.log only

# 3. Test (logic library, headless)
& $ctest --preset debug
& $ctest --preset release

# 4. Run
.\build\windows-msvc\Debug\CarnivalShootingGallery.exe
.\build\windows-msvc\Release\CarnivalShootingGallery.exe
```

The first build compiles SFML, FreeType, ogg, vorbis, and FLAC (a few minutes). Later builds are incremental.

### Outputs

```
build/windows-msvc/<Debug|Release>/
├── CarnivalShootingGallery.exe   the game (SFML static; this tree uses the VC++ runtime DLLs)
├── gallery_tests.exe      unit tests
├── assets/                mirrored from the project's assets/ after every build
└── carnival.log           written next to the exe on each run (overwritten)
```

## Asset paths

- The game finds assets at `<directory of CarnivalShootingGallery.exe>/assets/`. It never uses the current working directory, so it can be launched from Explorer, an IDE, or a terminal in any folder.
- A post-build step mirrors `assets/` there (`copy_directory_if_different`). Files *deleted* from `assets/` are not removed from the mirror; delete `build/windows-msvc/<config>/assets` to get a clean copy.
- If the folder is missing, the game shows an error naming the expected path and exits with code 1.

## Controls

| Input | Effect |
|---|---|
| Mouse | On the boards: the normal pointer, over painted placards (the lit lamp shows which one). In a round: aims; the game draws the reticle, and the OS pointer is hidden and confined to the window |
| Left click | Boards: press the placard under the pointer (never a shot). Paused: a click anywhere off the placards resumes. In a round: shoot at the reticle (D-028), only after "Go!" and before "Time!". 250 ms rifle cooldown: clicks during it are ignored and not counted. A hit knocks the target down and scores 10 (a red decoy costs 20; a gold duck scores 10 and gives 5 s of free ammo, D-036; ducks not hit within about 2-5 s duck down unscored); a miss leaves a mark on the backboard. Each shot uses one of 6 rounds (D-035); on an empty rifle a click only dry-fires and is not counted |
| Enter / Space | Title: start a round. Paused: resume. Results: play again |
| R | Round: reload (D-035). Paused: restart the round from the top. Results: play again |
| Right click | Round: reload (D-035). Ignored on the boards. Never a shot |
| P | Round: pause. Paused: resume |
| Esc | Title: quit. Round: pause. Paused: back to the main menu (the round is discarded). Results: quit |
| Focus loss | Pauses a running round. Coming back never auto-resumes |
| 1–8 | **Debug build only** (ignored in Release and Preview, D-034). Toggle a layer group: 1 background and booth, 2 decorations and signage, 3 track, 4 ducks, 5 counter, 6 rifle, 7 crosshair and HUD, 8 shot effects |
| F3 | **Debug build only** (D-034; use `--inspect` / `--inspect-targets` in other builds). Cycles the inspection overlay: off → layout view (slot outlines, lane band, horizon, rifle pivot and muzzle, viewport numbers, missing-asset list, plus target outlines) → target view (movement-bound brackets, posed hit masks: green = hittable, red = not, white = under the aim; per-target state table) → off |
| F4 | Post-process on/off (grade, vignette, and grain when the scanned plate exists) |
| K | **Debug build only** (D-034; `--knock N` works in every build). Knock down the next upright target (during a round) to preview its hit reaction and re-entry. Not a shot: no score, no stats |
| M | Sound on/off (any screen; D-030) |
| − / = (also numpad − / +) | Master volume down / up in 10 % steps (default 80 %) |
| N | Background music on/off (only audible once the optional `MUS_CALLIOPE` recording is installed) |
| Window close | Quit from any screen |

## Sprites: the grey-shape art (D-044)

Every image under `assets/images/` is generated from code. `src/shell/render/StandIn.cpp` draws each shape; `tools/sprite_bake.cpp` saves it as a PNG at the size the game uses. To change a sprite, edit its painter function, then rebuild and re-run the tool:

```powershell
& $cmake --build --preset debug --target sprite_bake
.\build\windows-msvc\Debug\sprite_bake.exe assets                 # all 19 sprites
.\build\windows-msvc\Debug\sprite_bake.exe assets FG_RIFLE        # just one (keys as in assets/ASSET_MANIFEST.md)
.\build\windows-msvc\Debug\sprite_bake.exe assets --scale 2       # 2 px per logical unit: sharper on high-DPI screens, same on-screen size
```

The output is deterministic: re-running unchanged code writes byte-identical files. After a change, update that sprite's SHA-256 and size in `assets/ASSET_MANIFEST.md` and `assets/licenses/<KEY>.md`. The sign's lettering is rendered from the Windows font Rockwell Bold, so baking needs Windows.

## Missing sprites: Debug vs Release (D-026, D-044)

- **Debug:** if a sprite file is missing, the game draws the same shape in code (F3 lists what is missing).
- **Release:** refuses to start (exit code 1, listing every missing asset) unless you pass `--stand-ins`.
- **`ctest --preset release`:** the `asset_contract` test **fails** until every MVP asset is `READY` in `assets/ASSET_MANIFEST.md`. Since D-044 every image passes; only the ten MVP sounds, still `IN_PROGRESS` until their listening review, keep it failing. This is the D-023 release gate, working as intended.

The Release exe uses the WIN32 subsystem. To get its exit code in PowerShell, start it with `Start-Process -Wait -PassThru`.

## Diagnostic switches

None are needed to play. They exist for automated smoke tests and inspection.

| Switch | Meaning |
|---|---|
| `--frames N` | Render N frames, then shut down normally |
| `--screenshot PATH` | With `--frames`: save the final frame as a PNG |
| `--assets DIR` | Use DIR as the asset root instead of `<exe>/assets` |
| `--no-dialog` | Report fatal errors to the log/stderr only, with no message box |
| `--stand-ins` | Allow missing MVP images (plain grey shapes). Always on in Debug |
| `--inspect` | Start with the F3 overlay on |
| `--window WxH` | Initial client size in physical pixels (aspect-ratio checks) |
| `--aim X,Y` | Pin the aim point in logical units; the mouse is ignored (deterministic screenshots) |
| `--fire N`, `--click N` | Left click at the current aim on loop iteration N. A shot only during a live round; on a board it presses the placard under the aim. Repeatable |
| `--key N:NAME` | Press a game key on iteration N: `esc`, `space`, `enter`, `r`, `p`. Repeatable |
| `--focus-lost N`, `--focus-gained N` | Simulate the window losing or regaining focus on iteration N |
| `--close N` | Window close request on iteration N |
| `--inspect-targets` | Start in the F3 target view |
| `--targets NAME` | Target preset (D-027): `sections` (default, five independent shuttle carriages) or `chain` (the brief's single loop) |
| `--reaction NAME` | Hit reaction for every target: `flip` folds back on the hinge (default), `drop` tips and sinks behind the waves |
| `--knock N` | Debug knock-down of the next upright target at frame N |
| `--sim-dt S` | Feed exactly S seconds per frame (0 < S ≤ 0.1) instead of the wall clock, for deterministic frame-rate comparisons |
| `--fps N` | Turn vsync off and cap the frame rate at N |
| `--volume N` | Master volume 0–100 (default 80) |
| `--mute` | Start muted (M toggles). Cues still run through the voice pool, so `--log-audio` still verifies them |
| `--no-music` | Start with the optional music off (N toggles) |
| `--no-audio` | Never open the audio device: fully silent (headless runs, screenshots) |
| `--log-audio` | Log every sound cue as it plays (cue, voice, pitch, volume, audio time) |
| `--no-flicker` | Steady lighting: no bulb dips, generator sags or bunting sway (D-031) |

Smoke tests. The game boots to the main menu, and the round only accepts shots after "Go!" (1.5 s), so scripts start a round with `--key 2:enter` first:

```powershell
# Each board (aim over a placard lights its lamp)
.\build\windows-msvc\Debug\CarnivalShootingGallery.exe --frames 5 --window 1600x900 --aim 960,742 --screenshot title.png --no-dialog
.\build\windows-msvc\Debug\CarnivalShootingGallery.exe --frames 320 --sim-dt 0.0166667 --key 2:enter --fire 150 --key 300:esc --aim 960,673 --screenshot paused.png --no-dialog
.\build\windows-msvc\Debug\CarnivalShootingGallery.exe --frames 660 --sim-dt 0.1 --fps 1000 --key 2:enter --aim 760,550 --fire 40 --fire 45 --screenshot results.png --no-dialog
# Booth layout and aspect ratio
.\build\windows-msvc\Debug\CarnivalShootingGallery.exe --frames 30 --window 2520x1080 --inspect --screenshot m1_21x9.png --no-dialog
```

Any `--frames` or scripted run ignores real mouse, keyboard and focus events and never confines the pointer (D-029), so it cannot be disturbed by, or disturb, the person at the desk.

## Exit codes

| Code | Meaning |
|---|---|
| 0 | Normal shutdown |
| 1 | Startup failure (missing assets folder, no window/GL context, bad command line) or unexpected error. Details are in `carnival.log` and the message box |

## Dependency pinning

| Dependency | Pin |
|---|---|
| SFML | 3.0.2, commit `0fa201c969e48ecc253581c5841ce73f44d42f49`, fetched as the GitHub archive of that commit and verified by SHA-256 `df03afdc2858d0e15e1c9ff06bddd113c18b2a6c089532f5b8d05dbb2e653f2a` |
| FreeType, ogg, vorbis, FLAC | Release tags fixed by SFML 3.0.2's own CMake: `VER-2-13-2`, `v1.3.5`, `v1.3.7`, `1.4.3` |
| miniaudio | Vendored inside the SFML source |

### If the download is blocked (risk R-4)

If the download fails (for example behind a TLS-intercepting proxy or antivirus):

1. Download `https://github.com/SFML/SFML/archive/0fa201c969e48ecc253581c5841ce73f44d42f49.tar.gz` by hand and check its SHA-256 matches the value above.
2. Extract it anywhere outside the project.
3. Configure with `& $cmake --preset windows-msvc -DFETCHCONTENT_SOURCE_DIR_SFML=<extracted folder>`.

SFML's sub-dependencies are git clones. If those are blocked as well, set the matching `FETCHCONTENT_SOURCE_DIR_<NAME>` variables (`FREETYPE`, `OGG`, `VORBIS`, `FLAC`) the same way.

## Distribution: preview build (D-033)

A separate build tree, `build/windows-msvc-preview/`, produces the desktop distribution. It is Release-optimized, starts with the grey stand-ins (no `--stand-ins` needed), says "(Preview)" in its title, and links the MSVC runtime statically, so the exe imports only Windows system DLLs. The development tree above is not touched.

```powershell
# One command: configure -> build -> test -> package
& $cmake --workflow --preset preview
# Output: game\CarnivalShootingGallery.exe (ready to play; D-043)
#         dist\CarnivalShootingGallery-0.1.0-preview-win64.zip (the same folder, zipped)

# Or step by step (the build step alone also refreshes game\):
& $cmake --preset windows-msvc-preview
& $cmake --build --preset preview
& $ctest --preset preview
& $cmake --install build\windows-msvc-preview --config Release --component Game --prefix dist\CarnivalShootingGallery-0.1.0-preview-win64
```

Distribution layout:

```
CarnivalShootingGallery-0.1.0-preview-win64/
├── CarnivalShootingGallery.exe
├── README.md
├── THIRD_PARTY_NOTICES.md
├── assets/                 runtime assets, ASSET_MANIFEST.md, licenses/ (per-asset evidence, CC0 text)
├── licenses/third-party/   SFML, FreeType, ogg, vorbis, FLAC, miniaudio, stb_image, minimp3, glad
└── carnival.log            created on first run (or %LOCALAPPDATA%\CarnivalShootingGallery\ if the folder is read-only)
```

The strict Release build (`--preset release`) still refuses to start until every MVP asset is READY (D-023).

`ctest --preset preview` runs `asset_contract` without the release gate; the strict gate still runs in `ctest --preset release`.

> Records below were made before D-033 renamed the exe from `carnival_gallery.exe` to `CarnivalShootingGallery.exe`; to rerun a recorded command, use the new name.

## Verified on 2026-11-11

| Check | Result |
|---|---|
| CMake configure | OK (SFML + sub-deps fetched, hash verified) |
| Debug and Release builds | OK, zero warnings under `/W4 /WX /permissive-` |
| Unit tests | 21/21 pass, Debug and Release |
| Launch from `C:\` (not the build folder) | OK, assets resolved next to the exe |
| Window contents | Title card renders in the letterboxed 1920×1080 canvas (screenshots at 1280×720 and 3200×1800) |
| High DPI (250 % scaling) | Window opens scaled to the display DPI, crisp text |
| Close via Esc / `--frames` / title-bar close (`WM_CLOSE`) | Orderly teardown logged, exit code 0, no process left |
| Debug CRT leak report at exit | Empty (no leaked heap blocks) |
| Missing asset folder | Clear error naming the path, exit code 1 |

## M1 verified on 2026-11-11

| Check | Result |
|---|---|
| Debug and Release builds | OK, zero warnings under `/W4 /WX /permissive-` |
| Unit tests | 48/48 pass (Placement, SceneLayout rules, Viewport, Time) |
| Asset contract | Debug: pass with 20 MVP-missing warnings. Release: fails on the release gate, as intended |
| Composition | Renders at 1600×900, 1200×900 (4:3, bars top and bottom), 2520×1080 (21:9, bars left and right), 640×360. No distortion |
| Effects | Test shot draws the muzzle puff and recoil, and logs "miss mark on backboard" |
| Release without assets | Exit code 1, listing all 20 missing MVP assets |
| Release with `--stand-ins` | 120 frames, exit code 0 |

## Duck target system verified on 2026-11-11 (D-027)

| Check | Result |
|---|---|
| Debug and Release builds | OK, zero warnings under `/W4 /WX /permissive-` |
| Unit tests | 87/87 pass, Debug and Release: alpha mask, affine transform, motion profile (bounds, timing, no instant reversal, edges), lane (config validation, bounds over about 5 minutes with knock-downs, lane band, chain spacing, state transitions at exact times, re-entry at an end stop, AC-6/7/8, reset, single-transform R-3 check), layout |
| Frame-rate independence (logic) | A scripted 40 s session with 8 knock-downs is identical at 30/60/144/240 Hz, with jittery 2–100 ms frames, and with one step per event, for both presets and both reactions |
| Frame-rate independence (app) | `--sim-dt` at 30/60/144/240 Hz for 4.0 s, with a knock-down at t = 1.0 s: identical exit snapshots to 4 decimals, and identical transition times |
| Real clock | `--fps 30` (1280×720), `--fps 144` (2520×1080), `--fps 240` (640×360), vsync (1200×900): about 3.08 s of lane time in about 3 s, clean exit |
| Window sizes | Targets stay in their sections and letterbox correctly at 16:9, 21:9 and 640×360 |
| Hit regions | Hit masks are built from the drawn image (stand-in silhouette: 23,310 solid texels of 256×210). The F3 target view shows the mask posed exactly over the sprite |
| Asset contract | Debug: pass. Release: fails on the D-023 gate, as intended (all MVP art is still MISSING) |

Commands used:

```powershell
.\build\windows-msvc\Debug\carnival_gallery.exe --frames 66 --sim-dt 0.0166666667 --knock 60 --aim 880,560 --inspect-targets --screenshot targets.png --no-dialog
.\build\windows-msvc\Debug\carnival_gallery.exe --frames 240 --sim-dt 0.016666666666666666 --knock 61 --no-dialog   # compare the exit snapshot in carnival.log across rates
```

## Shooting verified on 2026-11-11 (D-028)

| Check | Result |
|---|---|
| Debug and Release builds | OK, zero warnings |
| Unit tests | 105/105, Debug and Release. The 18 new shooting tests cover: rifle 249/250 ms boundary and cooldown summed from frames; accuracy rounding (0 shots → "—", 1/3, 2/3, 1/8, 0.5 %, 99.5 %); hit, miss, transparent-corner miss; 10 clicks in one frame = 1 shot; cooldown clicks not counted; no double scoring; falling/down/rising never hit and upright again scores; paused targets; non-finite aim; reset; mouse pixel → logical → hit at 8 window sizes and aspects; letterbox-bar clicks clamp to the canvas edge |
| Scripted smoke (`--aim 882,548 --fire 66 --fire 67 --fire 90`) | Hit target #2 (+10), second click ignored (233 ms left), third shot at the now-down target = miss; score 10, accuracy 50 %; Debug and Release |
| Real OS mouse clicks | Clicks at known client pixels of a 2520×1080 (side bars) and a 1200×900 (top/bottom bars) window logged exactly the expected logical aim, including the clamp for a click in a bar |
| Visual | Score board, tally line, "+10" tag, warm reticle on hit, muzzle puff and recoil, miss mark; the background stays still |

## Rounds and screens verified on 2026-11-11 (D-029)

| Check | Result |
|---|---|
| Debug and Release builds | OK, zero warnings under `/W4 /WX /permissive-` |
| Unit tests | 134/134, Debug and Release. New: `round_clock_tests` (phases at 1.5 / 61.5 / 62.5 s, boundary split, display rounding, low-time window, 60 s from 30/60/144/240 Hz deltas, countdown clicks not shots, click in the zero-crossing frame counts, targets stop exactly at zero, ticks 10..1 then TimeUp then RoundFinished with the real stats) and `game_flow_tests` (every transition by placard and key, keys that must do nothing, board and resume clicks never shots, pause by Esc / P / focus loss freezes clock and targets, no auto-resume, results stats, session best, ten back-to-back clean restarts, restart from pause, input after close) |
| `ctest --preset debug` | 2/2 pass |
| `ctest --preset release` | `asset_contract` fails: the D-023 release gate, as intended, until the MVP assets are READY |
| Scripted 12-round session (Debug, 8332 frames) | Every transition exercised (Title→Playing, Playing⇄Paused ×42, Paused→Title ×3, Paused→Playing fresh ×3, Playing→Results ×12, Results→Playing ×11). Every round hit TIME UP at t = 61.500 s; all 12 rounds produced identical stats for identical input; every resume kept the exact remaining time; exit code 0 |
| Memory and leaks | Private bytes flat at 71–77 MB across the 12 rounds; Debug CRT leak report empty |
| Release scripted run | Title → round → pause → R restart → results; exit code 0 |
| Visual | Screenshots of title, countdown ("Ready…"), live round, low time (red numerals), paused, results, and results with "New session best!" |

## Audio and atmosphere verified on 2026-11-12 (D-030, D-031)

Preparing sounds (only needed when an original or a cut changes):

```powershell
& $cmake --build --preset release --target audio_prep
sh asset-src/work/audio/prep.sh                     # re-cuts every runtime sound from asset-src/originals/audio
perl asset-src/work/audio/evidence.pl               # regenerates assets/licenses/<ID>.md
# Listening review / device check, no game code involved:
.\build\windows-msvc\Release\audio_prep.exe play assets\audio\effects\shot.wav
.\build\windows-msvc\Release\audio_prep.exe play asset-src\work\audio\audition\band_organ_song_a_222-265s.ogg
```

Results:

- `gallery_tests` 152/152 (13 soundscape + 5 atmosphere tests new). `asset_contract` Debug 7/7, including the new evidence-file and audio-format checks; Release fails only the D-023 gate, as intended.
- A scripted, muted session (`--mute --log-audio --sim-dt 0.0166667`, 80 s: start, 60 clicks, pause/resume, a full round, restart, menu, close) logged 208 cues that match the session exactly: 73 shots = 14 hits + 59 misses, 14 score clacks, 14 slams, 13 resets, 10 ticks, 2 end bells, 1 results bell, 2 start bells (two rounds), 6 board clicks. Every shot cue is in the same frame as its shot.
- Shutdown logs "Audio: stopped and released" before the renderer, assets and window go; exit code 0.
- **Not verified here: audible output.** In the original test environment Windows refused the output device (`[WASAPI] Failed to initialize device ... Invalid argument`), even for `audio_prep play` with no game code. The game logs one warning and continues silently. To check, run the `audio_prep play` line above; it prints `device: <name>` and plays the file, or says the device could not be opened.
- Screenshot two frames after a hit: grade, puff, recoil, "+10" tag and flash lift render correctly; the reticle and HUD stay outside the post-process.

## Engineering verification 2026-11-12 (D-032)

| Area | Result |
|---|---|
| Builds | Debug + Release, zero warnings under `/W4 /WX /permissive-` (before and after fixes) |
| Unit tests | 163/163 Debug and Release (11 new: occlusion, frame stats, whole session identical at 30/60/144/240 Hz with pause, rifle cooldown frozen by pause, Ending phase, fire-rate cap) |
| `ctest --preset release` | `asset_contract` fails the D-023 release gate (20 images MISSING, 10 sounds IN_PROGRESS), as intended |
| Startup / shutdown | Exit 0, orderly teardown (audio → renderer → assets → window), Debug CRT leak report empty; missing asset folder → exit 1; Release without assets → exit 1 listing them; corrupt image → exit 1; corrupt font → exit 1 (fixed, D2); bad command line with `--no-dialog` → exit 1, no dialog (fixed, D3) |
| Resize + mouse conversion (real OS messages) | Window resized live to 1368×1062 (bars top/bottom) and 2568×912 (bars left/right): new viewport logged; a click in the 14-unit gap between placards did nothing; a click 3 units inside the Quit placard's edge quit |
| Hit detection | Shot on a booth post no longer scores the hidden chain duck; a visible duck in the same frame still scores (fixed, D1) |
| Frame-rate independence (app) | Full scripted round, 30 shots, at `--sim-dt` 30/60/144/240 Hz: identical hit/miss sequence, TIME UP at 61.500 s, score 100, 10/30, 33 % |
| Performance | **Not conclusive.** See the verification report: runs were interrupted (window minimised/closed). Indication only, integrated GPU, high-DPI window, photo-sized fixture, uncapped: mean 11.1 ms, p99 17–31 ms; MSAA off measured faster but runs were disturbed |

## Desktop distribution verified on 2026-11-12 (D-033)

Tested on one Windows 11 x64 PC with a high-DPI display.

| Check | Command / method | Result |
|---|---|---|
| Reproducible build | `cmake --workflow --preset preview` in a fresh `build/windows-msvc-preview/` | exit 0; configure, build, test, package all succeed; zero compiler warnings (`/W4 /WX`) |
| Tests (preview) | same workflow; `gallery_tests.exe` directly | 2/2 ctest; 186/186 unit tests (after D-039) |
| Package | `dist/CarnivalShootingGallery-0.1.0-preview-win64.zip`, 4,496,845 bytes, SHA-256 `960AD6E2399364F896138C1F393D4740AEF403C1A6097FBCD75FEF10059647A1` (rebuilt 2026-11-13 for D-043 by `cmake --workflow --preset preview`: only the bundled README changed; 0 warnings, 2/2 ctest, 186/186 unit tests; published as GitHub release v0.1.0-preview; supersedes 321DFD6B…, 64D366D9…, 30A7CAA8…, ED4932DB…, 75272303…, 22162269…, F82A3B68…, DB620354…, 3161AD99…, 48885447…, 04872C3B…) | exe, README, THIRD_PARTY_NOTICES, `assets/` (13 sounds, shader, manifest, 13 evidence files + CC0 text, no `.gitkeep`), `licenses/third-party/` (10 files) |
| Runtime DLLs | `dumpbin /dependents` | OPENGL32, GDI32, WINMM, KERNEL32, USER32, ADVAPI32 only (no MSVCP140/VCRUNTIME140) |
| Naming | Explorer version info; window title polled during a run | ProductName "Carnival Shooting Gallery", version 0.1.0-preview, PreRelease flag; title "Carnival Shooting Gallery (Preview)" |
| Launch from distribution folder | zip extracted to a temporary folder outside the project, started with working directory `C:\WINDOWS`, scripted: Enter → 2 shots → Esc → Esc → Esc | exit 0; asset root = `<exe>\assets`; 13 sounds loaded, 20 MVP stand-ins; HIT + miss scored; real output device opened, 10 cues played; Title → Playing → Paused → Title → Exiting; "Shutdown complete; exit code 0" |
| Title screen | `--frames 240 --screenshot` from the same folder | menu renders correctly with stand-ins |
| Read-only install folder | copy with write denied via `icacls` | exit 0; no log in the exe folder; log written to `%LOCALAPPDATA%\CarnivalShootingGallery\carnival.log` (test folder removed afterwards) |
| Missing `assets/` | folder renamed, `--no-dialog` | exit 1, "The game's asset folder was not found" |
| Development tree preserved | `windows-msvc` configure, Debug + Release build, `ctest --preset debug` / `release` | builds OK; Debug 2/2; Release `asset_contract` fails on the D-023 gate as before (20 MISSING + 10 IN_PROGRESS); strict Release exe exits 1 listing 20 required asset problems |

Not tested: other PCs or Windows versions, a machine without Rockwell/Georgia/Segoe UI, listening to the audio, a full 60-second round with real mouse input from the package.
