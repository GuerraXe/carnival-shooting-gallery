#pragma once

#include "gallery/flow/Game.hpp"
#include "gallery/game/DuckConfig.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace shell {

// Command-line switches. None are needed to play; they exist for automated
// smoke tests and diagnostics (see docs/BUILD.md).
//
//   --frames N         run N rendered frames, then shut down normally
//   --screenshot PATH  with --frames: save the last frame as PNG to PATH
//   --assets DIR       use DIR as the asset root instead of <exe>/assets
//   --no-dialog        report fatal errors to the log/stderr only (no message box)
//   --stand-ins        draw labelled grey layout cards for missing MVP images
//                      instead of refusing to start (always on in Debug builds)
//   --inspect          start with the F3 inspection overlay on
//   --inspect-targets  start in the F3 target view (bounds, hit regions, state table)
//   --window WxH       initial client size in physical pixels (aspect-ratio tests)
//   --aim X,Y          initial aim point in logical 1920x1080 units
//   --fire N           left click at the aim at frame N (a shot only while a round
//                      is live; on a board it presses the placard under the aim)
//   --click N          same as --fire
//   --key N:NAME       press a key at frame N: esc, space, enter, r, p
//   --focus-lost N     the window loses focus at frame N (and --focus-gained N)
//   --close N          window close request at frame N
//                      (all scripted input is repeatable and runs in the order given
//                      per frame, before that frame's update, like real input)
//   --targets NAME     target preset: sections (default, D-027) or chain (brief G-14)
//   --reaction NAME    hit reaction for every target: flip (default) or drop
//   --knock N          debug: knock down the next upright target at frame N
//   --sim-dt S         feed exactly S seconds per frame (0 < S <= 0.1) instead of
//                      the wall clock: deterministic runs for frame-rate checks
//   --fps N            vsync off, frame rate capped at N (frame-rate checks)
//   --volume N         master volume 0..100 (default 80; - and = change it in game)
//   --mute             start muted (M toggles)
//   --no-music         start with the optional band-organ music off (N toggles)
//   --no-audio         never open the audio device (silent; for headless runs)
//   --log-audio        log every sound cue as it plays (verification)
//   --no-flicker       steady lighting: no bulb dips, generator sags or bunting sway
struct LaunchOptions {
    std::optional<std::uint64_t> maxFrames;
    std::optional<std::filesystem::path> screenshot;
    std::optional<std::filesystem::path> assetRoot;
    bool fatalDialog = true;
    bool standIns = false;
    bool inspect = false;
    bool inspectTargets = false;
    std::optional<std::uint32_t> windowWidth;
    std::optional<std::uint32_t> windowHeight;
    std::optional<float> aimX;
    std::optional<float> aimY;
    // Scripted input for end-to-end smoke tests (--fire/--click/--key/--focus-*/--close).
    struct ScriptStep {
        enum class Kind { Click, Key, FocusLost, FocusGained, Close };
        std::uint64_t frame = 0;
        Kind kind = Kind::Click;
        gallery::Key key = gallery::Key::Escape;
    };
    std::vector<ScriptStep> script;
    gallery::LanePreset lanePreset = gallery::LanePreset::Sections;
    std::optional<gallery::HitReaction> reaction;
    std::optional<std::uint64_t> knockAtFrame;
    std::optional<double> simDt;
    std::optional<std::uint32_t> fps;
    int volumePercent = 80;
    bool mute = false;
    bool music = true;
    bool audio = true;
    bool logAudio = false;
    bool flicker = true;
};

// Throws std::invalid_argument with a human-readable message on bad input.
[[nodiscard]] LaunchOptions parseLaunchOptions(int argc, const char* const* argv);

} // namespace shell
