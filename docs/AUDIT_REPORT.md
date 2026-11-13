# Independent Audit — Carnival Shooting Gallery 0.1.0

Date: 2026-11-12 · Scope: the whole project tree (then named `p`, renamed `carnival-shooting-gallery` on 2026-11-12) (code, build, tests, assets, docs) reviewed as an acceptance audit, assuming nothing from earlier passes · Machine: Windows 11 x64, MSVC 14.44, CMake 3.31.6, SFML 3.0.2.

Severity: **Critical** blocks acceptance · **High** must be resolved before a real release · **Medium** should be resolved or explicitly accepted · **Low** minor.
Type: **Defect** (verified, reproducible) · **Deviation** (differs from the approved brief) · **Risk** (unverified) · **Opinion** (subjective recommendation).

## 1. Overall assessment

The **engineering and gameplay are in good shape. The visual product is not there yet.** The rules (aiming, hitscan, alpha-mask hits, occlusion, scoring, the clock, state transitions) are correct, deterministic and well tested. The architecture is clean. But the game was designed around photographs, and **not one exists**. Every image is a labelled grey card, so the visual half of the brief (§7, AV-1 to AV-5) cannot be met or even judged. Audio exists, but nobody has listened to it.

Verdict: **accept as an engineering preview; do not accept as the MVP.**

## 2. Build and test evidence (this audit)

| Check | Command | Result |
|---|---|---|
| Debug build | `cmake --build --preset debug` | OK, 0 warnings (`/W4 /WX /permissive-`) |
| Strict Release build | `cmake --build --preset release` | OK, 0 warnings |
| Unit tests | `ctest --preset debug`; `gallery_tests.exe` | 2/2; **163/163** |
| Release gate | `ctest --preset release` | `asset_contract` **FAILS as designed** (D-023): 20 MVP assets MISSING, 10 sounds IN_PROGRESS |
| Preview workflow | `cmake --workflow --preset preview` | configure, build, test (2/2), package OK |
| Package | `dist/CarnivalShootingGallery-0.1.0-preview-win64.zip` | before the fix, SHA-256 matched BUILD.md (`04872C3B…`). Re-packaged after the fix: 4,485,827 bytes, `48885447…273F` |
| End-to-end round (preview exe) | `--frames 3800 --sim-dt 1/60 --key 5:enter --aim 960,560 --fire …` (9 shots) | Title → Playing → Results; 4 hits / 5 misses; score 40; accuracy 44 % (4/9 rounded half up); a shot 0.1 s before zero counted; TIME UP at exactly 61.500 s; exit 0 |
| Screenshots | title and mid-round, 1280×720 | layout correct; all art is stand-in cards (see §4) |

## 3. Findings

### Critical

| ID | Type | Finding | Evidence | Status |
|---|---|---|---|---|
| C-1 | Deviation | **No photographic assets.** All 19 MVP images and the display font are MISSING. The scene is grey cards with file-name labels. It does not look like a carnival booth, and AV-1 to AV-5 (layering, lighting, readability, grain, perspective) cannot be assessed. Film grain is off (no scanned plate) and there are no bulb glows (no photographed glow), both by design. | Startup log (20 MISSING); screenshots; release gate | **Open.** Owner action: source the photographs (ASSET_GUIDE §7, Q-1 to Q-5). Out of audit scope: no generated substitutes. |

### High

| ID | Type | Finding | Status |
|---|---|---|---|
| H-1 | Defect | **Development keys were live in player builds.** F3 (inspection overlay), 1–8 (layer toggles) and K (knock-down) worked in Release and in the distributed preview. Pressing 4 hid every duck with no on-screen sign, but the ducks stayed hittable. K knocked targets down without a shot. ARCHITECTURE §7 says the overlay is Debug-only. | **Fixed** (D-034): the keys work only in Debug builds; the `--inspect` and `--knock` switches are kept. Rebuilt and re-tested; README and BUILD.md updated. |
| H-2 | Risk | **Performance (AV-7) unverified.** Measured frame times with stand-ins mean nothing for full-size photographs. 4× MSAA on both the window and a full-resolution offscreen target is the likely cost on the integrated reference GPU. | Open. Measure once the photographs exist. |
| H-3 | Deviation | **Default target motion differs from the approved brief.** The `sections` preset (five independent shuttles, D-027) replaces G-12/G-14/AC-5 (one chain moving left→right). D-027 is still "Proposed, needs the owner's approval". AC-5 as written passes only with `--targets chain`. | Open: owner decision. |
| H-4 | Risk | **Audio has never been heard.** All 10 MVP sounds are IN_PROGRESS. Cue logic and teardown are verified, but tone, level and appropriateness (A-13) are not. | Open: owner listening review (`audio_prep play`). |

### Medium

| ID | Type | Finding |
|---|---|---|
| M-1 | Deviation | The live HUD shows hits, misses and accuracy under the score. Brief G-24 says accuracy appears **only on results**, to keep the HUD minimal. D-028 §6 made the change without marking it as an amendment to the brief. Needs the owner's decision. |
| M-2 | Risk | G-19 ("lane never empty") is not guaranteed. A downed target stays down for at least 2 s and then waits for its end stop. With a 250 ms cooldown, a good player can down all five targets in about 1.25 s and leave the lane empty for over a second. ARCHITECTURE §4.3 claims G-19 only while at most four are down. Check in a playtest (A-6). |
| M-3 | Risk | The display lettering uses a Windows system font (Rockwell Bold, then Georgia, then Segoe UI) that cannot be redistributed. Documented as TEMP. If none of the three is installed, the game will not start. |
| M-4 | Risk | The project has **no version control**, so there is no history and no baseline for the next team. Recommend `git init` (owner decision). |

