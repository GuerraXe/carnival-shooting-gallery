// sprite_bake — saves the game's grey shapes as its sprite files (D-044).
// Development tool only: never shipped, never run by the game.
//
// Every image sprite under assets/images/ is drawn by StandInPainter
// (src/shell/render/StandIn.cpp), the same code the renderer falls back on,
// at the size the game uses it, and saved as a PNG at its catalog path. To
// change a sprite, change the painter and re-run this tool; or edit the PNG
// in an image editor (and record that in the manifest).
//
//   sprite_bake <asset root> [--scale N] [KEY ...]     e.g. sprite_bake assets FG_RIFLE
//
// With no KEY, every sprite below is written. --scale N (default 1) saves
// N pixels per logical unit, for sharper sprites on high-DPI screens; the
// game places every sprite by its logical size, so it looks the same size.
// The duck is always 256x210 (its original 2 px per unit). Output is straight (not
// premultiplied) RGBA, with colour bled a few pixels into transparent areas
// so smooth scaling never darkens an edge.

#include "gallery/assets/AssetCatalog.hpp"
#include "gallery/flow/UiLayout.hpp"
#include "gallery/view/SceneLayout.hpp"
#include "shell/render/DrawUtil.hpp"
#include "shell/render/StandIn.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/View.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace L = gallery::layout;
using gallery::AssetId;
using gallery::Rect;
using shell::render::StandInPainter;

namespace {

struct Sprite {
    AssetId id;
    gallery::Size logical;   // size in logical units, as the game places it
    float scale;             // pixels per logical unit in the file
    bool antialias;          // 4x MSAA, like the game's world target
    std::function<void(sf::RenderTarget&, Rect, float scale)> paint;
};

sf::Font loadDisplayFont() {
    // The same system font the game uses for its lettering (AssetStore).
    fs::path windows = "C:\\Windows";
    char* windir = nullptr;
    std::size_t len = 0;
    if (_dupenv_s(&windir, &len, "WINDIR") == 0 && windir) {
        windows = windir;
    }
    std::free(windir);
    const fs::path fonts = windows / "Fonts";
    for (const char* name : {"ROCKB.TTF", "georgiab.ttf", "segoeui.ttf"}) {
        sf::Font f;
        if (f.openFromFile(fonts / name)) {
            std::cout << "font: " << (fonts / name).string() << "\n";
            return f;
        }
    }
    throw std::runtime_error("no display font (Rockwell Bold, Georgia Bold, Segoe UI) found");
}

// Straight alpha, and colour carried into fully transparent texels.
void finish(sf::Image& img) {
    const sf::Vector2u s = img.getSize();
    const auto w = static_cast<int>(s.x);
    const auto h = static_cast<int>(s.y);
    std::vector<bool> known(static_cast<std::size_t>(w) * h, false);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const sf::Vector2u p{static_cast<unsigned>(x), static_cast<unsigned>(y)};
            sf::Color c = img.getPixel(p);
            if (c.a == 0) {
                continue;
            }
            if (c.a < 255) {   // drawn onto a transparent target: premultiplied
                auto un = [&](std::uint8_t v) {
                    return static_cast<std::uint8_t>(
                        std::min(255L, std::lround(static_cast<float>(v) * 255.0f / c.a)));
                };
                c = sf::Color(un(c.r), un(c.g), un(c.b), c.a);
                img.setPixel(p, c);
            }
            known[static_cast<std::size_t>(y) * w + x] = true;
        }
    }
    for (int pass = 0; pass < 4; ++pass) {
        std::vector<bool> next = known;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (known[static_cast<std::size_t>(y) * w + x]) {
                    continue;
                }
                int r = 0, g = 0, b = 0, n = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int nx = x + dx, ny = y + dy;
                        if (nx < 0 || ny < 0 || nx >= w || ny >= h ||
                            !known[static_cast<std::size_t>(ny) * w + nx]) {
                            continue;
                        }
                        const sf::Color c =
                            img.getPixel({static_cast<unsigned>(nx), static_cast<unsigned>(ny)});
                        r += c.r;
                        g += c.g;
                        b += c.b;
                        ++n;
                    }
                }
                if (n > 0) {
                    img.setPixel({static_cast<unsigned>(x), static_cast<unsigned>(y)},
                                 sf::Color(static_cast<std::uint8_t>(r / n),
                                           static_cast<std::uint8_t>(g / n),
                                           static_cast<std::uint8_t>(b / n), 0));
                    next[static_cast<std::size_t>(y) * w + x] = true;
                }
            }
        }
        known = std::move(next);
    }
}

