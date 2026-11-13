# Project Brief — Carnival Shooting Gallery (working title)

| Field | Value |
|---|---|
| Status | Pre-production. Brief only; no implementation has started |
| Version | 0.1 (draft) |
| Date | 2026-11-11 |
| Project root | `carnival-shooting-gallery/` (originally `p/`, containing an empty `main.cpp`; renamed 2026-11-12) |

---

## 1. Vision

The game is a first-person, single-player shooting gallery set at an old-fashioned American carnival. The player stands behind a wooden booth counter and looks across it at a row of mechanical duck targets moving along a track. Before anything else, the game should feel like a real place: photographic scenery, aged wood, hand-painted signs, warm incandescent bulbs, and a squeaky chain-driven target track.

Two priorities apply, in this order:

1. **Responsive gameplay.** Aiming and firing must feel immediate and fair.
2. **Visual authenticity and atmosphere.** If an effect adds input latency or makes targets hard to read, it gets cut or toned down.

A full session is short: one 60-second round, a results screen, then play again. The game should be easy to understand within the first second.

### 1.1 Experience pillars

| Pillar | What it means in practice |
|---|---|
| **A real booth** | Every visible object looks physical: wood grain, chipped paint, metal rivets, worn decals. No flat vector UI floating in the world unless it is styled as a physical object, such as a painted scoreboard. |
| **Nostalgic warmth** | Amber and tungsten light, faded reds and creams, slight film grain. It should feel like a memory of a summer evening, not a modern arcade. |
| **Snappy and fair** | The shot resolves where the player clicked, the moment they clicked. Hits are clear. Misses are clearly misses. |
| **Short and repeatable** | A 60-second round, a clear score, and one click to try again. |

### 1.2 Explicit non-goals

These are out of scope for the MVP and for the enhancements in §9 unless this brief is revised:

- Multiplayer of any kind, local or online
- Online accounts, cloud saves, online leaderboards
- Inventory, shops, currencies, unlockable weapons
- Elaborate progression: XP, levels, skill trees, meta-progression
- Free movement or camera translation. The viewpoint is fixed.
- Story or narrative campaign
- Microtransactions or ads

---

## 2. Target platform and technical framing

These are working assumptions; see §11.

- **Platform:** Windows desktop, mouse and keyboard.
- **Resolution:** 1920×1080 reference layout, scaling to other 16:9 resolutions. Other aspect ratios get letterboxing or pillarboxing in MVP.
- **Frame rate:** 60 FPS target, with no dips below 60 on a mid-range integrated GPU.
- **Rendering approach:** a 2.5D layered scene. Photographic and photo-textured sprite layers are composited at fixed depths behind a fixed camera, with optional small parallax tied to aim. True 3D is not required to deliver the vision.
- **Language and engine:** not yet decided. An empty `main.cpp` exists, which suggests C++ with a lightweight 2D/multimedia library such as SDL2 or SFML, or raylib. This decision should be made before Milestone 1.

---

## 3. Core gameplay loop

```
Title / Ready screen
      │  click "Start" (or press Space / Enter)
      ▼
Round (60 s) ── aim with mouse, click to fire, ducks cross the lane
      │  timer hits 0
      ▼
Results screen ── score, shots, hits, accuracy
      │  click "Play Again" (or press R / Enter)
      └──────────► new Round (skips the title screen)
```

---

## 4. Gameplay requirements and expected behavior (MVP)

### 4.1 Viewpoint

| ID | Requirement | Expected behavior |
|---|---|---|
| G-1 | First-person fixed viewpoint | The camera never translates or rotates in response to gameplay. The player always sees the same framing: booth counter at the bottom, target lane in the middle, carnival backdrop behind. |
| G-2 | Optional aim parallax (MVP-optional, see §9) | If implemented, background layers may shift by no more than ±1% of screen width as the cursor moves, to suggest depth. This must never move the target hitboxes relative to their drawn position. |

### 4.2 Aiming

| ID | Requirement | Expected behavior |
|---|---|---|
| G-3 | Mouse-controlled aiming | The system cursor is hidden during a round and replaced with an in-world rifle sight (reticle). The reticle sits exactly at the mouse position with no smoothing, acceleration, or lag added by the game. |
| G-4 | Cursor confinement | During a round, the cursor is confined to the game window/viewport. It is released when the game is paused, when the results screen is shown, or when the window loses focus. |
| G-5 | Visible rifle | A mounted carnival rifle is drawn in the lower foreground, anchored to the counter. It pivots slightly toward the reticle so it appears to track the aim. The rifle is decorative: the shot always lands at the reticle, not along the barrel's drawn line. |

