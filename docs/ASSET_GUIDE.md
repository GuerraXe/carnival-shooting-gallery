# Asset Guide — Carnival Shooting Gallery

| Field | Value |
|---|---|
| Status | **Images: superseded by D-044 (2026-11-13).** The owner kept the grey shapes as the final art; every image is now the project's own sprite (see the manifest). The photographic sourcing rules below no longer apply to images and are kept for the record. **Audio acquired 2026-11-11** (13 CC0 recordings, pending listening review, D-030). Font: the Windows system font Rockwell Bold, by owner decision |
| Version | 0.1 |
| Date | 2026-11-11 |
| Inputs | [PROJECT_BRIEF.md](PROJECT_BRIEF.md) §7, [ARCHITECTURE.md](ARCHITECTURE.md) §3.4, [DECISIONS.md](DECISIONS.md) D-012, D-020 to D-023 |
| Manifest | [../assets/ASSET_MANIFEST.md](../assets/ASSET_MANIFEST.md) is the single record of every asset, its source, its license, and its status |

This guide defines what assets the game needs, where they may come from, how they are processed, and how their rights are recorded. Visual quality is a top priority for the project. **A missing asset stays recorded as missing. It is never silently replaced with something unsuitable.**

---

## 1. Inventory of existing assets (2026-11-11)

Inspected: the whole project tree (then named `p/`, now `carnival-shooting-gallery/`).

| Location | Contents |
|---|---|
| `carnival-shooting-gallery/` (then `p/`) | `main.cpp` (empty), `docs/` (brief, architecture, decisions) |
| Image, audio, or font files | **None** |

**Result:** every asset in §4 starts at status `MISSING`.

### 1.1 Candidate sources already evaluated

These were found and checked during this pass. They are recorded in the manifest so nobody has to evaluate them again.

| Ref | Candidate | Verified license | Verdict | Why |
|---|---|---|---|---|
| EV-01 | *Shooting Gallery called Hunting in the Cascades*, Frank H. Nowell, 1909, University of Washington Special Collections (AYP 62), via Wikimedia Commons | Public domain (pre-1931 US publication; author died 1950), per the Commons file page | **Reference only** | The period booth architecture and "2 SHOTS FOR 5¢" signage are excellent mood reference. But the image is 409×512 and grayscale, which is far too small for game use. A higher-resolution scan may exist at UW, but this pass did not look for one |
| EV-02 | *Le stand de tir aux canards à la fête foraine.JPG*, Commons user Dinkum, 2012 | CC0 1.0, per the Commons file page | **Rejected for game use.** Reference for duck-rail layout only | The CC0 license covers the photographer's rights only. The photo itself shows **third-party copyrighted cartoon characters** (a painted mural) and **branded toy packaging**. It is also lit by cold fluorescent and LED light, shows modern plastic ducks, and was taken at an oblique angle |
| EV-03 | *Le stand de tir aux canards à la fête foraine 2.JPG*, same author | CC0 1.0 | **Rejected** | Same problems as EV-02, plus a child is in the frame |
| EV-04 | *Gun at funfair.jpg*, emina visnic (Flickr "missbenefit"), 2006 | CC BY 2.0 | **Rejected** | An identifiable person is holding the gun. 1600×1200. Wrong viewpoint for the rifle prop |
| EV-05 | *Wild West Shooting Gallery.jpg* (Commons) | Marked public domain by uploader "Pdanial", but the author field says "Zaid Hamid" | **Rejected** | The provenance is inconsistent (uploader ≠ named author), and 640×480 is too small |

**Lesson from EV-02 and EV-03:** a permissive license on a photo does **not** clear the things *in* the photo, such as murals, characters, logos, product packaging, or people. Every review must check the subject matter as well as the license (§3.4).

---

## 2. Visual consistency specification

Every asset must be compatible with one imaginary camera and one lighting setup. Assets that don't fit are rejected or re-shot. Grading cannot fully fix them.

### 2.1 Camera

