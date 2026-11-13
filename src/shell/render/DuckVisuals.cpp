#include "shell/render/DuckVisuals.hpp"

#include "gallery/view/SceneLayout.hpp"
#include "shell/assets/AssetStore.hpp"
#include "shell/platform/Logger.hpp"
#include "shell/platform/StartupError.hpp"
#include "shell/render/DrawUtil.hpp"
#include "shell/render/StandIn.hpp"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTexture.hpp>

#include <cmath>
#include <exception>
#include <format>
#include <span>

namespace shell::render {

using gallery::AssetId;
namespace L = gallery::layout;

namespace {

// The stand-in card is rendered at 2x its on-screen size, like the target
// photos (Asset Guide §2.4: ducks at 2x for crisp scaling and accurate masks).
constexpr float kStandInSupersample = 2.0f;

gallery::AlphaMask maskOf(const sf::Image& img) {
    const sf::Vector2u s = img.getSize();
    return gallery::AlphaMask::fromRgba(
        std::span<const std::uint8_t>(img.getPixelsPtr(), static_cast<std::size_t>(s.x) * s.y * 4),
        static_cast<int>(s.x), static_cast<int>(s.y));
}

sf::Texture maskTexture(const gallery::AlphaMask& m) {
    sf::Image img({static_cast<unsigned>(m.width()), static_cast<unsigned>(m.height())},
                  sf::Color::Transparent);
    for (int y = 0; y < m.height(); ++y) {
        for (int x = 0; x < m.width(); ++x) {
            if (m.solidAt(x, y)) {
                img.setPixel({static_cast<unsigned>(x), static_cast<unsigned>(y)}, sf::Color::White);
            }
        }
    }
    sf::Texture t;
    if (!t.loadFromImage(img)) {
        throw StartupError("Could not create the target hit-mask texture (out of video memory?)");
    }
    return t;
}

} // namespace

DuckVisuals::DuckVisuals(const AssetStore& assets, Logger& log) {
    const AssetId ids[kVariants] = {AssetId::TgtDuckA, AssetId::TgtDuckB, AssetId::TgtDuckC};
    const sf::Texture* fallback = assets.texture(AssetId::TgtDuckA);
    shapes_.resize(kVariants);

    for (std::size_t v = 0; v < kVariants; ++v) {
        AssetId used = ids[v];
        const sf::Texture* tex = assets.texture(ids[v]);
        if (!tex && fallback) {
            tex = fallback;   // E1 variants B/C reuse A until photographed
            used = AssetId::TgtDuckA;
        }
        if (tex) {
            const sf::Image img = tex->copyToImage();
            const auto iw = static_cast<float>(img.getSize().x);
            const auto ih = static_cast<float>(img.getSize().y);
            gallery::DuckShape& s = shapes_[v];
            s.size = {L::kDuckHeight * iw / ih, L::kDuckHeight};
            s.imageSize = {iw, ih};
            s.mask = maskOf(img);
            images_[v] = tex;
            standIn_[v] = false;
            log.info(std::format("Target variant {}: {} {}x{} px -> {:.1f}x{:.1f} logical, "
                                 "hit mask {} solid texels",
                                 static_cast<char>('A' + v),
                                 gallery::assetEntry(used).key,
                                 img.getSize().x, img.getSize().y, s.size.width, s.size.height,
                                 s.mask.solidCount()));
            continue;
        }

        // No photograph: render the labelled stand-in silhouette once.
        if (!standInImage_) {
            const gallery::Size logical{L::kDuckHeight * L::kDuckStandInAspect, L::kDuckHeight};
            const sf::Vector2u px{
                static_cast<unsigned>(std::lround(logical.width * kStandInSupersample)),
                static_cast<unsigned>(std::lround(logical.height * kStandInSupersample))};
            std::optional<sf::RenderTexture> rt;
            try {
                rt.emplace(px);
            } catch (const std::exception& e) {
                throw StartupError(std::string("Could not create the target stand-in texture: ") +
                                   e.what());
            }
            StandInPainter painter;
            rt->clear(sf::Color::Transparent);
            const gallery::Rect r{0.0f, 0.0f, static_cast<float>(px.x), static_cast<float>(px.y)};
            painter.duck(*rt, r, standin::kDuck);
            rt->display();
            // The hit mask comes from the silhouette as drawn.
            const gallery::AlphaMask silhouette = maskOf(rt->getTexture().copyToImage());
            standInImage_.emplace(rt->getTexture());
            standInImage_->setSmooth(true);

            gallery::DuckShape s;
            s.size = logical;
            s.imageSize = {static_cast<float>(px.x), static_cast<float>(px.y)};
            s.mask = silhouette;
            shapes_[v] = s;
            log.info(std::format("Target variant {}: MISSING photo -> stand-in silhouette "
                                 "{:.1f}x{:.1f} logical, hit mask {} solid texels of {}x{}",
                                 static_cast<char>('A' + v), logical.width, logical.height,
                                 silhouette.solidCount(), px.x, px.y));
        } else {
            shapes_[v] = shapes_[0];
        }
        images_[v] = &*standInImage_;
        standIn_[v] = true;
    }

    for (std::size_t v = 0; v < kVariants; ++v) {
        masks_[v] = maskTexture(shapes_[v].mask);
    }
}

std::vector<float> DuckVisuals::widths() const {
    std::vector<float> w;
    for (const auto& s : shapes_) {
        w.push_back(s.size.width);
    }
    return w;
}

} // namespace shell::render