### 4.3 Firing

| ID | Requirement | Expected behavior |
|---|---|---|
| G-6 | Left mouse button fires | Each LMB press fires exactly one shot. Holding the button does **not** auto-fire. |
| G-7 | Hitscan resolution | A shot is resolved instantly on the frame it is received, against the targets' positions on that frame. There is no projectile travel time. |
| G-8 | Fire-rate limit | A minimum interval of **250 ms** (assumed) applies between shots, representing the rifle's action. Clicks during the cooldown are ignored and **not** counted as shots. |
| G-9 | Shot feedback | Every accepted shot produces a muzzle flash, rifle recoil, a gunshot sound, and a brief reticle kick. This feedback must start within the same frame the shot is processed. |
| G-10 | Miss feedback | A shot that hits nothing leaves a small dent or puff of dust on the backboard at the impact point. It fades within about 2 seconds. Miss marks are capped at about 20 on screen. |
| G-11 | No shots outside a round | Clicks on the title and results screens operate menus only. They never count as shots. |

### 4.4 Targets

| ID | Requirement | Expected behavior |
|---|---|---|
| G-12 | Five duck targets at start | Each round begins with exactly **five** ducks on the track, evenly spaced, all visible within the lane or entering it. |
| G-13 | Defined shooting lane | Ducks travel along one horizontal track rail spanning the full width of the booth opening. The lane is clearly framed by the booth's physical opening. |
| G-14 | Movement | All ducks on the lane move in the same direction (left→right for MVP) at a constant speed. Ducks exit the lane on one side and re-enter from the other, as if on a continuous chain loop. |
| G-15 | Speed | Base speed is about **25% of lane width per second**, so a duck crosses in about 4 seconds. Speed stays constant through the round in MVP. Tuning is expected. |
| G-16 | Hit detection | Each duck has a hit region matching its painted silhouette, using a polygon or per-pixel alpha mask, not a loose rectangle. A shot inside the silhouette is a hit. Only the front-most duck under the reticle is hit by one shot, so one shot can never score more than one target. |
| G-17 | Hit reaction | A hit duck immediately flips backward on its hinge (≈150–250 ms animation), plays a metallic "ping" or "clack", and shows a brief point pop-up such as "+10" styled like a painted tag. |
| G-18 | Temporary removal | After the flip, the hit duck is **inactive**. It cannot be hit again and is not drawn standing in the lane. It continues travelling with the chain while knocked down. It returns upright when it has gone off-screen and re-entered, after at least **2 seconds** (assumed). A knocked-down duck does not block shots at ducks behind it. |
| G-19 | Lane never empty | Spacing and reset timing must ensure at least one hittable duck is always visible during the round, except for momentary edge cases of under 0.5 s. |

### 4.5 Scoring and accuracy

| ID | Requirement | Expected behavior |
|---|---|---|
| G-20 | Points per hit | Each duck hit awards **10 points** in MVP. All ducks have equal value. |
| G-21 | No penalties | Misses do not subtract points. They only lower accuracy. |
| G-22 | Tracking | The game tracks shots fired (accepted shots only, per G-8), hits, and score. |
| G-23 | Accuracy formula | Accuracy = hits ÷ shots fired × 100, shown as a whole-number percentage, rounded half up. If 0 shots were fired, accuracy displays as **"—"**, not 0% or an error. |
| G-24 | Live display | Score and time remaining are visible throughout the round. Accuracy may be shown live or only on results; MVP shows it **only on results** to keep the HUD minimal. |

### 4.6 Round timer

| ID | Requirement | Expected behavior |
|---|---|---|
| G-25 | 60-second round | The timer starts at 60 and counts down in whole seconds on screen. Internally it is tracked at frame precision. |
| G-26 | Start | The timer starts after a short **"Ready… Go!"** cue of about 1.5 s. Ducks are already moving during the cue, but shots are not accepted until "Go". |
| G-27 | Low-time warning | In the final 10 seconds, the timer display changes, for example to red paint or a pulse, and a ticking or bell cue plays. |
| G-28 | End | At 0, input is disabled immediately. Shots in the final frame count if they were received before the timer reached 0. A buzzer or bell sounds, ducks stop, and the results screen appears after about 1 s. |
| G-29 | Pause on focus loss | If the window loses focus, the round pauses: timer frozen, ducks frozen, cursor released. When focus returns, a "Click to resume" overlay appears. The resume click does not fire a shot. |