| Property | Specification | Rationale |
|---|---|---|
| Viewpoint | Player's eye 30–45 cm above the counter, 3–4 m from the backboard (brief §7.6) | Matches the game's single fixed view |
| Horizon | At about 46% of frame height (just above the target lane) | Shared by every plate; drawn on the art reference sheet |
| Camera tilt | **Level.** Verticals stay vertical | Lets independently shot layers stack without keystone mismatch |
| Booth and environment plates | 35–50 mm full-frame equivalent | Natural perspective for a person standing at a booth |
| Duck targets, signage, flat props | Shot **straight-on**, with the lens axis perpendicular to the object, using a long lens (85–135 mm equivalent) from a distance | Flat cutouts must read as flat and frontal, without wide-angle distortion |
| Rifle prop | From behind and slightly above the stock, about 20–30° down, barrel pointing into the scene and toward the upper-left | This is how the shooter sees a mounted rifle (brief §6, layer 8) |
| Focus and depth of field | Foreground and lane sharp. Far background softened in the photo or in compositing (brief §7.5: depth of field is baked into the art) | — |

### 2.2 Lighting

| Property | Specification |
|---|---|
| Key light | **Above and slightly in front of the target plane.** This represents the booth's header bulbs. Shadows fall downward and slightly behind objects |
| Key color | Tungsten or incandescent, about 2700–3200 K |
| Fill | Low, warm. Shadows stay readable and never crushed to pure black (brief §7.3) |
| Background | Dusk or blue hour: cooler and darker than the booth, so the booth is the warm lit focus |
| Rejected | Direct midday sun, on-camera flash, fluorescent or LED-cold casts, strong colored gels, harsh rim light from behind |

### 2.3 Color and tone

- Masters are color-corrected to a **matched, neutral-warm baseline**. The final nostalgic grade (warmth, faded blacks, grain, vignette) comes from the runtime post-process (Architecture §5.3).
- This prevents double-grading, and it keeps a stylistic grade baked into one photo from making it look different from its neighbors.
- **Exception:** HUD and UI boards are not post-processed (D-014). They are pre-graded to match the final look.
- Working and runtime color space is **sRGB**. Masters are stored at 16 bits per channel where the source allows.
- **Ducks must have the highest value contrast in the scene** against the backboard (brief §7.3). Check this with a grayscale preview of the composited frame.

### 2.4 Scale

These real-world sizes anchor the art. Logical screen sizes are at 1920×1080.

| Object | Real size | On screen (logical px) |
|---|---|---|
| Duck target | 25–30 cm tall | ≈ 90–110 px tall (8–10% of height) |
| Incandescent bulb | ≈ 6 cm | ≈ 20–30 px (header row, nearer the camera) |
| Counter depth | ≈ 50 cm | Bottom ≈ 18% of the frame |
| Booth opening | ≈ 3.5–4 m wide | Nearly the full screen width |

---

## 3. Acquisition rules

### 3.1 Source tiers

| Tier | Sources | Conditions |
|---|---|---|
| **A: preferred** | **Own photography** by the project owner (or a named collaborator under a written grant); **CC0 1.0**; **verified public domain** | For PD: record the *basis* (e.g. "published in the US before 1931", "US federal government work"). An uploader's "PD" label alone is not enough (see EV-05) |
| **B: acceptable** | **CC BY 4.0 / 3.0 / 2.0**; **Pexels License**; **Unsplash License** (standard, not Unsplash+) | CC BY: credit, license link, and note of changes in the in-game credits and the manifest. Pexels/Unsplash: no attribution required, but their restrictions apply (§3.2). Save a dated copy of the terms in `assets/licenses/` |
| **C: owner decision required** | **CC BY-SA** (derived assets inherit share-alike); **paid stock** (iStock, Getty, Adobe Stock, etc.) under a purchased license; **commissioned photography** | Do not include without the owner's explicit approval, recorded in the manifest's notes |
| **Rejected** | NonCommercial (**NC**), NoDerivatives (**ND**, because we cut out and grade every asset), "free for personal use", no license stated, "found on image search", editorial-only stock, anything watermarked, Unsplash+ without a purchased license, anything AI-generated or AI-extended | Never included, even temporarily |

