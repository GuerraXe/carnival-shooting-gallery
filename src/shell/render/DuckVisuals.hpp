#pragma once

// Target images and the hit shapes derived from them (D-009, D-027, R-3).
//
// For each paint variant (TGT_DUCK_A/B/C) the image that will be DRAWN is
// also the image the hit mask is built from:
//   - the photograph, when the file exists (B and C fall back to A);
//   - otherwise the labelled flat-grey stand-in silhouette (Asset Guide §5.4),
//     rendered once into a texture so it goes through exactly the same
//     image -> logical transform as a photograph would.
// The logic layer receives plain gallery::DuckShape values (no SFML).

#include "gallery/game/DuckLane.hpp"

#include <SFML/Graphics/Texture.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace shell {
class AssetStore;
class Logger;
} // namespace shell

namespace shell::render {

class DuckVisuals {
public:
    static constexpr std::size_t kVariants = 3;

    // Needs the GL context (construct after the window and the AssetStore).
    DuckVisuals(const AssetStore& assets, Logger& log);

    DuckVisuals(const DuckVisuals&) = delete;
    DuckVisuals& operator=(const DuckVisuals&) = delete;

    // Index = paint variant.
    [[nodiscard]] const std::vector<gallery::DuckShape>& shapes() const noexcept { return shapes_; }
    [[nodiscard]] std::vector<float> widths() const;

    // The texture to draw for a variant (photo or stand-in). Never null.
    [[nodiscard]] const sf::Texture& image(std::size_t variant) const noexcept {
        return *images_[variant % kVariants];
    }
    // Opaque-white-where-solid texture of the hit mask (inspection overlay).
    [[nodiscard]] const sf::Texture& maskImage(std::size_t variant) const noexcept {
        return masks_[variant % kVariants];
    }
    [[nodiscard]] bool isStandIn(std::size_t variant) const noexcept {
        return standIn_[variant % kVariants];
    }

private:
    std::vector<gallery::DuckShape> shapes_;
    std::array<const sf::Texture*, kVariants> images_{};
    std::array<bool, kVariants> standIn_{};
    std::array<sf::Texture, kVariants> masks_;
    std::optional<sf::Texture> standInImage_;
};

} // namespace shell::render
