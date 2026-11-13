# Carnival Shooting Gallery

A first-person, single-player shooting gallery set at an old-fashioned carnival. You stand behind a wooden booth counter and shoot at mechanical duck targets moving along a track. A round lasts 60 seconds: each hit scores 10, and the results board shows your score, accuracy and best score for the session.

**Version 0.1.0-preview.** This is a preview. The game is fully playable, but none of the photographs it is designed around exist yet, so every image (booth, ducks, rifle, signs, boards) is drawn as a plain grey placeholder shape. The sound effects and ambience are real recordings. See [Known limitations](#known-limitations).

Written in C++20 with [SFML 3.0.2](https://www.sfml-dev.org/).

> ### ▶ Where is the game? Which file do I open?
>
> The file to open is **`CarnivalShootingGallery.exe`**. It is **not** in this repository's source code; you get it one of two ways:
>
> | You… | The exe is at | How |
> |---|---|---|
> | **just want to play** | `CarnivalShootingGallery-0.1.0-preview-win64\CarnivalShootingGallery.exe`, inside the zip | Download the zip from the [**Releases** page](https://github.com/GuerraXe/carnival-shooting-gallery/releases/latest), extract it, double-click the exe ([Option A](#option-a-play-from-the-zip-no-build-tools-needed)) |
> | **built it yourself** | **`game\CarnivalShootingGallery.exe`** in the project folder | `cmake --workflow --preset preview` creates the `game\` folder ([Option B](#option-b-run-your-own-build-from-the-game-folder)) |
>
> Keep the `assets` folder next to the exe; the game needs it.

## Installing and running

**Requirements:** 64-bit Windows 10 (version 1607 or later) or Windows 11, a mouse, and a graphics card or chip with OpenGL support (any PC from the last decade). Nothing else to install: no Visual C++ Redistributable, no SFML, no runtime. Only tested on Windows 11 (see [Known limitations](#known-limitations)).

### Option A: play from the zip (no build tools needed)

The game ships as `CarnivalShootingGallery-0.1.0-preview-win64.zip`. Download it from this repository's **Releases** page. If you built it yourself, it is in the project's `dist\` folder (see [Building from source](#building-from-source)).

1. Right-click the zip → **Extract All…** and pick a folder you can write to, such as your Desktop or Documents. Avoid `C:\Program Files`, because the game writes its log next to itself (it falls back to `%LOCALAPPDATA%` if it can't).
2. Open the extracted folder `CarnivalShootingGallery-0.1.0-preview-win64`. It contains:
   ```
   CarnivalShootingGallery.exe   the game
   assets\                       sounds, shader, licence evidence (must stay next to the exe)
   licenses\                     third-party licence texts
   README.md, THIRD_PARTY_NOTICES.md
   ```
3. Double-click **`CarnivalShootingGallery.exe`**.
   - If Windows shows **"Windows protected your PC"** (SmartScreen does this for unsigned programs that came from another computer or the internet), click **More info → Run anyway**.
4. Optional: right-click the exe → **Send to → Desktop (create shortcut)**. The game finds its `assets` folder relative to the exe, so shortcuts and any working folder are fine.

To **uninstall**, delete the extracted folder. The game writes nothing else, apart from the `%LOCALAPPDATA%\CarnivalShootingGallery\carnival.log` fallback described above.

### Option B: run your own build from the `game` folder

After [building from source](#building-from-source) with `cmake --workflow --preset preview`, the project folder has a ready-to-play **`game\`** folder, laid out exactly like the zip:

```
carnival-shooting-gallery\
└── game\
    ├── CarnivalShootingGallery.exe   ← double-click this to play
    ├── assets\
    ├── licenses\
    └── README.md, THIRD_PARTY_NOTICES.md
```

Open `game\` in Explorer and double-click **`CarnivalShootingGallery.exe`**, or from PowerShell in the project folder run `.\game\CarnivalShootingGallery.exe`. You can move or copy the whole `game` folder anywhere you like. Every preview build refreshes it.

### Other exe files (for developers)

The `build\` folder also contains files named `CarnivalShootingGallery.exe`. These are intermediate build outputs. **To play, use `game\CarnivalShootingGallery.exe` instead.**

| Path (relative to the project folder) | What it is |
|---|---|
| **`game\CarnivalShootingGallery.exe`** | **The game. Open this one.** |
| `dist\CarnivalShootingGallery-0.1.0-preview-win64.zip` | The same as `game\`, zipped, for sharing or for another PC (Option A) |
| `build\windows-msvc-preview\Release\…exe` | The preview build output that `game\` is copied from |
| `build\windows-msvc\Debug\…exe` | Development build: a console window with live logs opens beside the game |
| `build\windows-msvc\Release\…exe` | Strict release build. It **refuses to start** until every required photograph exists (decision D-023) |

Each exe needs the **`assets` folder that sits next to it**. If you copy an exe somewhere else on its own, the game stops at startup with "asset folder was not found". Copy the whole folder instead. The `game\`, `build\` and `dist\` folders are generated: they don't exist until you build, and they aren't stored in the repository.

### Playing

1. The title board appears. Click **Start Round**, or press Enter.
2. "Ready…" then "Go!". You have **60 seconds**. Aim with the mouse and click to shoot.
3. When the time is up, the results board shows your score, hits, misses, decoys hit, accuracy and session best. Click **Play Again** (or press Enter / R), or press **Esc** to quit.

Press **Esc** at any time during a round to pause. From the pause board, Esc again returns to the title, and Esc there quits. Closing the window quits from any screen.

### If something goes wrong

| Problem | What to do |
|---|---|
| "The game's asset folder was not found" | The `assets` folder must sit next to `CarnivalShootingGallery.exe`. Re-extract the whole zip instead of copying only the exe |
| Black window, or an OpenGL error at startup | Update the graphics driver. The game needs basic OpenGL support |
| No sound | Check the Windows output device and volume, and press **M** in case the game is muted. Without an audio device the game runs silently on purpose |
| The mouse pointer disappears | That is intentional during a round: the game draws its own sight. Press Esc to get the pointer back |
| Anything else | Read `carnival.log` next to the exe (or in `%LOCALAPPDATA%\CarnivalShootingGallery\`). It records startup, every shot and any error |

## Controls

| Input | Effect |
|---|---|
| Mouse | Aim during a round (the pointer is hidden and kept inside the window). On the menu boards, point at a placard |
| Left click | Shoot during a round (after "Go!"; one shot per 250 ms). On a board, press the placard under the pointer |
| Right click | Reload during a round (1.2 s; the rifle holds 6 rounds). Clicking on an empty rifle only dry-fires |
| Enter / Space | Start a round (title), resume (paused), play again (results) |
| P | Pause / resume |
| R | Reload (in a round), restart the round (paused) or play again (results) |
| Esc | Title: quit. In a round: pause. Paused: back to the main menu. Results: quit |
| M | Sound on/off |
| − / = (also numpad − / +) | Volume down / up (10 % steps, default 80 %) |
| N | Background music on/off (no music track is installed yet) |
| F4 | Post-process (colour grade and vignette) on/off |
| F3, 1–8, K | Development builds only (Debug): inspection overlay, layer toggles and a debug knock-down. Ignored in this preview |

### Targets and ammo

- **Ducks** score 10. They are quick, and they **duck down on their own** if you don't hit them in time: about 1.8 to 4.2 seconds, a different time for each duck, getting shorter as the round goes on (never under 1.3 s). Two targets never leave at the same moment. An escaped duck scores nothing and costs nothing; it comes back up later.
- **Red decoys** cost 20 points. They linger longer, about 2.4 to 5.6 seconds, then leave on their own. The score can go below zero. They count as shots but not as hits, so they lower your accuracy.
- **Gold ammo ducks** turn up about 1 in 20 times a target comes back up, and **at least once every 15 seconds or so** (guaranteed), and they don't stay long: about 1.3 to 3 seconds. They score 10 and give **5 seconds of free ammo**: shots don't use rounds, and no reload is needed. After that, you have a full magazine.
- The rifle holds **6 rounds**. Reload with R or right click; it takes 1.2 s, and you can't fire meanwhile. The ammo board on the counter shows what's left.

Switching to another window pauses a running round. Closing the window quits from any screen.

## Building from source

### Dependencies

| Tool | Version tested | Notes |
|---|---|---|
| Windows | 11, x64 | The only platform that has been built and run |
| Visual Studio 2022 Build Tools | MSVC 14.44 | "Desktop development with C++" workload |
| CMake | 3.31.6 (bundled with VS); 3.26 minimum | The VS-bundled copy is not on `PATH`; see below |
| Network | First configure only | Downloads SFML and its dependencies |

You don't install SFML yourself. CMake downloads SFML 3.0.2 as a fixed commit archive checked by SHA-256, and SFML pins its own dependencies (FreeType 2.13.2, libogg 1.3.5, libvorbis 1.3.7, libFLAC 1.4.3) to release tags. Everything is linked statically. If the download is blocked, see `docs/BUILD.md`, "If the download is blocked".

### Configure and build

1. Install **Visual Studio 2022 Build Tools** (free: https://visualstudio.microsoft.com/downloads/ → "Tools for Visual Studio" → Build Tools) with the **"Desktop development with C++"** workload. That brings MSVC and CMake. Git is only needed to clone.
2. Get the source, either `git clone https://github.com/GuerraXe/carnival-shooting-gallery.git` or by downloading and extracting the repository ZIP.
3. In PowerShell, from the project folder (replace `path\to`; the folder may have any name and live anywhere):

```powershell
cd "path\to\carnival-shooting-gallery"

# CMake bundled with the Build Tools, at their default install location
# (adjust if you installed elsewhere, or just use "cmake"/"ctest" if they are on PATH)
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$ctest = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe"

# Distribution: configure, build, test and zip in one step
& $cmake --workflow --preset preview
#   -> game\CarnivalShootingGallery.exe                      (ready to play: double-click it)
#   -> dist\CarnivalShootingGallery-0.1.0-preview-win64.zip   (the same, zipped)

# Development tree (Debug with console logs, strict Release)
& $cmake --preset windows-msvc
& $cmake --build --preset debug
& $ctest --preset debug
```

| Preset | Build tree | What it is |
|---|---|---|
| `windows-msvc` + `debug` | `build/windows-msvc/Debug/` | Development: console window with live logs, placeholders always allowed, CRT leak report |
| `windows-msvc` + `release` | `build/windows-msvc/Release/` | Strict release: **refuses to start** until every required asset is finished (decision D-023). Its `ctest --preset release` asset check fails for the same reason |
| `windows-msvc-preview` + `preview` | `build/windows-msvc-preview/Release/` | The distribution: optimized, placeholders allowed, "(Preview)" title, static C++ runtime |

The first build compiles SFML and its dependencies (a few minutes per tree). Every build copies `assets/` next to the exe, so each build folder can be launched directly.

### Launching a build

Double-click `game\CarnivalShootingGallery.exe` (see [Option B](#option-b-run-your-own-build-from-the-game-folder)). The other exe files under `build\` are listed in [Other exe files](#other-exe-files-for-developers).

Command-line switches for automated tests (`--frames`, `--screenshot`, `--no-dialog`, `--mute`, `--stand-ins`, …) are listed in `docs/BUILD.md`. None are needed to play.

### Logs and exit codes

Each run writes `carnival.log` next to the exe, overwriting the previous one. If that folder is read-only (for example, under Program Files), the log goes to `%LOCALAPPDATA%\CarnivalShootingGallery\carnival.log` instead. Exit code 0 is a normal quit. Exit code 1 is a startup failure or an unexpected error; the game shows a message box and writes the details to the log.

## Project structure

```
.
├── CMakeLists.txt, CMakePresets.json   build definition and presets
├── cmake/ThirdPartyNotices.cmake       copies dependency license texts into the build
├── include/gallery/                    public headers of the SFML-free game logic library
├── src/
│   ├── logic/                          game rules: targets, shooting, round clock, layout, soundscape
│   ├── shell/                          SFML side: app loop, assets, audio, rendering, Windows platform code
│   └── main.cpp                        entry point; turns failures into a logged error and exit code 1
├── tests/                              unit tests (logic library) and the asset-contract check
├── assets/                             runtime assets shipped with the game, plus ASSET_MANIFEST.md and licenses/
├── asset-src/                          originals and work files behind assets/ (not shipped)
├── tools/audio_prep.cpp                development tool that cuts the runtime sounds from the originals
├── docs/                               brief, architecture, decisions, asset guide, build guide, verification report
├── README.md, THIRD_PARTY_NOTICES.md
├── game/                               generated: the playable game (open game/CarnivalShootingGallery.exe)
└── build/, dist/                       generated; not part of the source
```

## Asset licensing and attribution

**Images and fonts:** none are included yet. Every image in the game is a placeholder drawn by the code. The display lettering temporarily uses a font already installed with Windows (see below), and that font is not distributed with the game.

**Sounds:** all 13 sounds are real recordings from [Freesound](https://freesound.org), released by their creators under **CC0 1.0** (public domain dedication; attribution not required). They were trimmed, faded, converted to mono where noted, and normalized. Credit:

| Sound | File | Creator | Source |
|---|---|---|---|
| Air-rifle shot | `audio/effects/shot.wav` | FOSSarts | https://freesound.org/people/FOSSarts/sounds/750571/ |
| Metallic ping (hit) | `audio/effects/duck_hit.wav` | JarredGibb | https://freesound.org/people/JarredGibb/sounds/217459/ |
| Target slamming flat | `audio/effects/duck_flip.wav` | RutgerMuller | https://freesound.org/people/RutgerMuller/sounds/50736/ |
| Hinge rattle (reset) | `audio/effects/duck_reset.wav` | alienistcog | https://freesound.org/people/alienistcog/sounds/124710/ |
| Pellet into wood (miss) | `audio/effects/miss_thud.wav` | TRP | https://freesound.org/people/TRP/sounds/573835/ |
| Start bell | `audio/effects/bell_start.wav` | TRP | https://freesound.org/people/TRP/sounds/571096/ |
| End bell | `audio/effects/end_bell.wav` | TRP | https://freesound.org/people/TRP/sounds/571096/ |
| Track mechanism loop | `audio/effects/chain_loop.wav` | kyles | https://freesound.org/people/kyles/sounds/452238/ |
| Score counter clack | `audio/effects/score_tick.wav` | kyles | https://freesound.org/people/kyles/sounds/637735/ |
| Register bell (results) | `audio/effects/results_ding.wav` | kyles | https://freesound.org/people/kyles/sounds/453763/ |
| Final-seconds tick | `audio/effects/tick.wav` | fauxpress | https://freesound.org/people/fauxpress/sounds/42139/ |
| Wooden UI click | `audio/effects/ui_click.wav` | Anzbot | https://freesound.org/people/Anzbot/sounds/565027/ |
| Fairground ambience | `audio/ambience/midway_loop.ogg` | HECKFRICKER | https://freesound.org/people/HECKFRICKER/sounds/635159/ |

Each sound has an evidence file in `assets/licenses/` with the retrieval date, the verbatim license statement and the exact edit made. The CC0 legal code is in `assets/licenses/CC0-1.0.txt`. Status and checksums for every asset are in `assets/ASSET_MANIFEST.md`.

**Third-party code:** SFML, FreeType, libogg, libvorbis, libFLAC, miniaudio, stb_image, minimp3 and glad are linked into the exe. Their notices are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md), and their full license texts ship in `licenses/third-party/`.

**This project's own source code:** no license has been chosen yet, so all rights are reserved by the author.

## Known limitations

- **Placeholder visuals.** All 19 required images (plus 8 optional ones) and the display font are missing. The game is designed around them and draws plain grey shapes in their place. That is why this is a preview build; the strict Release build will not start until they exist.
- **System font dependency.** Until a licensed display font is added, menus and the score display use Rockwell Bold, falling back to Georgia Bold, then Segoe UI, from the Windows Fonts folder. These are the only files the game reads from outside its own folder. If none of them is installed, the game stops at startup with a message.
- **Sounds pending review.** The sound clips were chosen by level and transient analysis and have not yet been approved by ear. In testing the game opened the real output device and played its cues, but nobody has listened to them yet. If no audio device is available, the game runs silently.
- **No music.** The optional band-organ track has not been installed; the N key has nothing to play.
- **Windows only, one machine tested.** Built and run on a single Windows 11 x64 PC. Other Windows versions, other GPUs, Linux and macOS are untested. The platform layer is Windows-specific, so other operating systems would also need code changes.
- **Performance not verified.** The 60 FPS target has not been measured cleanly with full-size images, because none exist yet.
- **Smaller issues** (from `docs/VERIFICATION_REPORT.md`): fire rate and shot timing are tied to frames (at 30 fps, holding the trigger fires every 267 ms instead of 250 ms); the score clack can sound just after a quick restart; the best score lasts only for the session; there is no icon, installer or settings screen.