Summary of the terms verified on 2026-11-11 (re-check at acquisition time, because terms change):

| Platform | Terms as stated on its license page |
|---|---|
| **Poly Haven** | "Our assets are all licensed as CC0"; no credit required. Site content *other than* the assets (renders, logos, text) is not CC0, and scraping is prohibited |
| **ambientCG** | All assets under CC0 1.0; credit appreciated but not required. The license page does **not** say how assets were made. Confirm per asset that it is photo-derived (§3.3) |
| **Pexels** | Free use and modification; no attribution needed. Do not sell unaltered copies; do not redistribute on stock or wallpaper platforms; do not imply endorsement; do not use in trademarks; identifiable people must not be shown in a bad light |
| **Unsplash** | Free commercial and non-commercial use; no attribution needed. Cannot be sold without significant modification; cannot be compiled into a competing service. Unsplash+ has separate terms |
| **Freesound** | License is **per sound**: CC0, CC BY, or CC BY-NC. BY-NC is rejected by our rules. The retired "Sampling+" license is treated as rejected unless reviewed |
| **Library of Congress** | Its "Free to Use" sets are described as rights-free, and most come from the Prints & Photographs Division. Record each **item's** own rights statement anyway |
| **Wikimedia Commons** | License is per file. Read the file page's license block **and** check that author, source, and uploader agree |

### 3.2 Verification procedure (every asset, before it enters `asset-src/`)

1. Open the asset's **own** page, not a search result or an aggregator thumbnail.
2. Record the exact source URL, the creator as credited, the license name and version, and the license URL.
3. Save evidence to `assets/licenses/<ASSET_ID>.md`:
   - the retrieval date
   - the verbatim license statement from the page
   - an archived snapshot URL (web.archive.org), when one can be made
4. Check provenance consistency: author vs. uploader, plausible camera and EXIF metadata, and a **reverse-image search** to make sure the "free" copy is not a re-upload of a commercial stock image.
5. Check the subject matter (§3.4).
6. Check for AI-generated content (§3.3).
7. Add or update the row in [`ASSET_MANIFEST.md`](../assets/ASSET_MANIFEST.md). Any failure → status `REJECTED`, with the reason.

**If any field cannot be verified, the asset is not used.** Fields are never filled in by guesswork; write "unknown" and reject.

### 3.3 Authenticity: photographic, not generated

Required: photographs of real physical objects and places.

Prohibited:

- Generic digital illustration or vector art standing in for objects or scenery
- Procedural textures (noise-generated wood, metal, grain, and so on) substituted for photographs
- AI-generated images, **and** AI generative editing: generative fill, outpainting or "expand", AI upscaling that invents detail, AI relighting

Allowed editing:

- Selection and masking tools, including automatic ones, **as long as every edge is manually inspected and cleaned**
- Cloning or healing from the **same** photograph
- Perspective correction, color and tone correction, scaling, sharpening, denoising
- Compositing several verified photographs together
- Painted lettering composited onto a photographed surface (signage), **only** with owner approval, recorded per asset. Lettering physically painted and then photographed is preferred

**Screening:** reject when an asset:

- is tagged as AI-generated by its platform
- has a description mentioning AI tools
- has an uploader with a gallery of obviously generated work
- shows tell-tale artifacts: malformed text, impossible geometry, smeared details, or inconsistent lighting within one object

**Functional exceptions:** non-photographic elements are allowed only where they are functional rather than environmental, and each one is listed in the manifest as an exception:

- the aiming reticle (pending owner confirmation, see §7)
- the vignette, which is an optical falloff computed in the shader, not an image

### 3.4 Subject-matter clearance

The following must not appear in any asset that ships in the game, regardless of the photo's license:

