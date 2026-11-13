# Asset Manifest — Carnival Shooting Gallery

This is the single source of truth for every asset: its purpose, source, license, modifications, and status. The rules and specifications behind it are in [`docs/ASSET_GUIDE.md`](../docs/ASSET_GUIDE.md). This file replaces the `docs/ASSET_SOURCES.md` named in the brief (§7.7, AS-2) and the architecture (D-020).

- **Last updated:** 2026-11-12
- **Counts:** 13 audio assets acquired (`IN_PROGRESS`, awaiting the owner's listening review) · 1 audio `CANDIDATE` (music) · 0 placeholders · 27 entries still `MISSING` (26 images, 1 font) · 1 reference · 4 rejected image candidates · 10 evaluated audio candidates (§2.1)
- **Status values:** `MISSING` → `CANDIDATE` → `VERIFIED` → `IN_PROGRESS` → `READY`. Also `PLACEHOLDER`, `REFERENCE_ONLY`, `REJECTED` (see Asset Guide §5.2)
- **Rule:** never fill a source or license field by guesswork. Write `unknown`, and the asset cannot advance past `CANDIDATE`.

---

## 1. Required assets

The columns *Source*, *Creator*, *License*, *Attribution*, *Modifications*, *Original SHA-256*, and *Evidence* are filled in as each asset advances. They are blank (`—`) while an asset is `MISSING`.

### 1.1 Images

| ID | Purpose | Runtime path | Prio | Status | Source | Creator | License | Attribution required | Modifications | Original SHA-256 | Evidence |
|---|---|---|---|---|---|---|---|---|---|---|---|
| BG_FAR | Dusk sky, ride silhouettes, distant lights (layer 1) | images/backgrounds/bg_far_dusk.jpg | MVP | MISSING | — | — | — | — | — | — | — |
| BG_MID | Tents, string lights, crowd silhouettes (layer 2) | images/backgrounds/bg_mid_midway.png | MVP | MISSING | — | — | — | — | — | — | — |
| FG_BOOTH_FRAME | Booth posts, header beam, awning, framing the opening (layer 3) | images/foreground/booth_frame.png | MVP | MISSING | — | — | — | — | — | — | — |
| FG_BACKBOARD | Weathered painted planks behind the lane (layer 4) | images/foreground/backboard.jpg | MVP | MISSING | — | — | — | — | — | — | — |
| FG_TRACK_RAIL | Tileable metal rail and chain (layer 5) | images/foreground/track_rail.png | MVP | MISSING | — | — | — | — | — | — | — |
| FG_WAVE_STRIP | Blue-painted wooden wave cutout (layer 6) | images/foreground/wave_strip.png | MVP | MISSING | — | — | — | — | — | — | — |
| FG_COUNTER | Worn wooden counter, shooter's view (layer 7) | images/foreground/counter.png | MVP | MISSING | — | — | — | — | — | — | — |
| FG_RIFLE | Unbranded mounted carnival rifle seen from behind along the barrel (D-039); swivel + muzzle points TBD; silhouette within ±70 px of that line (layer 8) | images/foreground/rifle.png | MVP | MISSING | — | — | — | — | — | — | — |
| TGT_DUCK_A | Duck target, paint variant A; alpha = hit mask | images/targets/duck_a.png | MVP | MISSING | — | — | — | — | — | — | — |
| TGT_DUCK_B | Duck target, paint variant B | images/targets/duck_b.png | E1 | MISSING | — | — | — | — | — | — | — |
| TGT_DUCK_C | Duck target, paint variant C | images/targets/duck_c.png | E1 | MISSING | — | — | — | — | — | — | — |
| DEC_SIGN_HEADER | Hand-lettered booth header sign | images/decorations/sign_header.png | MVP | MISSING | — | — | — | — | — | — | — |
| DEC_BULB_LIT | Lit incandescent bulb in socket | images/decorations/bulb_lit.png | MVP | MISSING | — | — | — | — | — | — | — |
| DEC_BULB_GLOW | Real bulb bloom on black (additive) | images/decorations/bulb_glow.png | MVP | MISSING | — | — | — | — | — | — | — |
| DEC_BUNTING | Fabric bunting or pennants | images/decorations/bunting.png | E1 | MISSING | — | — | — | — | — | — | — |
| DEC_PRIZES | Generic, unbranded hanging plush prizes | images/decorations/prizes_*.png | E1 | MISSING | — | — | — | — | — | — | — |
| UI_SCORE_BOARD | Painted score board (pre-graded) | images/ui/score_board.png | MVP | MISSING | — | — | — | — | — | — | — |
| UI_TIMER_BOARD | Timer plate or sign (pre-graded) | images/ui/timer_board.png | MVP | MISSING | — | — | — | — | — | — | — |
| UI_PANEL | Board for title, pause, and results (pre-graded) | images/ui/panel_board.png | MVP | MISSING | — | — | — | — | — | — | — |
| UI_RETICLE | Aiming reticle; **functional exception pending Q-2** | images/ui/reticle.png | MVP | MISSING | — | — | — | — | — | — | — |
| OV_GRAIN | Scanned real film grain, tileable | images/overlays/film_grain.png | MVP | MISSING | — | — | — | — | — | — | — |
| OV_MISS_MARK | Pellet dent or splinter in painted wood | images/overlays/miss_mark.png | MVP | MISSING | — | — | — | — | — | — | — |
| OV_MUZZLE_PUFF | Real smoke or cork puff on black | images/overlays/muzzle_puff.png | MVP | MISSING | — | — | — | — | — | — | — |
| OV_DUST_SCRATCH | Scanned film dust and scratches | images/overlays/dust_scratch_*.png | E1 | MISSING | — | — | — | — | — | — | — |
| OV_LIGHT_LEAK | Photographed light leaks | images/overlays/light_leak_*.jpg | E1 | MISSING | — | — | — | — | — | — | — |
| OV_HIT_TAG | Painted wooden "+10" tag | images/ui/hit_tag.png | E1 | MISSING | — | — | — | — | — | — | — |
| OV_MUZZLE_FLASH | Real muzzle flash photographed on black, additive, a few frames (D-031). Optional: an air rifle shows little flame, so without it the flash is an exposure lift only | images/overlays/muzzle_flash.png | E1 | MISSING | — | — | — | — | — | — | — |

### 1.2 Audio

| ID | Purpose | Runtime path | Prio | Status | Source | Creator | License | Attribution required | Modifications | Original SHA-256 | Evidence |
|---|---|---|---|---|---|---|---|---|---|---|---|
All audio below was acquired 2026-11-11 (D-030). Every file is a real recording, CC0, cut from an untouched original in `asset-src/originals/audio/` by one recorded command in [`asset-src/work/audio/prep.sh`](../asset-src/work/audio/prep.sh). Status stays `IN_PROGRESS` until the owner's listening review (Asset Guide §6), because segments were chosen by level and transient analysis, not by ear. The originals are Freesound's public HQ MP3 previews; the lossless originals need a Freesound login.

| ID | Purpose | Runtime path | Prio | Status | Source | Creator | License | Attribution required | Modifications | Original SHA-256 | Evidence |
|---|---|---|---|---|---|---|---|---|---|---|---|
| SFX_SHOT | Air-rifle shot | audio/effects/shot.wav | MVP | IN_PROGRESS (listening review) | https://freesound.org/people/FOSSarts/sounds/750571/ | FOSSarts | CC0 1.0 | No | 0.884–1.400 s, lead trim, fades, mono, −1 dBFS | 260240c0e5760332… | [SFX_SHOT.md](licenses/SFX_SHOT.md) |
| SFX_DUCK_HIT | Metallic ping | audio/effects/duck_hit.wav | MVP | IN_PROGRESS (listening review) | https://freesound.org/people/JarredGibb/sounds/217459/ | JarredGibb | CC0 1.0 | No | whole, lead trim, fades, mono, −1 dBFS | 4ad8d3b057a81349… | [SFX_DUCK_HIT.md](licenses/SFX_DUCK_HIT.md) |
| SFX_DUCK_FLIP | Hinged target slamming flat | audio/effects/duck_flip.wav | MVP | IN_PROGRESS (listening review) | https://freesound.org/people/RutgerMuller/sounds/50736/ | RutgerMuller | CC0 1.0 | No | whole, fades, mono, −1 dBFS | 761395b794468938… | [SFX_DUCK_FLIP.md](licenses/SFX_DUCK_FLIP.md) |
| SFX_DUCK_RESET | Hinge rattle as the target swings back up | audio/effects/duck_reset.wav | E1 | IN_PROGRESS (listening review) | https://freesound.org/people/alienistcog/sounds/124710/ | alienistcog | CC0 1.0 | No | 2.780–3.750 s, fades, mono, −1 dBFS | 950b0b89c7341156… | [SFX_DUCK_RESET.md](licenses/SFX_DUCK_RESET.md) |
| SFX_MISS | Pellet into wood | audio/effects/miss_thud.wav | MVP | IN_PROGRESS (listening review) | https://freesound.org/people/TRP/sounds/573835/ | TRP | CC0 1.0 | No | 0.075–0.360 s, fades, mono, −1 dBFS | 194a061b61f24d61… | [SFX_MISS.md](licenses/SFX_MISS.md) |
| SFX_CHAIN_LOOP | Mechanical track loop | audio/effects/chain_loop.wav | MVP | IN_PROGRESS (listening review) | https://freesound.org/people/kyles/sounds/452238/ | kyles | CC0 1.0 | No | 20.0–28.0 s, 0.5 s loop crossfade, mono, −3 dBFS | 0ed88c225fba9191… | [SFX_CHAIN_LOOP.md](licenses/SFX_CHAIN_LOOP.md) |
| SFX_BELL_START | Round start bell | audio/effects/bell_start.wav | MVP | IN_PROGRESS (listening review) | https://freesound.org/people/TRP/sounds/571096/ | TRP | CC0 1.0 | No | 1.684–4.600 s, fades, mono, −1 dBFS | e2f56ab6d771c00c… | [SFX_BELL_START.md](licenses/SFX_BELL_START.md) |
| SFX_TICK | Final-10-seconds tick | audio/effects/tick.wav | MVP | IN_PROGRESS (listening review) | https://freesound.org/people/fauxpress/sounds/42139/ | fauxpress | CC0 1.0 | No | 5.490–5.800 s, fades, mono, −1 dBFS | 889622967f81b08c… | [SFX_TICK.md](licenses/SFX_TICK.md) |
| SFX_END | End bell (rung twice) | audio/effects/end_bell.wav | MVP | IN_PROGRESS (listening review) | https://freesound.org/people/TRP/sounds/571096/ | TRP | CC0 1.0 | No | 229.300–234.400 s, fades, mono, −1 dBFS | e2f56ab6d771c00c… | [SFX_END.md](licenses/SFX_END.md) |
| SFX_UI_CLICK | Wooden UI click | audio/effects/ui_click.wav | MVP | IN_PROGRESS (listening review) | https://freesound.org/people/Anzbot/sounds/565027/ | Anzbot | CC0 1.0 | No | 2.715–2.900 s, fades, mono, −1 dBFS | 00820a3855330c0e… | [SFX_UI_CLICK.md](licenses/SFX_UI_CLICK.md) |
| SFX_SCORE_TICK | Score board counter clack after a hit | audio/effects/score_tick.wav | E1 | IN_PROGRESS (listening review) | https://freesound.org/people/kyles/sounds/637735/ | kyles | CC0 1.0 | No | 30.420–30.720 s, fades, mono, −1 dBFS | 6bc4b03538bdad57… | [SFX_SCORE_TICK.md](licenses/SFX_SCORE_TICK.md) |
| SFX_RESULTS | Register bell: the score rung up | audio/effects/results_ding.wav | E1 | IN_PROGRESS (listening review) | https://freesound.org/people/kyles/sounds/453763/ | kyles | CC0 1.0 | No | 0.863–2.100 s, fades, mono, −1 dBFS | e22c0138cebad95f… | [SFX_RESULTS.md](licenses/SFX_RESULTS.md) |
| AMB_MIDWAY | Fairground crowd and machinery loop | audio/ambience/midway_loop.ogg | MVP | IN_PROGRESS (listening review: speech check) | https://freesound.org/people/HECKFRICKER/sounds/635159/ | HECKFRICKER | CC0 1.0 | No | 125–215 s, 3 s loop crossfade, stereo, −3 dBFS, Ogg | 8ed87898d5baf866… | [AMB_MIDWAY.md](licenses/AMB_MIDWAY.md) |
| MUS_CALLIOPE | Distant band organ or calliope (composition **and** recording cleared) | audio/music/calliope_loop.ogg | E1 (optional; was MVP-droppable, D-030) | CANDIDATE (see §2, EV-A10) | https://freesound.org/people/CHallSmith/sounds/870752/ | CHallSmith | CC0 1.0 (recording) | No | — not exported | 097524a6015f80f5… (preview) | — |

Full SHA-256 values are in each evidence file. The full CC0 1.0 legal code is [`licenses/CC0-1.0.txt`](licenses/CC0-1.0.txt) (downloaded from creativecommons.org 2026-11-11).

### 1.3 Fonts

| ID | Purpose | Runtime path | Prio | Status | Source | Creator | License | Attribution required | Modifications | Original SHA-256 | Evidence |
|---|---|---|---|---|---|---|---|---|---|---|---|
| FNT_DISPLAY | Carnival showcard lettering for HUD and screens | fonts/display.ttf | MVP | MISSING (candidate "Rye", license **not yet verified**) | — | — | — | — | — | — | — |

---

## 2. Evaluated candidates (reference and rejected)

Recorded so they are not evaluated again. None of these files are stored in the project.

| Ref | File / title | Source URL | Creator | License (as stated on the file page, 2026-11-11) | Status | Reason |
|---|---|---|---|---|---|---|
| EV-01 | Shooting Gallery called Hunting in the Cascades, Pay Streak, Alaska Yukon Pacific Exposition, Seattle, Washington, 1909 (AYP 62).jpeg | https://commons.wikimedia.org/wiki/File:Shooting_Gallery_called_Hunting_in_the_Cascades,_Pay_Streak,_Alaska_Yukon_Pacific_Exposition,_Seattle,_Washington,_1909_(AYP_62).jpeg | Frank H. Nowell (1864–1950); held by University of Washington Special Collections | Public domain: published in the US before 1931; author died 1950. Tagged Public Domain Mark 1.0 | REFERENCE_ONLY | Excellent period mood reference (booth arch, "2 SHOTS FOR 5¢"), but only 409×512 grayscale. A higher-resolution UW scan was not searched for |
| EV-02 | Le stand de tir aux canards à la fête foraine.JPG | https://commons.wikimedia.org/wiki/File:Le_stand_de_tir_aux_canards_%C3%A0_la_f%C3%AAte_foraine.JPG | Dinkum (Commons user), own work, 2012-10-21 | CC0 1.0 | REJECTED (layout reference only) | Shows third-party copyrighted cartoon characters (mural) and branded toy packaging; cold fluorescent and LED light; modern plastic ducks; oblique angle |
| EV-03 | Le stand de tir aux canards à la fête foraine 2.JPG | https://commons.wikimedia.org/wiki/File:Le_stand_de_tir_aux_canards_%C3%A0_la_f%C3%AAte_foraine_2.JPG | Dinkum (Commons user), own work, 2012-10-21 | CC0 1.0 | REJECTED | Same as EV-02, plus a child in the frame |
| EV-04 | Gun at funfair.jpg | https://commons.wikimedia.org/wiki/File:Gun_at_funfair.jpg | emina visnic (Flickr "missbenefit") | CC BY 2.0 | REJECTED | Identifiable person; wrong viewpoint for the rifle; 1600×1200 |
| EV-05 | Wild West Shooting Gallery.jpg | https://commons.wikimedia.org/wiki/File:Wild_West_Shooting_Gallery.jpg | Author field "Zaid Hamid"; PD release by uploader "Pdanial" | Public domain (PD-user) | REJECTED | Provenance inconsistent (named author ≠ releasing uploader); 640×480 |

### 2.1 Audio candidates evaluated 2026-11-11 and not used

| Ref | File / title | Source URL | Creator | License (as stated, 2026-11-11) | Status | Reason |
|---|---|---|---|---|---|---|
| EV-A01 | Wurstelprater Wien 2024 Luftgewehr Pressluft schießt zweimal.ogg | https://commons.wikimedia.org/wiki/File:Wurstelprater_Wien_2024_Luftgewehr_Pressluft_schie%C3%9Ft_zweimal.ogg | DrTrumpet | CC BY-SA 4.0 | CANDIDATE (owner decision, tier C) | **The most authentic shot found:** a real air rifle at the Vienna Prater shooting range, 2024, Zoom H6. Share-alike needs owner approval (Asset Guide §3.1 tier C, like Q-5). Would replace SFX_SHOT if approved |
| EV-A02 | Carl Frei kermisorgel – SoundCloud – Thijs Haenen.ogg | https://commons.wikimedia.org/wiki/File:Carl_Frei_kermisorgel_-_SoundCloud_-_Thijs_Haenen.ogg | Thijs Haenen | CC BY-SA 3.0 | REJECTED | BY-SA (tier C), and the tune ("Mina de Rumbakoningin") is not shown to be public domain |
| EV-A03 | Margie on JBS steam calliope.ogg / Calliope tune.ogg | https://commons.wikimedia.org/wiki/File:Margie_on_JBS_steam_calliope.ogg | Calliopeman316 / Walter van den Broek | CC BY-SA 3.0 | REJECTED | BY-SA; "Margie" (1920) composition status not verified |
| EV-A04 | Homer Denney / Island Queen rags (various) | https://commons.wikimedia.org/wiki/File:Water_Queen_by_Homer_Denney_(1906).ogg | uploader's renderings | Public domain (as labelled) | REJECTED | Renderings from notation software (MuseScore source linked), i.e. synthesised, not a recording of a real instrument |
| EV-A05 | Craig Smith "SSE", "Gold" and "Red" library effects (e.g. S31-13 Shooting gallery.wav) | https://freesound.org/people/craigsmith/sounds/675479/ | uploaded by craigsmith; created by Hollywood studio editors, 1930s–80s | CC0 (as labelled by the uploader) | REJECTED | Provenance: the uploader digitised effects made by others (donated to USC). Same rule as EV-05: the releasing uploader is not the creator |
| EV-A06 | Air Rifle | https://freesound.org/people/Fission9/sounds/466310/ | Fission9 | CC0 | REJECTED (in favour of SFX_SHOT) | Powerful rifle in an open valley with long echo and a supersonic crack: wrong for a booth |
| EV-A07 | Fun fair at night, in the park Altánek Stromovka (Prague) | https://freesound.org/people/felix.blume/sounds/466054/ | felix.blume | CC0 | REJECTED (alternative for AMB_MIDWAY) | Excellent recording, but "musics are mixing together": recognisable recorded music is against the AMB_MIDWAY spec |
| EV-A08 | Carnival Midway Food Court and Nearby Midway Games | https://freesound.org/people/PostProdDog/sounds/580720/ | PostProdDog | CC0 | REJECTED (alternative for AMB_MIDWAY) | A barker explains a game in intelligible speech |
| EV-A09 | Bell Spittoon Metal Target Being Hit Sound Effect | https://freesound.org/people/deleted_user_7146007/sounds/383338/ | deleted user | CC0 (as labelled) | REJECTED | Uploader account deleted: provenance cannot be checked |
| EV-A10 | Large Band Organ at Carousel | https://freesound.org/people/CHallSmith/sounds/870752/ | CHallSmith | CC0 (recording) | CANDIDATE for MUS_CALLIOPE | Recordist states the **third** of five songs is "Over the Waves" (Juventino Rosas, 1888: public domain composition); the other titles are unknown. The free preview stops at 396 s of 915 s, and song boundaries were found only by level analysis. Audition cuts for the owner: `asset-src/work/audio/audition/band_organ_song_a_222-265s.ogg` and `…song_b_270-396s.ogg`. To use it: confirm by ear which segment is "Over the Waves" (or download the full original with a Freesound login), then export it with `audio_prep loop` to `assets/audio/music/calliope_loop.ogg` and write the evidence file. Until then the game has no music (Asset Guide §3.6) |

---

## 3. Acquisition checklist

Work top-down. Items in bold are on the critical path for the first composited frame (M1). Every item follows the verification procedure in Asset Guide §3.2 before anything is stored.

### 3.1 Owner decisions (block sourcing)

- [ ] **Q-1:** can props be photographed by the owner, or is a tier-C budget (paid stock or commission) approved?
- [ ] Q-2: reticle as a functional non-photographic exception?
- [ ] Q-3: composited lettering on photographed boards allowed?
- [ ] Q-4: storage for large masters (Git LFS, external storage, or don't keep)?
- [ ] Q-5: is CC BY-SA acceptable for background plates?

### 3.2 Own photography session (if Q-1 = yes)

Setup: tripod, RAW, base ISO, warm key light above and in front, gray card, level camera. Record the setup in a photo.

- [ ] **Duck target(s):** obtain a physical sheet-metal or plywood gallery duck. Shoot straight-on with an 85–135 mm equivalent lens; three paint variants if available → TGT_DUCK_A/B/C
- [ ] **Rifle prop:** unbranded toy, cork, or air rifle on a swivel, shot from the shooter's viewpoint → FG_RIFLE
- [ ] **Counter:** aged wooden bench or counter, shot from 30–45 cm above its surface → FG_COUNTER
- [ ] **Painted planks** (backboard) and **blue-painted board** (wave strip) → FG_BACKBOARD, FG_WAVE_STRIP
- [ ] **Booth structure woods and striped canvas** → FG_BOOTH_FRAME
- [ ] **Incandescent bulb lit in a socket**, dark room, including its bloom → DEC_BULB_LIT, DEC_BULB_GLOW
- [ ] Painted sign blank or hand-painted sign → DEC_SIGN_HEADER, UI_SCORE_BOARD, UI_TIMER_BOARD, UI_PANEL
- [ ] Pellet dent in painted wood (macro) → OV_MISS_MARK
- [ ] Rail and chain detail → FG_TRACK_RAIL
- [ ] Sound recording in the same session: tap the duck (ping), the hinge flip, a pellet into wood, chain running → SFX_DUCK_HIT, SFX_DUCK_FLIP, SFX_MISS, SFX_CHAIN_LOOP

### 3.3 Licensed sourcing (any Q-1 answer)

- [ ] **Dusk fairground plate(s):** Wikimedia Commons (funfair and Ferris-wheel-at-night categories), Library of Congress free-to-use sets, Unsplash, Pexels. No identifiable people or ride brands → BG_FAR, BG_MID
- [ ] **Photo-scanned wood, paint, and metal textures:** Poly Haven or ambientCG, **photo-derived only** → compositing material for FG_* items
- [ ] **Scanned film grain** (CC0) → OV_GRAIN
- [ ] Smoke puff on black (CC0) → OV_MUZZLE_PUFF
- [x] **Air-rifle or cork shot recording:** Freesound with the CC0 or CC BY filter; no BY-NC → SFX_SHOT *(done 2026-11-11, D-030; BY-SA alternative EV-A01 awaits owner decision)*
- [x] Bell, tick, buzzer, wooden click: Freesound CC0 or CC BY → SFX_BELL_START, SFX_TICK, SFX_END, SFX_UI_CLICK *(done 2026-11-11)*
- [x] Fairground crowd ambience with no intelligible speech or music: Freesound CC0 or CC BY → AMB_MIDWAY *(done 2026-11-11; listening check for speech pending)*
- [ ] Band organ or calliope recording with **recording and composition both cleared** → MUS_CALLIOPE (droppable) *(candidate EV-A10: owner must confirm the song by ear)*
- [ ] Display font: verify the license of "Rye" (or an alternative) on its own page; copy `OFL.txt` → FNT_DISPLAY
- [ ] Look for a higher-resolution scan of EV-01 at University of Washington Special Collections (reference use)

### 3.4 Enhancement-tier (after MVP)

- [ ] Bunting or pennants → DEC_BUNTING
- [ ] Generic plush prizes → DEC_PRIZES
- [ ] Film dust and scratches scans → OV_DUST_SCRATCH
- [ ] Light leaks → OV_LIGHT_LEAK
- [ ] Painted "+10" tag → OV_HIT_TAG
- [ ] Physically painted pond-scene backboard → FG_BACKBOARD (v2)

---

## 4. Entry template (copy when an asset advances past `MISSING`)

```
ID:                 TGT_DUCK_A
Status:             VERIFIED | IN_PROGRESS | READY | PLACEHOLDER
Purpose:            …
Runtime path:       assets/images/targets/duck_a.png
Original path:      asset-src/originals/targets/<original filename>
Master path:        asset-src/work/targets/duck_a.xcf
Source URL:         … (or "own photography")
Creator:            … (as credited on the source page)
License:            … (name + version + license URL)
Attribution text:   … (exact credit line, or "not required")
Evidence file:      assets/licenses/TGT_DUCK_A.md
Retrieved / shot:   YYYY-MM-DD
Original SHA-256:   …
Modifications:      e.g. cutout, perspective corrected, WB to 3200 K baseline, scaled to 220 px, color bleed 6 px
Subject clearance:  no people / logos / third-party art / watermarks — checked by <name> YYYY-MM-DD
Review (Guide §6):  passed YYYY-MM-DD | notes
Notes:              …
```

## 5. Credits

Assets requiring attribution are listed here and in the in-game credits. **Currently none require it.** CC0 needs no credit, but the recordists are thanked here (and should be in any in-game credits):

- Sound recordings from Freesound (freesound.org), all CC0 1.0: FOSSarts (air rifle), JarredGibb (metal ping), RutgerMuller (metal flap), alienistcog (hinges), TRP (wooden knock, boxing bell), kyles (gear mechanism, adding machine, toy register bell), fauxpress (antique clock), Anzbot (wooden clicks), HECKFRICKER (fairground ambience).