### 4.7 Results and restart

| ID | Requirement | Expected behavior |
|---|---|---|
| G-30 | Results screen | Shows final **score**, **hits**, **shots fired**, and **accuracy %**. It is styled as a physical object, such as a painted prize board or a ticket. |
| G-31 | Session best | Shows the best score of the current session, and notes "New best!" when it is beaten. It is not saved to disk in MVP. |
| G-32 | Restart | A "Play Again" button, plus R or Enter, starts a fresh round. All state is reset: score, shots, hits, timer, duck positions, miss marks. Restart is instantaneous, with no loading delay over 0.5 s. |
| G-33 | Quit | Esc on the results or title screen exits the game, or shows a quit confirmation. During a round, Esc pauses. |

---

## 5. Controls

| Input | Title screen | During round | Paused | Results screen |
|---|---|---|---|---|
| Mouse move | Move cursor | Aim reticle | Move cursor | Move cursor |
| Left click | Press buttons | **Fire** | Resume (no shot) | Press buttons |
| Space / Enter | Start | — | Resume | Play again |
| R | — | — | Restart round | Play again |
| Esc | Quit | Pause | Quit to title | Quit |

Right mouse button and the scroll wheel are unused in MVP. Key rebinding is not in MVP.

---

## 6. Screen layout

Reference frame is 1920×1080. The layers, from nearest to farthest:

```
┌───────────────────────────────────────────────────────────────┐
│  BACKGROUND: midway at dusk: Ferris wheel, string lights,     │
│  tents, distant crowd (photographic, slightly soft focus)     │
│ ┌───────────────────────────────────────────────────────────┐ │
│ │ BOOTH HEADER: painted sign "SHOOT THE DUCKS"; bulbs;      │ │
│ │ striped awning edge                    [SCORE board]      │ │
│ ├───────────────────────────────────────────────────────────┤ │
│ │ BACKBOARD (painted wooden scenery: pond, cattails)        │ │
│ │                                                           │ │
│ │  ═══ 🦆 ═════ 🦆 ═════ 🦆 ═════ 🦆 ═════ 🦆 ═══  ← target lane  │
│ │  ▓▓▓▓▓▓▓▓▓▓ track rail / wave cutout ▓▓▓▓▓▓▓▓▓▓▓▓          │ │
│ │                        ⊕ reticle                          │ │
│ ├───────────────────────────────────────────────────────────┤ │
│ │ BOOTH SIDE POSTS frame the opening left and right         │ │
├─┴───────────────────────────────────────────────────────────┴─┤
│  FOREGROUND: worn wooden counter; mounted rifle (lower right);│
│  [TIMER: tin clock or flip sign, lower left]                  │
└───────────────────────────────────────────────────────────────┘
```

### 6.1 Layer stack (back to front)

| # | Layer | Content | Notes |
|---|---|---|---|
| 1 | Sky / far background | Dusk sky, Ferris wheel silhouette, distant lights | Photographic, lowest contrast, slightly blurred |
| 2 | Mid background | Neighboring tents, string lights, crowd silhouettes | Photographic, mild blur |
| 3 | Booth frame and header | Painted sign, bulbs, awning, side posts | Sharp; defines the playable opening |
| 4 | Backboard | Painted pond scenery | Receives miss marks |
| 5 | **Target lane** | Ducks plus the rail behind the wave cutout | Highest contrast and sharpest focus in the scene |
| 6 | Foreground wave strip | Wooden wave cutout hiding the duck bases/chain | Painted blue wood |
| 7 | Counter | Aged wooden counter | Fills the bottom ~18% of the frame |
| 8 | Rifle | Mounted rifle on swivel | Lower right; must not cover the lane |
| 9 | Diegetic HUD | Score board (top right, on booth), timer (bottom left, on counter) | Styled as physical objects |
| 10 | Reticle | Rifle sight | Always on top; never obscured |
| 11 | Post-process | Film grain, vignette, warm grade | Applied to layers 1–9 only. **Must not apply to the reticle.** |

### 6.2 Layout rules

