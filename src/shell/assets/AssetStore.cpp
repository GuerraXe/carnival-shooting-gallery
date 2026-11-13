#include "shell/assets/AssetStore.hpp"

#include "shell/platform/Logger.hpp"
#include "shell/platform/Platform.hpp"
#include "shell/platform/StartupError.hpp"

#include <SFML/Audio/InputSoundFile.hpp>

#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>

namespace shell {

namespace fs = std::filesystem;
using gallery::AssetEntry;
using gallery::AssetId;
using gallery::AssetKind;
using gallery::AssetPriority;

namespace {

// Plates and props are photographs that the renderer scales down (and at
// 4K slightly up). Smooth filtering + mipmaps avoid shimmer and aliasing
// without adding any sharpening.
void configurePhoto(sf::Texture& t, AssetId id) {
    t.setSmooth(true);
    if (id == AssetId::FgTrackRail || id == AssetId::OvGrain) {
        t.setRepeated(true);   // tiled strip / tiled grain plate
    }
    // Mipmaps for images drawn well below their native size. The grain plate
    // is sampled 1:1 and must keep its full-resolution structure.
    if (id != AssetId::OvGrain) {
        (void)t.generateMipmap();   // best effort; plain bilinear if unsupported
    }
}

// What the renderer does instead while an MVP asset is missing.
const char* fallbackFor(AssetId id) {
    switch (id) {
    case AssetId::FntDisplay: return "installed slab-serif system font (TEMP, local only, never shipped)";
    case AssetId::UiReticle: return "TEMP functional reticle (Q-2 pending)";
    case AssetId::OvGrain: return "film grain OFF (no procedural substitute, D-022)";
    case AssetId::DecBulbGlow: return "no glow drawn (no synthetic gradient)";
    case AssetId::UiPanel: return "labelled grey board behind the title/pause/results lettering";
    default:
        break;
    }
    const AssetKind kind = gallery::assetEntry(id).kind;
    if (kind == AssetKind::Sound || kind == AssetKind::Stream) {
        return "silent (no synthesised substitute)";
    }
    return "labelled grey layout stand-in";
}

} // namespace

AssetStore::AssetStore(const AssetPaths& paths, Logger& log, MissingAssetPolicy policy) {
    std::vector<std::string> fatal;

    for (const AssetEntry& e : gallery::kAssetCatalog) {
        const fs::path file = paths.resolve(e.path);
        const bool present = fs::is_regular_file(file);

        if (!present) {
            missing_.push_back(e.id);
            continue;
        }
        if (e.kind == AssetKind::Font) {
            // A font file that exists but does not open is an error like any
            // other undecodable asset, never a silent switch to the stand-in.
            sf::Font f;
            if (!f.openFromFile(file)) {
                fatal.push_back(std::format("{}: could not decode {}", e.key, platform::toUtf8(file)));
                continue;
            }
            font_.emplace(std::move(f));
            ++loaded_;
            log.info(std::format("Loaded {} ({})", e.key, e.path));
            continue;
        }
        if (e.kind == AssetKind::Sound) {
            // Short effects are decoded once here, never on the event path (D-019).
            sf::SoundBuffer b;
            if (!b.loadFromFile(file)) {
                fatal.push_back(std::format("{}: could not decode {}", e.key, platform::toUtf8(file)));
                continue;
            }
            log.info(std::format("Loaded {} ({}) {:.3f} s, {} Hz, {} ch", e.key, e.path,
                                 b.getDuration().asSeconds(), b.getSampleRate(),
                                 b.getChannelCount()));
            sounds_[static_cast<std::size_t>(e.id)].emplace(std::move(b));
            ++loaded_;
            continue;
        }
        if (e.kind == AssetKind::Stream) {
            // Long loops stream while playing; check now that the file decodes,
            // so a broken file fails at startup instead of going quiet later.
            sf::InputSoundFile probe;
            if (!probe.openFromFile(file)) {
                fatal.push_back(std::format("{}: could not decode {}", e.key, platform::toUtf8(file)));
                continue;
            }
            log.info(std::format("Found {} ({}) {:.1f} s, {} Hz, {} ch (streamed)", e.key, e.path,
                                 probe.getDuration().asSeconds(), probe.getSampleRate(),
                                 probe.getChannelCount()));
            streams_[static_cast<std::size_t>(e.id)] = file;
            ++loaded_;
            continue;
        }
        sf::Texture t;
        if (!t.loadFromFile(file)) {
            // A file that exists but does not decode is always an error.
            fatal.push_back(std::format("{}: could not decode {}", e.key, platform::toUtf8(file)));
            continue;
        }
        configurePhoto(t, e.id);
        log.info(std::format("Loaded {} ({}) {}x{}", e.key, e.path, t.getSize().x, t.getSize().y));
        textures_[static_cast<std::size_t>(e.id)].emplace(std::move(t));
        ++loaded_;
    }

    // Report missing assets: MVP ones are errors under the Fail policy.
    std::size_t mvpMissing = 0;
    for (const AssetId id : missing_) {
        const AssetEntry& e = gallery::assetEntry(id);
        const bool mvp = e.priority == AssetPriority::Mvp;
        mvpMissing += mvp ? 1 : 0;
        if (mvp && policy == MissingAssetPolicy::Fail) {
            fatal.push_back(std::format("{}: missing assets/{}", e.key, e.path));
        } else {
            log.warn(std::format("MISSING {} (assets/{}) -> {}", e.key, e.path,
                                 mvp ? fallbackFor(id)
                                 : (e.kind == AssetKind::Sound || e.kind == AssetKind::Stream)
                                     ? "optional (E1), not played"
                                     : "optional (E1), not drawn"));
        }
    }

    if (!fatal.empty()) {
        std::ostringstream msg;
        msg << fatal.size() << " required asset problem(s) under " << platform::toUtf8(paths.root())
            << ":\n";
        for (const auto& line : fatal) {
            msg << "  " << line << "\n";
        }
        msg << "See assets/ASSET_MANIFEST.md. For a layout inspection build, run with --stand-ins.";
        throw StartupError(msg.str());
    }

    // Display font fallback (TEMP, D-029): while FNT_DISPLAY is missing, use
    // the closest installed relative of painted showcard lettering: a slab
    // serif, then a book serif, then the M0 sans. These are read from the
    // local Windows font folder at run time and never copied into assets/.
    if (!font_) {
        for (const char* name : {"ROCKB.TTF", "georgiab.ttf", "segoeui.ttf"}) {
            const fs::path sys = platform::windowsFontPath(name);
            sf::Font f;
            if (fs::is_regular_file(sys) && f.openFromFile(sys)) {
                font_.emplace(std::move(f));
                fontIsStandIn_ = true;
                log.warn("Display font stand-in: " + platform::toUtf8(sys) +
                         " (TEMP until FNT_DISPLAY)");
                break;
            }
        }
        if (!font_) {
            throw StartupError("No display font: assets/" +
                               std::string(gallery::assetEntry(AssetId::FntDisplay).path) +
                               " is missing and no system fallback (Rockwell, Georgia, Segoe UI) "
                               "could be opened.");
        }
    }
    // Utility face for stand-in labels and the inspection overlay: plainly
    // not part of the game's lettering, so a "MISSING" card can never be
    // mistaken for a painted sign. Falls back to the display font.
    {
        const fs::path sys = platform::windowsFontPath("segoeui.ttf");
        sf::Font f;
        if (fs::is_regular_file(sys) && f.openFromFile(sys)) {
            utilityFont_.emplace(std::move(f));
        }
    }

    // Shader source (project code, degradable: missing -> plain blit).
    const fs::path shader = paths.resolve(gallery::kPostProcessShaderPath);
    std::ifstream in(shader, std::ios::binary);
    if (in) {
        std::ostringstream ss;
        ss << in.rdbuf();
        shaderSource_ = ss.str();
    } else {
        log.warn("Post-process shader not found: " + platform::toUtf8(shader) +
                 " (post-process disabled)");
    }

    log.info(std::format("Assets: {} loaded, {} missing ({} MVP shown as stand-ins)", loaded_,
                         missing_.size(), mvpMissing));
}

const sf::Texture* AssetStore::texture(AssetId id) const noexcept {
    const auto& t = textures_[static_cast<std::size_t>(id)];
    return t ? &*t : nullptr;
}

const sf::SoundBuffer* AssetStore::sound(AssetId id) const noexcept {
    const auto& s = sounds_[static_cast<std::size_t>(id)];
    return s ? &*s : nullptr;
}

std::optional<fs::path> AssetStore::streamPath(AssetId id) const {
    return streams_[static_cast<std::size_t>(id)];
}

} // namespace shell