### Low

| ID | Type | Finding |
|---|---|---|
| L-1 | Defect (documented) | The 90 ms score-board clack can sound after a restart that comes within 90 ms of a hit. Not audible in practice. |
| L-2 | Defect (documented) | Fire rate and shot timing are quantised to frames (±1 frame). At 30 fps, holding the trigger fires every 267 ms instead of every 250 ms. |
| L-3 | Risk | The 100 ms frame clamp (D-007) means a stall makes the round longer than 60 s of wall time. One example is dragging the window, because Windows blocks the loop during a drag. AC-11 (±100 ms) holds only without stalls. |
| L-4 | Opinion | Effects freeze on Pause and Results. Miss marks and "+10" tags stay frozen, dimmed, behind the board. Acceptable as a freeze-frame. |
| L-5 | Docs | The header of `PROJECT_BRIEF.md` still says "Pre-production … no implementation has started". Brief AS-2 names `docs/ASSET_SOURCES.md`, but the project uses `assets/ASSET_MANIFEST.md` and `assets/licenses/` (D-020). The substitution is documented, but the brief was never updated. |
| L-6 | Housekeeping | Stale `carnival_gallery.exe` files are left in `build/windows-msvc/{Debug,Release}`, and `dist/_CPack_Packages/` is left over too. None of them ship. |
| L-7 | Opinion | `Renderer.cpp` (~960 lines) mixes scene passes, effects, HUD and the debug overlay. Splitting out the inspection overlay and HUD boards would help maintenance. No defect. |

## 4. Area-by-area review

**Engineering.**
- **Modular:** an SFML-free logic library (`gallery_logic`: targets, shooting, clock, flow, layout, soundscape) is unit-tested headless. The SFML shell (app, assets, audio, render, platform) sits on top.
- **Ownership:** handled by declaration order in `Application`, which is documented and correct. Assets have a single owner (`AssetStore`). Audio stops every source before its buffers go. There are no raw owning pointers.
- **Rendering and hit testing** share one transform (`duckImageToLogical`), and the hit masks are built from the drawn image.
- **Dependencies:** SFML only, pinned by hash; no unnecessary dependencies or abstraction layers.
- **Allocations:** none per frame on the event path (reserved buffers, fixed effect rings).
- **Lifecycle:** clean exit 0. Debug CRT leak report empty (earlier pass).

**Gameplay.** Verified by code reading, tests and the scripted round:
- **Aim:** one pixel→logical conversion, clamped to the canvas.
- **Firing:** shots resolve before the update, against the targets as last drawn (D-008).
- **Hits:** front-most upright target only, with scenery occlusion. The 250 ms cooldown ignores extra clicks without counting them.
- **Scoring:** 10 per hit; accuracy is integer half-up, with "—" at 0 shots.
- **Clock:** splits frames at phase boundaries exactly. Low-time ticks run 10..1.
- **Pause:** freezes the clock, the targets and the rifle cooldown.
- **Restart:** resets everything in place. A resume click is never a shot, and key repeat is disabled.

**Visual quality.** Not assessable (C-1). The layer stack, proportions, lane placement, HUD placement and the warm grade and vignette shader are in place and match brief §6. The painted-board UI theme is consistent. Everything else waits for the art.

**Polish.**
- **Firing feedback:** recoil, reticle kick, a warm exposure flash, the muzzle puff (stand-in), a "+10" tag and miss dents.
- **Audio:** cues are mapped to events in the same frame (unheard; H-4).
- **Responsiveness:** input is handled before the update and drawn the same frame. Latency was not measured on a real display.

**Documentation.**
- BUILD.md and README commands are accurate: run as written in this audit.
- Dependencies, presets and the package contents are recorded.
- Every one of the 13 sounds has a licence and evidence file (CC0, Freesound) and appears in THIRD_PARTY_NOTICES.
- The decision log is thorough.
- Gaps: L-5, and the requirement deviations H-3 and M-1, which wait for the owner's sign-off.

## 5. Outstanding risks, ranked

1. The photographs may never reach the art bar (A-3). That puts the whole visual pillar at risk.
2. Performance with full-size photographs on the reference integrated GPU (H-2).
3. The audio may not suit the tone (H-4).
4. The target motion and HUD deviations are not approved (H-3, M-1).
5. No version control (M-4).

## 6. Recommended next steps (no new scope)

- Source the MVP photographs and font. Then re-run the release gate, the F3 mask check, AV-1 to AV-5 and AV-7.
- Owner decisions on D-027 (`sections` vs `chain`), G-24 (live accuracy) and EV-A01. Run the listening review.
- Measure AV-7 with the real art. If it misses, drop MSAA on the offscreen target first.
- Playtest A-4 to A-6 and G-19. `git init` and a first commit.

## 7. Confidence

| Area | Confidence | Basis |
|---|---|---|
| Game logic and rules | High | Full code read, 163 tests, scripted end-to-end round |
| State transitions and input | High | Code read and flow tests. Real-mouse play was not run (it would capture the owner's cursor) |
| Build and packaging | High | Every preset rebuilt and the package reproduced in this audit |
| Audio | Medium-low | Logic verified; never heard |
| Visual quality | Not assessable | No art exists |
| Performance | Low | No measurement with representative assets |