- The target lane sits in the vertical middle third of the screen, roughly 40–60% of screen height.
- Neither the rifle nor the HUD may overlap the lane at any aim position.
- The score must be legible at a glance: about 48 px or more at 1080p.
- The timer must be legible at a glance and readable in peripheral vision.
- Overlays for Title, Pause, and Results appear in the center of the screen, styled as a painted board or a hanging sign. The gameplay scene stays dimmed behind them.

---

## 7. Visual direction

### 7.1 Overall look

A **warm, slightly faded photograph of a carnival booth at dusk**, in the style of mid-century Americana: a 1950s–60s traveling midway. It should read as "real place, softly remembered", not cartoon and not hyper-clean CG.

### 7.2 Materials and objects

| Element | Direction |
|---|---|
| Wood | Weathered planks, visible grain, worn edges where hands rest, flaking paint |
| Painted signage | Hand-lettered circus/showcard type (slab serifs, drop shadows), sun-faded reds, creams, golds |
| Ducks | Flat sheet-metal or plywood cutouts in yellow or white with orange bills, painted eyes, chipped edges, a visible hinge/bracket at the base. Several slight paint variations so they do not look cloned. |
| Metal | Rivets, rust bloom, scuffed nickel on the rifle |
| Lights | Exposed incandescent bulbs in sockets; warm glow halos; one or two that flicker subtly |
| Decorations | Bunting, striped awning, plush prizes hanging at the edges (static), pennants |

### 7.3 Color palette (guide, not final)

| Role | Color direction |
|---|---|
| Light | Tungsten amber (~2700K) |
| Primary accent | Faded carnival red |
| Secondary | Cream / off-white, mustard gold |
| Cool contrast | Muted teal/blue for the wave cutouts and dusk sky |
| Shadows | Warm brown-black, never pure black |

Ducks must keep the strongest value contrast in the scene against the backboard. Readability of targets overrides palette fidelity.

### 7.4 Lighting

- One dominant warm key light from the booth header bulbs, shining down and forward.
- All layers must agree on light direction. Duck highlights and shadows match the bulbs above.
- Ducks cast a soft shadow onto the backboard so they sit in the space.
- Background is cooler and darker, so the booth reads as the warm, lit focus.

### 7.5 Photographic treatment (post-process)

| Effect | MVP strength | Rule |
|---|---|---|
| Film grain | Subtle, animated | Not visible as noise on the reticle or the HUD numbers |
| Vignette | Gentle, ~15–25% corner darkening | Must not darken lane ends enough to hide ducks |
| Warm color grade | Slight lift in blacks, warm midtones | Applied globally except the reticle |
| Dust / scratches / light leaks | **Enhancement only** (see §9) | Must never be mistaken for a target |
| Depth of field | Baked into the background art; not real-time | — |

### 7.6 Perspective and scale

- Single eye-level viewpoint: the player's eyes are about 30–45 cm above the counter, roughly 3–4 m from the backboard.
- All photographic plates and object art must share this horizon line and vanishing point.
- Ducks are about 25–30 cm tall in the world, which is about 8–10% of screen height at 1080p.
- Real-world references, such as a bulb at ~6 cm or a counter depth of ~50 cm, keep scale consistent across assets.

### 7.7 Asset sourcing (constraint)

All imagery must be original, licensed, or public-domain with recorded provenance. Do not copy recognizable branding from real carnivals, toy manufacturers, or existing games. Keep a `docs/ASSET_SOURCES.md` log once assets are added.

---

## 8. Audio (MVP minimum)

Audio is not the focus of this brief, but feedback depends on it:

- Rifle shot (cork or air-rifle pop, not a firearm blast)
- Duck hit "ping" or "clack", plus the flip
- Miss "thud" on wood
- Track chain loop (quiet, continuous)
- Ambient carnival bed: distant calliope or organ, crowd murmur (low, loopable)
- Round start bell, final-10-seconds ticks, end buzzer/bell
- UI click

A master volume control and mute (M) are an enhancement unless trivial to add.

---

## 9. MVP vs. later enhancements

### 9.1 MVP (first playable). Everything here is required.

- Fixed first-person booth view with layered scene (§6), using placeholder or early photographic art
- Mouse aim with an in-world reticle; LMB single-shot fire with a cooldown
- Five ducks on one left→right looping lane at constant speed
- Silhouette-accurate hit detection; flip-down reaction; temporary removal and re-entry
- 10 points per hit; shots, hits, and accuracy tracked
- 60-second timer with Ready/Go start, low-time warning, and end buzzer
- Title screen → round → results → play again loop
- Pause on Esc and on focus loss
- Warm grade, subtle grain, and vignette
- Core sound effects (§8)
- Session-only best score

