#pragma once

#include "gallery/assets/AssetCatalog.hpp"
#include "shell/assets/AssetPaths.hpp"

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace shell {

class Logger;

// What to do about an MVP image that is not on disk.
//   Fail     — D-012 behaviour: collect every problem, throw StartupError once.
//   StandIn  — the layout/inspection build: log it, and let the renderer draw a
//              labelled flat-grey layout card in its slot (Asset Guide §5.4,
//              "flat mid-grey card of the correct size and silhouette").
//              Nothing is ever substituted silently.
enum class MissingAssetPolicy { Fail, StandIn };

// Loads every catalog asset once at startup and owns it for the process
// lifetime (ARCHITECTURE §3). Construct after the window (needs a GL context).
// Never modified after construction, so references stay valid.
class AssetStore {
public:
    AssetStore(const AssetPaths& paths, Logger& log, MissingAssetPolicy policy);

    AssetStore(const AssetStore&) = delete;
    AssetStore& operator=(const AssetStore&) = delete;

    // nullptr when the image is missing (the renderer then draws a stand-in
    // or, for E1 assets, nothing).
    [[nodiscard]] const sf::Texture* texture(gallery::AssetId id) const noexcept;

    // Decoded effect (AssetKind::Sound); nullptr when missing: that cue is
    // silent. Buffers live as long as the store, which outlives AudioSystem
    // (ARCHITECTURE §3.2: an sf::Sound must die before its SoundBuffer).
    [[nodiscard]] const sf::SoundBuffer* sound(gallery::AssetId id) const noexcept;
    // Verified path of a streamed loop (AssetKind::Stream); nullopt when missing.
    [[nodiscard]] std::optional<std::filesystem::path> streamPath(gallery::AssetId id) const;

    // Display font: every piece of game lettering (HUD numerals, screens,
    // placards). Falls back to an installed slab serif (TEMP, D-029) while
    // FNT_DISPLAY is missing.
    [[nodiscard]] const sf::Font& font() const noexcept { return *font_; }
    [[nodiscard]] bool fontIsStandIn() const noexcept { return fontIsStandIn_; }
    // Utility face for stand-in labels and inspection text (never game UI).
    [[nodiscard]] const sf::Font& utilityFont() const noexcept {
        return utilityFont_ ? *utilityFont_ : *font_;
    }

    // Post-process fragment shader source; empty if unavailable.
    [[nodiscard]] const std::string& postProcessSource() const noexcept { return shaderSource_; }

    [[nodiscard]] const std::vector<gallery::AssetId>& missing() const noexcept { return missing_; }
    [[nodiscard]] std::size_t loadedCount() const noexcept { return loaded_; }

private:
    std::array<std::optional<sf::Texture>, gallery::kAssetCount> textures_;
    std::array<std::optional<sf::SoundBuffer>, gallery::kAssetCount> sounds_;
    std::array<std::optional<std::filesystem::path>, gallery::kAssetCount> streams_;
    std::optional<sf::Font> font_;
    std::optional<sf::Font> utilityFont_;
    bool fontIsStandIn_ = false;
    std::string shaderSource_;
    std::vector<gallery::AssetId> missing_;
    std::size_t loaded_ = 0;
};

} // namespace shell
