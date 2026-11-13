#include "shell/app/LaunchOptions.hpp"

#include <charconv>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace shell {

namespace {

std::filesystem::path utf8Path(std::string_view s) {
    // The exe manifest sets the UTF-8 active code page, so argv is UTF-8.
    return std::filesystem::path(std::u8string(s.begin(), s.end()));
}

template <class T>
bool parseNumber(std::string_view v, T& out) {
    const auto [end, ec] = std::from_chars(v.data(), v.data() + v.size(), out);
    return ec == std::errc{} && end == v.data() + v.size();
}

// "A<sep>B" -> two numbers.
template <class T>
bool parsePair(std::string_view v, char sep, T& a, T& b) {
    const auto at = v.find(sep);
    return at != std::string_view::npos && parseNumber(v.substr(0, at), a) &&
           parseNumber(v.substr(at + 1), b);
}

} // namespace

LaunchOptions parseLaunchOptions(int argc, const char* const* argv) {
    using Step = LaunchOptions::ScriptStep;
    LaunchOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        auto value = [&]() -> std::string_view {
            if (i + 1 >= argc) {
                throw std::invalid_argument(std::string(arg) + " needs a value");
            }
            return argv[++i];
        };
        auto positive = [&](std::string_view v) {
            std::uint64_t n = 0;
            if (!parseNumber(v, n) || n == 0) {
                throw std::invalid_argument(std::string(arg) + " expects a positive integer, got '" +
                                            std::string(v) + "'");
            }
            return n;
        };

        if (arg == "--frames") {
            options.maxFrames = positive(value());
        } else if (arg == "--screenshot") {
            options.screenshot = utf8Path(value());
        } else if (arg == "--assets") {
            options.assetRoot = utf8Path(value());
        } else if (arg == "--no-dialog") {
            options.fatalDialog = false;
        } else if (arg == "--stand-ins") {
            options.standIns = true;
        } else if (arg == "--inspect") {
            options.inspect = true;
        } else if (arg == "--inspect-targets") {
            options.inspectTargets = true;
        } else if (arg == "--window") {
            const std::string_view v = value();
            std::uint32_t w = 0, h = 0;
            if (!parsePair(v, 'x', w, h) || w < 64 || h < 64 || w > 16384 || h > 16384) {
                throw std::invalid_argument("--window expects WxH (64..16384), got '" +
                                            std::string(v) + "'");
            }
            options.windowWidth = w;
            options.windowHeight = h;
        } else if (arg == "--aim") {
            const std::string_view v = value();
            float x = 0, y = 0;
            if (!parsePair(v, ',', x, y)) {
                throw std::invalid_argument("--aim expects X,Y in logical units, got '" +
                                            std::string(v) + "'");
            }
            options.aimX = x;
            options.aimY = y;
        } else if (arg == "--fire" || arg == "--click") {
            options.script.push_back({positive(value()), Step::Kind::Click});
        } else if (arg == "--focus-lost") {
            options.script.push_back({positive(value()), Step::Kind::FocusLost});
        } else if (arg == "--focus-gained") {
            options.script.push_back({positive(value()), Step::Kind::FocusGained});
        } else if (arg == "--close") {
            options.script.push_back({positive(value()), Step::Kind::Close});
        } else if (arg == "--key") {
            const std::string_view v = value();
            const auto colon = v.find(':');
            std::uint64_t frame = 0;
            const std::string_view name = colon == std::string_view::npos ? std::string_view{} : v.substr(colon + 1);
            std::optional<gallery::Key> key;
            if (name == "esc") key = gallery::Key::Escape;
            else if (name == "space") key = gallery::Key::Space;
            else if (name == "enter") key = gallery::Key::Enter;
            else if (name == "r") key = gallery::Key::R;
            else if (name == "p") key = gallery::Key::P;
            if (colon == std::string_view::npos || !parseNumber(v.substr(0, colon), frame) || frame == 0 || !key) {
                throw std::invalid_argument("--key expects N:NAME with NAME one of esc, space, enter, r, p; got '" +
                                            std::string(v) + "'");
            }
            options.script.push_back({frame, Step::Kind::Key, *key});
        } else if (arg == "--targets") {
            const std::string_view v = value();
            if (v == "sections") {
                options.lanePreset = gallery::LanePreset::Sections;
            } else if (v == "chain") {
                options.lanePreset = gallery::LanePreset::Chain;
            } else {
                throw std::invalid_argument("--targets expects sections or chain, got '" +
                                            std::string(v) + "'");
            }
        } else if (arg == "--reaction") {
            const std::string_view v = value();
            if (v == "flip") {
                options.reaction = gallery::HitReaction::FlipBack;
            } else if (v == "drop") {
                options.reaction = gallery::HitReaction::TiltDrop;
            } else {
                throw std::invalid_argument("--reaction expects flip or drop, got '" +
                                            std::string(v) + "'");
            }
        } else if (arg == "--knock") {
            options.knockAtFrame = positive(value());
        } else if (arg == "--sim-dt") {
            const std::string_view v = value();
            double s = 0.0;
            if (!parseNumber(v, s) || !(s > 0.0) || s > 0.1) {
                throw std::invalid_argument("--sim-dt expects seconds in (0, 0.1], got '" +
                                            std::string(v) + "'");
            }
            options.simDt = s;
        } else if (arg == "--fps") {
            const std::uint64_t n = positive(value());
            if (n > 1000) {
                throw std::invalid_argument("--fps expects 1..1000");
            }
            options.fps = static_cast<std::uint32_t>(n);
        } else if (arg == "--volume") {
            const std::string_view v = value();
            std::uint64_t n = 0;
            if (!parseNumber(v, n) || n > 100) {
                throw std::invalid_argument("--volume expects 0..100, got '" + std::string(v) + "'");
            }
            options.volumePercent = static_cast<int>(n);
        } else if (arg == "--mute") {
            options.mute = true;
        } else if (arg == "--no-music") {
            options.music = false;
        } else if (arg == "--no-audio") {
            options.audio = false;
        } else if (arg == "--log-audio") {
            options.logAudio = true;
        } else if (arg == "--no-flicker") {
            options.flicker = false;
        } else {
            throw std::invalid_argument(
                "unknown option '" + std::string(arg) +
                "' (valid: --frames N, --screenshot PATH, --assets DIR, --no-dialog, --stand-ins, "
                "--inspect, --inspect-targets, --window WxH, --aim X,Y, --fire N, --click N, --key N:NAME, "
                "--focus-lost N, --focus-gained N, --close N, --targets sections|chain, "
                "--reaction flip|drop, --knock N, --sim-dt S, --fps N, --volume N, --mute, "
                "--no-music, --no-audio, --log-audio, --no-flicker)");
        }
    }
    if (options.screenshot && !options.maxFrames) {
        throw std::invalid_argument("--screenshot requires --frames N (the shot is taken on "
                                    "the last frame)");
    }
    return options;
}

} // namespace shell