**MVP art bar:** the scene must already show the layered composition, warm lighting, and wood/paint materials. Placeholder art is fine if it is photographic in character, for example processed reference photos or quickly textured cutouts, not flat colored rectangles. Final-quality art is not required.

### 9.2 Enhancement tier 1 (polish, same game)

- Final-quality photographic art and duck variations
- Aim parallax on background layers (G-2)
- Dust, scratches, and occasional light-leak overlays
- Flickering bulbs, swaying bunting, subtle smoke or haze
- Wooden "+10" tags and a score board with mechanical flip-digit animation
- Persistent local best score (saved to a local file)
- Volume and mute settings, plus a fullscreen/windowed toggle
- Support for other aspect ratios (extend the art instead of letterboxing)

### 9.3 Enhancement tier 2 (light gameplay variety, still single-session)

- Second lane moving the opposite direction, at a different depth
- Bonus target types, such as a golden duck worth more or a fast-moving small target
- Speed increase over the round
- Combo/streak bonus for consecutive hits
- Limited "ammo per round" variant with reload animation, as an optional mode

All tier-2 items must stay within the non-goals in §1.2: no accounts, no inventory, no meta-progression.

---

## 10. Acceptance criteria (MVP)

The MVP is accepted when **all** of the following can be shown in a build on the target platform.

### 10.1 Functional

| # | Criterion | Verification |
|---|---|---|
| AC-1 | Game launches to a title screen; Start begins a round within 0.5 s | Manual |
| AC-2 | The camera/view does not change position during play | Manual |
| AC-3 | Reticle tracks the mouse 1:1 with no visible lag; system cursor is hidden during a round and confined to the window | Manual + latency check (AC-14) |
| AC-4 | Each LMB press fires exactly one shot; holding does not auto-fire; clicks during the 250 ms cooldown are ignored and not counted | Automated (input/rate logic) + manual |
| AC-5 | Exactly five ducks are present at round start and move left→right at constant speed, looping continuously | Automated (state at t=0) + manual |
| AC-6 | A click inside a duck's painted silhouette scores a hit; a click in the transparent area of its bounding box is a miss | Automated (hit-test against mask) |
| AC-7 | One shot hits at most one duck, the front-most | Automated |
| AC-8 | A hit duck flips down within 250 ms, cannot be hit again until it resets, and returns upright only after re-entering the lane with ≥ 2 s elapsed | Automated (state machine) + manual |
| AC-9 | Each hit adds exactly 10 points; misses do not change the score | Automated |
| AC-10 | Accuracy = round(hits ÷ shots × 100); "—" shown when shots = 0 | Automated (incl. 0-shot, 100%, rounding cases) |
| AC-11 | Timer starts at 60 after the Ready/Go cue, reaches 0 within ±100 ms of 60 s wall time, then disables firing | Automated (simulated clock) + manual stopwatch |
| AC-12 | Results screen shows score, hits, shots, accuracy, and session best, all matching the round's actual events | Automated (scripted input sequence) + manual |
| AC-13 | Play Again (button, R, or Enter) fully resets state; ten back-to-back rounds show no carry-over of score, shots, miss marks, or duck state | Manual + automated reset test |
| AC-14 | Click-to-feedback (muzzle flash and sound) is perceived as immediate; measured input-to-visible-flash ≤ 2 frames at 60 FPS | Manual measurement (high-speed capture or frame logging) |
| AC-15 | Losing window focus pauses the round; resuming does not fire a shot or lose timer time | Manual |
| AC-16 | Clicks on title, pause, and results screens never count as shots | Automated + manual |

### 10.2 Visual

| # | Criterion | Verification |
|---|---|---|
| AV-1 | The scene shows at least distinct background, booth, lane, and foreground counter layers | Visual review |
| AV-2 | Lighting reads as warm incandescent from above; duck highlights and shadows agree with the bulbs | Visual review |
| AV-3 | Ducks are clearly the most readable elements on screen; 5 out of 5 playtesters identify the targets within 2 s without instruction | Playtest |
| AV-4 | Film grain, vignette, and warm grade are present but do not affect the reticle or obscure any duck at the lane ends | Visual review at lane extremes |
| AV-5 | All plates share one horizon and consistent scale; no layer looks pasted in at the wrong size | Visual review |
| AV-6 | Score and timer are readable at 1080p from normal seated distance | Visual review |
| AV-7 | 60 FPS sustained for a full round on the reference machine, with no frame over 33 ms | Frame-time log |