void bake(const Sprite& s, const fs::path& root, float extraScale) {
    const float scale = s.id == AssetId::TgtDuckA ? s.scale : s.scale * extraScale;
    const sf::Vector2u px{static_cast<unsigned>(std::lround(s.logical.width * scale)),
                          static_cast<unsigned>(std::lround(s.logical.height * scale))};
    sf::ContextSettings settings;
    if (s.antialias) {
        settings.antiAliasingLevel = 4;
    }
    sf::RenderTexture rt(px, settings);
    rt.setView(sf::View(sf::FloatRect({0.0f, 0.0f}, {s.logical.width, s.logical.height})));
    rt.clear(sf::Color::Transparent);
    s.paint(rt, {0.0f, 0.0f, s.logical.width, s.logical.height}, scale);
    rt.display();
    sf::Image img = rt.getTexture().copyToImage();
    finish(img);
    const fs::path out = root / fs::path(std::string(gallery::assetEntry(s.id).path));
    fs::create_directories(out.parent_path());
    if (!img.saveToFile(out)) {
        throw std::runtime_error("could not write " + out.string());
    }
    std::cout << gallery::assetEntry(s.id).key << ": " << px.x << "x" << px.y << " -> "
              << out.generic_string() << "\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: sprite_bake <asset root> [--scale N] [KEY ...]\n";
        return 2;
    }
    try {
        const fs::path root = argv[1];
        if (!fs::is_directory(root)) {
            throw std::runtime_error("asset root not found: " + root.string());
        }
        const StandInPainter p;
        std::optional<sf::Font> font;
        auto displayFont = [&]() -> const sf::Font& {
            if (!font) {
                font.emplace(loadDisplayFont());
            }
            return *font;
        };

        // The rifle is painted in barrel coordinates; in the 900x500 image the
        // barrel runs from the swivel to the muzzle (SceneLayout defaults), and
        // the game maps the image back through the same frame (drawRifleMesh).
        auto rifleImage = [](float along, float across) {
            const gallery::Vec2 axis = L::kRifleMuzzleInImage - L::kRiflePivotInImage;
            const float len = std::sqrt(axis.x * axis.x + axis.y * axis.y);
            const gallery::Vec2 a{axis.x / len, axis.y / len};
            const gallery::Vec2 n{-a.y, a.x};
            const gallery::Vec2 q = L::kRiflePivotInImage + a * (along * len) + n * across;
            return sf::Vector2f{q.x, q.y};
        };

        const std::vector<Sprite> sprites{
            {AssetId::BgFar, {L::kFarPlate.width, L::kFarPlate.height}, 1.0f, true,
             [&](auto& t, Rect r, float) { p.sky(t, r); }},
            {AssetId::BgMid, {L::kMidPlate.width, L::kMidPlate.height}, 1.0f, true,
             [&](auto& t, Rect r, float) { p.midground(t, r); }},
            {AssetId::FgBoothFrame, {L::kBoothFrame.width, L::kBoothFrame.height}, 1.0f, true,
             [&](auto& t, Rect, float) { p.boothFrame(t); }},
            {AssetId::FgBackboard, {L::kBackboard.width, L::kBackboard.height}, 1.0f, true,
             [&](auto& t, Rect r, float) { p.backboard(t, r); }},
            {AssetId::FgTrackRail, {64.0f, L::kTrackRail.height}, 1.0f, true,
             [&](auto& t, Rect r, float) { p.trackRail(t, r); }},
            {AssetId::FgWaveStrip, {L::kWaveStrip.width, L::kWaveStrip.height}, 1.0f, true,
             [&](auto& t, Rect r, float) { p.waveStrip(t, r); }},
            {AssetId::FgCounter, {L::kCounter.width, L::kCounter.height}, 1.0f, true,
             [&](auto& t, Rect r, float) { p.counter(t, r); }},
            {AssetId::FgRifle, L::kRifleImageSize, 1.0f, true,
             [&](auto& t, Rect, float) { p.rifle(t, rifleImage); }},
            // Same pixels the game rendered for its duck before (DuckVisuals:
            // 2 px per logical unit, no MSAA).
            {AssetId::TgtDuckA, {256.0f, 210.0f}, 1.0f, false,
             [&](auto& t, Rect r, float) { p.duck(t, r, shell::render::standin::kDuck); }},
            {AssetId::DecSignHeader, {L::kSignSlot.width, L::kSignSlot.height}, 1.0f, true,
             [&](auto& t, Rect r, float k) { p.sign(t, r, displayFont(), k); }},
            {AssetId::DecBulbLit, L::kBulbSize, 1.0f, true,
             [&](auto& t, Rect r, float) { p.bulb(t, r); }},
            {AssetId::DecBulbGlow, {L::kBulbGlowDiameter, L::kBulbGlowDiameter}, 1.0f, true,
             [&](auto& t, Rect r, float) { p.bulbGlow(t, r); }},
            {AssetId::DecBunting, {L::kBuntingSlot.width, L::kBuntingSlot.height}, 1.0f, true,
             [&](auto& t, Rect r, float) { p.bunting(t, r); }},
            {AssetId::UiScoreBoard, {420.0f, 180.0f}, 1.0f, true,
             [&](auto& t, Rect r, float) { p.board(t, r); }},
            {AssetId::UiTimerBoard, {300.0f, 180.0f}, 1.0f, true,
             [&](auto& t, Rect r, float) { p.board(t, r); }},
            {AssetId::UiPanel, {gallery::ui::kPanel.width, gallery::ui::kPanel.height}, 1.0f, true,
             [&](auto& t, Rect r, float) { p.board(t, r); }},
            {AssetId::UiReticle, {L::kReticleSize, L::kReticleSize}, 1.0f, true,
             [&](auto& t, Rect r, float) {
                 p.reticle(t, {r.width / 2.0f, r.height / 2.0f}, r.width, 0.0f,
                           sf::Color(232, 228, 218));
             }},
            // The game draws the mark at kMissMarkSize; the dot's radius is 6.
            {AssetId::OvMissMark, {L::kMissMarkSize, L::kMissMarkSize}, 1.0f, true,
             [&](auto& t, Rect r, float) {
                 p.missMark(t, {r.width / 2.0f, r.height / 2.0f}, 6.0f, 1.0f);
             }},
            {AssetId::OvMuzzlePuff, {L::kMuzzlePuffSize, L::kMuzzlePuffSize}, 1.0f, true,
             [&](auto& t, Rect r, float) { p.muzzlePuff(t, r, 1.0f); }},
        };

        float extraScale = 1.0f;
        std::vector<std::string> only;
        for (int i = 2; i < argc; ++i) {
            const std::string a = argv[i];
            if (a == "--scale" && i + 1 < argc) {
                extraScale = std::stof(argv[++i]);
                if (!(extraScale >= 1.0f && extraScale <= 4.0f)) {
                    throw std::runtime_error("--scale must be between 1 and 4");
                }
            } else {
                only.push_back(a);
            }
        }
        int written = 0;
        for (const Sprite& s : sprites) {
            const std::string key(gallery::assetEntry(s.id).key);
            if (!only.empty() && std::find(only.begin(), only.end(), key) == only.end()) {
                continue;
            }
            bake(s, root, extraScale);
            ++written;
        }
        if (written == 0) {
            throw std::runtime_error("no sprite matched the given keys");
        }
        std::cout << written << " sprite(s) written\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "sprite_bake: " << e.what() << "\n";
        return 1;
    }
}