- **Identifiable people.** Faces, distinctive tattoos, and children are rejected. Distant, unidentifiable crowd silhouettes in background plates are acceptable.
- **Third-party artwork or characters**, e.g. the mural in EV-02.
- **Logos, brand names, trademarks, or product packaging.** This includes firearm and air-gun maker marks on the rifle.
- **Names of real carnivals or companies** on signage. Signage text is the project's own (brief A-15).
- **Watermarks, stamps, or photographer signatures.** Never removed. The asset is rejected instead.
- **Private property under restricted photography terms.** For own photography at a fair or museum, check that photography is permitted. Get a property release if a specific identifiable private booth is the main subject.

### 3.5 Own photography: the expected primary path for hero assets

The 2026-11-11 search found **no** freely licensed photo that delivers a vintage American booth, ducks, or a rifle prop at usable resolution and the specified viewpoint (§1.1). Hero assets therefore most likely need to be **photographed for this project**:

- **Duck targets:** buy or borrow physical sheet-metal or plywood shooting-gallery ducks (vintage or reproduction). Shoot them straight-on against a neutral background, under one warm key light from above-front.
- **Rifle prop:** a toy, cork, or air rifle with no visible branding, mounted on a simple swivel. Shoot it from the shooter's viewpoint (§2.1).
- **Wood and paint surfaces:** aged boards, a weathered bench or counter, painted planks.
- **Bulbs:** real incandescent bulbs in sockets, photographed lit, in a dark room.

Shooting specification:

- RAW capture.
- Base ISO, on a tripod.
- A gray card frame in each setup, for white balance.
- Lights at a fixed position, recorded in a setup photo.

Record photographer, date, camera, and setup in the manifest. The owner holds the rights (tier A).

### 3.6 Note on music rights

A recording has its own copyright, separate from the composition it plays. An old public-domain march is not usable if the *recording* is protected. Acceptable options:

- A CC0 or CC BY **recording** of a PD composition, with both facts verified
- A recording whose own public-domain status is verified for the US, with the basis recorded

When in doubt, use crowd ambience only and leave out music. Music is the lowest-priority audio asset.

---

## 4. Required assets

**Priority key:**
- **MVP:** needed for the first playable game (brief §9.1).
- **E1 / E2:** enhancement tiers (brief §9.2 / §9.3).

**Runtime formats:**
- JPEG (quality 90) for opaque images.
- PNG-32 with straight alpha for anything with transparency.

Runtime pixel sizes are at 1× logical resolution unless noted. The full manifest, with status per asset, is [`ASSET_MANIFEST.md`](../assets/ASSET_MANIFEST.md).

### 4.1 Images