### 10.3 Scope

| # | Criterion |
|---|---|
| AS-1 | No features from the non-goals list (§1.2) are present |
| AS-2 | Every image and sound asset has a recorded source or license in `docs/ASSET_SOURCES.md` |

---

## 11. Assumptions to revisit during development

Each of these is a placeholder decision made so the brief is complete. Each should be confirmed or changed at the milestone listed.

| # | Assumption | Why it matters | Revisit at |
|---|---|---|---|
| A-1 | **Tech stack:** C++ with a lightweight 2D library (inferred only from an empty `main.cpp`) | Drives everything: rendering, shader post-process, audio, build tooling | Before any code (Milestone 0) |
| A-2 | **2.5D layered sprites** are enough for "physical-looking"; no real-time 3D | Real 3D would increase art and engineering cost substantially | After the first composited mock frame |
| A-3 | **Photographic art is obtainable.** Sourcing licensed or original photos of booths, ducks, and midways is feasible within budget | Biggest risk to the visual pillar; may require shooting or modeling and rendering assets instead | Before art production; prototype with placeholders |
| A-4 | Five ducks at 25% of lane width per second with a 250 ms fire cooldown produces a fun difficulty | Untested numbers | First playtest |
| A-5 | 10 points per hit, no miss penalty | Score range may feel flat; a miss penalty might be needed to make accuracy matter | First playtest |
| A-6 | Hit ducks re-enter after one lap plus ≥ 2 s | May leave the lane too empty or too full | First playtest |
| A-7 | Hitscan at the reticle, rifle purely decorative | Players might expect the shot to follow the barrel; check whether the decorative rifle confuses aim | First playtest |
| A-8 | Ducks move only left→right on a single lane | One lane may get monotonous within 60 s; a second lane is tier 2 | After MVP playtest |
| A-9 | Accuracy is hidden during the round, shown only on results | Some players may want live feedback | First playtest |
| A-10 | 16:9 with letterboxing is acceptable for MVP | Ultrawide or 16:10 players see bars | Before any public release |
| A-11 | Session-only best score is enough for MVP | Persisting locally is cheap but adds file I/O and a settings location | End of MVP |
| A-12 | Post-process (grain, vignette, grade) fits within the frame budget on integrated GPUs | Full-screen shaders can be expensive at high resolutions | Performance pass after first composited frame |
| A-13 | Air-rifle or cork-gun sound fits the tone better than a firearm sound | Tone and audience appropriateness | Audio pass |
| A-14 | Reference machine definition (mid-range integrated GPU, Windows 11) | Needed to make AV-7 testable | Milestone 0 |
| A-15 | The working title and the "SHOOT THE DUCKS" sign text are placeholders | Branding and trademark check | Before any public release |
| A-16 | Esc during a round pauses rather than quits | Players may expect different behavior | First playtest |

---

## 12. Proposed milestone outline (for planning only)

These are not commitments. They exist to sequence the MVP so the riskiest parts are proven first.

| Milestone | Goal | Exit check |
|---|---|---|
| M0 | Decide stack (A-1) and reference machine (A-14); create an empty window that renders and reads the mouse | Window opens at 60 FPS |
| M1 | Static layered scene with placeholder art and warm grade | AV-1, AV-5 roughly met |
| M2 | Reticle, firing, cooldown, feedback | AC-3, AC-4, AC-14 |
| M3 | Ducks on the lane, hit detection, hit reaction, re-entry | AC-5 to AC-8 |
| M4 | Score, timer, results, restart, pause | AC-9 to AC-13, AC-15, AC-16 |
| M5 | Audio, grain and vignette, art upgrade pass, first playtest | All of §10 met; §11 assumptions reviewed |

---

## 13. Glossary

| Term | Meaning |
|---|---|
| Lane | The horizontal track the ducks travel along |
| Shot | An accepted LMB press during an active round, outside the cooldown |
| Hit | A shot whose point falls inside an active duck's silhouette |
| Active duck | A duck standing upright and hittable |
| Diegetic HUD | Score and timer drawn as physical objects in the scene, not floating overlay text |
| Plate | A single photographic background layer |
