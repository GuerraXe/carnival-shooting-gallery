# Engineering Verification Report — Carnival Shooting Gallery

Date: 2026-11-12 · Build: 0.1.0 (C++20, **SFML 3.0.2**; the request mentioned Raylib, but the game does not use it) · Machine: one Windows 11 x64 PC, integrated GPU driving a 60 Hz high-DPI display.

## Defects found and fixed

| ID | Defect | Evidence | Fix | Retest |
|---|---|---|---|---|
| D1 | **Hidden targets could be shot.** Hit testing ignored scenery drawn in front of the ducks. In the `chain` preset a click on a booth post scored the duck behind it. In both presets a click on a wave crest scored the duck base it covers. | `--targets chain --aim 160,562 --fire 111` → "HIT target #3 +10" with the reticle on the post (screenshot) | Foreground occlusion mask built once from the drawn wave cutout, booth frame, sign and counter; `Round::targetUnder()` (D-032) | Same command → "miss"; a visible duck in the same frame → HIT; 4 regression tests |
| D2 | **Corrupt display font accepted silently.** It was logged as "MISSING", the system font was substituted, and the game exited 0. | Garbage `fonts/display.ttf` in a copied asset root | Treated as a decode error like every other asset | Exit 1, "could not decode" |
| D3 | **`--no-dialog` ignored on an invalid command line**, so an automated run blocked on a modal box. | `--fps 0 --no-dialog` still running after 1.5 s | Flag pre-scanned before parsing | Exits 1 by itself |
| D4 | Boot time counted as the first frame's delta (clamped to 100 ms; harmless on the frozen title screen) | Code review | Frame clock restarted when the loop begins | Tests pass |
| — | My own new `FrameStats` reported 200 ms for frames over the histogram range | New unit test failed | Last bin reports the exact max | 163/163 |

Added (verification support, not a gameplay feature): a frame-time summary line in `carnival.log` at exit (AV-7 needs a frame-time log).

## Verified working

- **Build:** Debug and Release, zero warnings under `/W4 /WX /permissive-`. No suppressions added.
- **Unit tests:** 163/163 in Debug and Release (152 existing + 11 new).
- **Initialization and shutdown:** exit 0, teardown in order (audio → renderer → assets → window), Debug CRT leak report empty. Missing asset folder, missing MVP assets (Release) and corrupt files each exit 1 with a clear message.
- **Asset loading:** everything is loaded once at startup and owned by `AssetStore`. Missing optional assets are logged, never substituted.
- **Resizing and coordinates:** real OS resize to 1368×1062 and 2568×912 updated the viewport. Real OS clicks, at pixels computed independently from the letterbox formula, landed correctly: a click in a 14-unit gap did nothing, and a click 3 units inside a placard edge activated it. Unit tests cover 8 window sizes and clicks in the bars.
- **Aiming and hit detection:** one transform is shared by drawing and hit testing (R-3 test). Alpha-mask silhouettes, front-most only, scenery now blocks shots.
- **Target movement and boundaries:** shuttle and chain bounds held over 5-minute tests. No instant reversal. Knocked-down targets re-enter at the lane ends.
- **Target states:** Upright → Falling → Down → Rising at exact times, at any frame slicing. Only Upright is hittable.
- **Score and accuracy:** 10 per hit, misses never change the score, accuracy rounds half up, "—" at 0 shots.
- **Round timer:** 1.5 s countdown + 60 s + 1 s. TIME UP at exactly 61.500 s in every app run.
- **Pause and restart:** pause freezes the clock, targets **and the rifle cooldown** (new test). The Ending phase cannot be paused or shot (new test). Restart and back-to-back rounds start clean.
- **Frame-rate independence:** a full scripted round in the app at simulated 30/60/144/240 Hz gave an identical 30-shot hit/miss sequence, score and accuracy. The new unit test does the same with pause/resume.
- **Memory ownership:** member order documented and correct. Renderer construction was moved earlier in the constructor body, but declaration order (and so teardown) is unchanged. Audio stops every source before its buffers go. No raw owning pointers.

## Failed tests

- `ctest --preset release` → `asset_contract` fails on the **D-023 release gate**: 20 MVP images MISSING, 10 sounds IN_PROGRESS (awaiting your listening review). This is intended until the assets exist.

## Untested or not conclusive

- **Performance with real photographs.** None exist. I used a temporary fixture of noise images at every catalog size (27.5 MB; deleted afterwards). Its runs were interrupted: the windows were minimised or closed during the test, so only short samples are valid. Indications at 3200×1800 on the integrated GPU, uncapped: mean 11.1 ms (90 fps), p95 14–17 ms, p99 17–31 ms. One vsync run showed 21 frames over 33 ms, but that run was disturbed and can't be counted. **AV-7 (60 FPS sustained, no frame > 33 ms) is not verified.**
- **Possible optimization, not applied:** the world target and the window both use 4× MSAA at full resolution. That does nothing for photo cutouts and only smooths stand-in polygon edges and the reticle. Turning it off measured 9.7 ms (post target) and 8.4 ms (both) against 11.1 ms, but those runs were disturbed too. Not changed: the data isn't clean, and it affects how the reticle looks.
- **Audible audio:** the test environment refused the output device. Cue logic, cue counts and teardown are verified; actual sound is not.
- **Real-mouse aiming during a live round:** not run, because it would hide and confine your cursor. Conversion is verified on the boards and in unit tests, and gameplay uses the same function.

## Remaining limitations

- Fire rate is quantised to frames. At 30 fps, holding the trigger gives one shot per 267 ms (225 per round, not 240). It is never faster than 4/s.
- A shot is timed at the start of its frame, not at the exact click instant (±1 frame).
- A delayed score clack (90 ms) could sound after a restart made within 90 ms of a hit.
- Inspection layer toggles (keys 1–8) hide layers visually but the occlusion mask still applies.
- All visuals are stand-ins until the photographs are supplied.