| ID | Category (request #) | Runtime file | Spec | Prio | Recommended source path |
|---|---|---|---|---|---|
| `BG_FAR` | Booth environment (1) | `images/backgrounds/bg_far_dusk.jpg` | 1960×1100 (2% parallax margin); dusk sky, Ferris wheel or ride silhouettes, distant lights; soft focus | MVP | CC0, PD, or tier-B dusk photo of a fairground; no identifiable people, no ride brand names |
| `BG_MID` | Booth environment (1) | `images/backgrounds/bg_mid_midway.png` | 1960×1100 with alpha above the skyline; tents, string lights, crowd silhouettes | MVP* | Same as above, or cut from the same photo as `BG_FAR`. *MVP may combine BG_FAR and BG_MID in one plate |
| `FG_BOOTH_FRAME` | Booth environment (1) | `images/foreground/booth_frame.png` | 1920×1080; posts, header beam, awning edge; transparent opening | MVP | Composite of verified wood, paint, and striped-canvas photos; own photography preferred |
| `FG_BACKBOARD` | Track and scenery (5) | `images/foreground/backboard.jpg` | 1920×≈500; weathered painted planks behind the lane | MVP | CC0 photo-scanned painted-plank texture (Poly Haven or ambientCG, photo-derived only) or own photography. *A painted pond scene is E1 and must be a physically painted panel the project has rights to* |
| `FG_TRACK_RAIL` | Track and scenery (5) | `images/foreground/track_rail.png` | Horizontally tileable strip, ≈ 512×64; metal rail with chain links | MVP | CC0 metal or chain photo, or own photography |
| `FG_WAVE_STRIP` | Track and scenery (5) | `images/foreground/wave_strip.png` | 1920×≈140; blue-painted wooden wave cutout hiding duck bases | MVP | Wave silhouette cut from a verified photo of blue-painted wood. **Modification recorded** |
| `FG_COUNTER` | Counter (2) | `images/foreground/counter.png` | 1920×≈220; worn wooden counter top edge and front, seen from the shooter's position | MVP | Own photo of a real aged counter or bench at the correct angle (preferred), or a CC0 plank scan put into perspective |
| `FG_RIFLE` | Rifle (6) | `images/foreground/rifle.png` | ≈ 900×500, **shot from behind, looking along the barrel** (D-039), swivel and muzzle points recorded in the manifest; the silhouette must lie within ±70 px of the swivel→muzzle line (scaled to the 900×500 frame); unbranded mounted carnival rifle | MVP | **Own photography** (§3.5). No suitable free photo was found |
| `TGT_DUCK_A`, `TGT_DUCK_B`, `TGT_DUCK_C` | Ducks (4) | `images/targets/duck_a.png` … | Runtime ≈ 220 px tall (2× on-screen size, for crisp scaling and accurate masks); master ≥ 1500 px tall; straight-on; hinge bracket visible; clean alpha edge with no halo | MVP (A only); B and C are E1 | **Own photography** of physical targets (§3.5). Alpha also defines the hit mask (D-009), so edge quality is gameplay-critical |
| `DEC_SIGN_HEADER` | Signage (3) | `images/decorations/sign_header.png` | ≈ 1400×260; hand-lettered placeholder title (brief A-15) | MVP | A physically painted sign photographed (preferred), or a photographed blank sign board with composited painted lettering (needs owner approval, §3.3) |
| `DEC_BULB_LIT` | Lights (7) | `images/decorations/bulb_lit.png` | ≈ 96×128; lit incandescent bulb in its socket; glow in a separate layer | MVP | Own photography in a dark room, or a CC0 photo |
| `DEC_BULB_GLOW` | Lights (7) | `images/decorations/bulb_glow.png` | ≈ 256×256 on black, for additive blending | MVP | Taken from the same bulb photograph (real lens bloom); no synthetic gradient |
| `DEC_BUNTING` | Decorations (3) | `images/decorations/bunting.png` | 1920×≈160; striped or pennant bunting | E1 | CC0 or tier-B photo of fabric bunting, or own photography |
| `DEC_PRIZES` | Decorations (7) | `images/decorations/prizes_*.png` | Hanging plush prizes at booth edges | E1 | Own photography of **generic, unbranded** plush toys only (§3.4) |
| `UI_SCORE_BOARD` | Signage / UI (3) | `images/ui/score_board.png` | ≈ 420×180; painted wooden score board; pre-graded (D-014) | MVP | Own photo or verified CC0 photo of a painted board; numbers drawn at runtime with the display font |
| `UI_TIMER_BOARD` | UI (3) | `images/ui/timer_board.png` | ≈ 300×180; tin plate or small wooden sign; pre-graded | MVP | Same as above |
| `UI_PANEL` | UI (3) | `images/ui/panel_board.png` | ≈ 1000×700; large painted board or ticket for title, pause, and results | MVP | Same as above |
| `UI_RETICLE` | UI | `images/ui/reticle.png` | ≈ 64×64; **functional exception** (§3.3) | MVP | Pending owner decision (§7, Q-2) |
| `OV_GRAIN` | Overlays (8) | `images/overlays/film_grain.png` | Tileable ≈ 512×512 grayscale; **scanned real film grain** | MVP | CC0 film-grain scan, or own scan of exposed, developed film stock. Replaces procedural noise (D-022) |
| `OV_MISS_MARK` | Overlays (8) | `images/overlays/miss_mark.png` | ≈ 48×48; pellet dent or splinter in painted wood | MVP | Own macro photo or CC0 photo |
| `OV_MUZZLE_PUFF` | Overlays (8) | `images/overlays/muzzle_puff.png` | ≈ 256×256 on black; small smoke or cork puff, additive | MVP | CC0 or own photo of a real smoke puff |
| `OV_DUST_SCRATCH` | Overlays (8) | `images/overlays/dust_scratch_*.png` | Scanned film dust and scratches | E1 | CC0 scans |
| `OV_LIGHT_LEAK` | Overlays (8) | `images/overlays/light_leak_*.jpg` | Photographed real light leaks | E1 | CC0 or own photography |
| `OV_HIT_TAG` | Overlays / UI | `images/ui/hit_tag.png` | Painted wooden "+10" tag | E1 | MVP draws "+10" text with the display font |
| `OV_MUZZLE_FLASH` | Overlays (8) | `images/overlays/muzzle_flash.png` | ≈ 256×256 on black; a real muzzle flash, additive, a few frames (D-031) | E1 | Own photo or CC0. Optional: without it the flash is an exposure lift only |

### 4.2 Audio

All audio must be **recordings of real sounds**, not synthesized. Each sound is checked for its per-sound license (Freesound: CC0 or CC BY only).

| ID | Runtime file | Spec | Prio | Recommended source |
|---|---|---|---|---|
| `SFX_SHOT` | `audio/effects/shot.wav` | Air-rifle or cork pop (brief A-13), ≤ 0.6 s, leading silence trimmed (R-9) | MVP | CC0 Freesound recording, or own recording of an air or cork gun |
| `SFX_DUCK_HIT` | `audio/effects/duck_hit.wav` | Metallic ping or clack | MVP | Own recording (tap a sheet-metal target), or CC0 |
| `SFX_DUCK_FLIP` | `audio/effects/duck_flip.wav` | Hinge clatter | MVP | Same as above |
| `SFX_MISS` | `audio/effects/miss_thud.wav` | Pellet into wood | MVP | Own recording or CC0 |
| `SFX_CHAIN_LOOP` | `audio/effects/chain_loop.wav` | Seamless mechanical chain or track loop, ≈ 4–8 s | MVP | Own recording (bicycle chain, gear motor) or CC0 |
| `SFX_BELL_START` | `audio/effects/bell_start.wav` | Hand bell or ding | MVP | CC0 |
| `SFX_TICK` | `audio/effects/tick.wav` | Mechanical clock tick | MVP | CC0 |
| `SFX_END` | `audio/effects/end_bell.wav` | Buzzer or bell | MVP | CC0 |
| `SFX_UI_CLICK` | `audio/effects/ui_click.wav` | Wooden click | MVP | CC0 |
| `SFX_DUCK_RESET` | `audio/effects/duck_reset.wav` | Hinge rattle as the target swings back up | E1 | Own recording or CC0 |
| `SFX_SCORE_TICK` | `audio/effects/score_tick.wav` | Mechanical counter clack (score board) after a hit | E1 | CC0 |
| `SFX_RESULTS` | `audio/effects/results_ding.wav` | Register bell when the results are rung up | E1 | CC0 |
| `AMB_MIDWAY` | `audio/ambience/midway_loop.ogg` | Distant crowd and fairground, ≥ 60 s seamless loop; no intelligible speech, no recognizable music | MVP | CC0 field recording |
| `MUS_CALLIOPE` | `audio/music/calliope_loop.ogg` | Distant band organ or calliope, low in the mix | E1 (optional, D-030) | Needs **both** a public-domain composition **and** a cleared recording (§3.6) |

### 4.3 Fonts

| ID | Runtime file | Spec | Prio | Source |
|---|---|---|---|---|
| `FNT_DISPLAY` | `fonts/display.ttf` | Showcard or slab-serif carnival lettering, used for HUD numerals, results, and buttons | MVP | SIL Open Font License (OFL) fonts only. **Candidate:** "Rye" on Google Fonts. *Its license was not confirmed in this pass (the page did not load)*. Verify before use, and copy the font's `OFL.txt` into `assets/licenses/` |

Fonts are the only non-photographic visual asset that is **required**. Lettering is functional, and it is styled to match painted signage.

### 4.4 Shaders (not an art asset; listed for completeness)

The architecture puts GLSL files in `assets/shaders/`. They are project source code, not acquired assets, and are not tracked in the manifest.

---

## 5. Directory structure and processing pipeline

### 5.1 Layout

```
<project folder>/
├── asset-src/                  ← masters and originals. NOT shipped, NOT loaded by the game
│   ├── originals/<category>/   untouched downloads or camera RAW files, byte-for-byte as obtained
│   ├── work/<category>/        layered edit files (.xcf / .kra / .psd / .tif), 16-bit
│   └── reference/              mood and reference images (e.g. EV-01). Never exported to runtime
└── assets/                     ← runtime. Copied next to the exe by the build (Architecture §3.4)
    ├── ASSET_MANIFEST.md       single source of truth for every asset
    ├── images/
    │   ├── backgrounds/
    │   ├── foreground/         counter, booth frame, backboard, track, wave strip, rifle
    │   ├── targets/
    │   ├── decorations/        signage, bulbs, bunting, prizes
    │   ├── ui/
    │   └── overlays/           film grain, miss marks, muzzle puff, dust and scratches, light leaks
    ├── audio/
    │   ├── effects/
    │   ├── ambience/
    │   └── music/
    ├── fonts/
    ├── licenses/               one evidence file per asset (§3.2), plus full license texts (CC0, CC BY x.y, OFL)
    └── shaders/                project source (Architecture §5.3)
```

`<category>` in `asset-src/` matches the runtime subfolder names.

**Repository note:** `asset-src/originals` and `asset-src/work` may grow large (RAW and layered TIFF files). When the project's git repository is created, track them with Git LFS or keep them outside git with a documented backup. That decision is deferred to repository setup (Q-4).

### 5.2 Pipeline stages

| Stage | Action | Manifest status after |
|---|---|---|
| 1. Identify | Need listed in §4 and the manifest | `MISSING` |
| 2. Find | Candidate found; source URL recorded | `CANDIDATE` |
| 3. Verify | §3.2 procedure; evidence file written to `assets/licenses/` | `VERIFIED` or `REJECTED` |
| 4. Preserve | Original saved **unchanged** to `asset-src/originals/`, with its SHA-256 recorded in the manifest | `VERIFIED` |
| 5. Edit | Master in `asset-src/work/`: correction, cutout, compositing. Every modification listed in the manifest | `IN_PROGRESS` |
| 6. Export | Runtime file written to `assets/…` at the §4 spec | `IN_PROGRESS` |
| 7. Review | §6 checklist in a composited test frame | `READY` or back to stage 5 |
| — | Temporary stand-in (§5.4) | `PLACEHOLDER` |
| — | Kept for reference, never shipped | `REFERENCE_ONLY` |

### 5.3 Export rules

- **Opaque plates:** JPEG, quality 90, sRGB, 4:4:4 chroma for plates containing signage text.
- **Cutouts:** PNG-32 with straight (non-premultiplied) alpha. Color is bled 4–8 px past the alpha edge so filtering never shows a dark or light halo.
- **Duck alpha:** the edge is antialiased but tight. Hit testing thresholds alpha at 128 (D-009), so semi-transparent fringe must not extend more than 1–2 px beyond the visible silhouette.
- **Metadata:** EXIF and GPS are stripped from runtime files. Originals stay untouched.
- **Audio:** WAV 16-bit 44.1/48 kHz mono for SFX; OGG Vorbis q5 stereo for loops; loops tested for seamlessness.
- **Size budget:** the total runtime image set is about 40 MB on disk and fits comfortably in VRAM at 1× (6 full-screen layers × 8 MB of RGBA). 4K-native plates are an E1 option.
- **Naming:** `lower_snake_case`, matching the paths in §4 and the compiled-in asset table (D-012). Placeholders are prefixed with `TEMP_` (§5.4).

### 5.4 Placeholder policy

Placeholders are allowed **only** to unblock layout and code work, and only when they are impossible to mistake for final art.

1. The file name starts with **`TEMP_`** (e.g. `images/targets/TEMP_duck_a.png`).
2. The manifest status is **`PLACEHOLDER`**, with the date and the reason.
3. The image carries a **visible "TEMP" stamp** in a corner.
4. Acceptable kinds of placeholder:
   - **Preferred:** the owner's own quick phone photo of a stand-in physical object, e.g. a rubber duck or a plank. This is photographic and the rights are clear.
   - **Allowed:** a flat mid-gray card of the correct size and silhouette, used only for layout blocking.
   - **Never:** an unlicensed image, a clip-art duck, a generated image, or a "close enough" image from search.
5. **Release gate:** the asset contract test (Architecture §7) fails a **Release** build if any manifest entry is `PLACEHOLDER`, `CANDIDATE`, or `MISSING` for an MVP-priority asset, or if any file under `assets/` has the `TEMP_` prefix.

**None exist yet. No placeholders have been created.**

---

## 6. Review checklist (stage 7)

An asset reaches `READY` only when every item passes, judged in a composited test frame with the post-process both on and off:

- [ ] License evidence file present; manifest row complete (source, creator, license, modifications, SHA-256 of the original)
- [ ] Subject matter clean: no people, logos, characters, watermarks, or real business names (§3.4)
- [ ] Photographic and not generated; edits within the §3.3 allowed list
- [ ] Perspective matches the camera spec (§2.1): horizon, level verticals, straight-on for flat props
- [ ] Light direction matches (§2.2): highlights on top and front, shadows falling down and back
- [ ] Color temperature and contrast match neighboring layers (§2.3); no cast left uncorrected
- [ ] Scale matches the real-world table (§2.4)
- [ ] Sharpness and noise consistent with neighbors; no upscaling artifacts
- [ ] Clean alpha edge with no halo on dark or light backgrounds; duck mask matches the visible silhouette (F3 overlay)
- [ ] Runtime file meets the §4 spec (size, format, naming)
- [ ] Ducks remain the highest-contrast element in a grayscale check of the full frame

---

## 7. Open questions for the owner

| # | Question | Why it matters |
|---|---|---|
| Q-1 | **Can you photograph physical props yourself** (duck targets, an unbranded toy or cork rifle, an aged wooden counter, incandescent bulbs)? If not, is a budget for paid stock or a commissioned photographer acceptable (tier C)? | No free photo was found for the hero assets (§1.1). This is the critical path for the whole visual pillar (brief A-3, risk R-1) |
| Q-2 | **May the aiming reticle be a clean, non-photographic graphic** (functional exception)? The alternative is a photographed peep sight or front post, which costs legibility | The rules forbid digital illustration; the reticle is a functional overlay where readability matters most (AV-4) |
| Q-3 | **May painted lettering be composited onto photographed sign boards**, or must signs be physically painted and then photographed? | Affects `DEC_SIGN_HEADER` and the UI boards |
| Q-4 | **How should large masters be stored?** Git LFS, outside git with a backup, or not kept at all after export? | `asset-src/` will hold RAW and layered files of several GB |
| Q-5 | Is **CC BY-SA** acceptable for background plates? | Many good fairground photos on Commons are BY-SA; share-alike would apply to the derived plate files |

---

## 8. Assumptions to revisit

| # | Assumption | Revisit when |
|---|---|---|
| AA-1 | A believable booth can be **composited** from separately photographed elements, rather than found as one photo | First composited mock frame (M1) |
| AA-2 | 1× logical runtime resolution (ducks at 2×) looks sharp enough at 1440p and 4K | Visual review at M1 on a high-DPI display |
| AA-3 | A scanned film-grain plate (rather than shader noise) gives authentic grain at acceptable cost | M5 performance pass |
| AA-4 | Physical shooting-gallery ducks (vintage or reproduction) can be obtained to photograph | Answer to Q-1 |
| AA-5 | Platform license terms checked on 2026-11-11 still apply at acquisition time | Every acquisition (§3.2 re-checks them) |
