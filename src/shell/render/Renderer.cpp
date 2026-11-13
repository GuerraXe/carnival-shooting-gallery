#include "shell/render/Renderer.hpp"

#include "shell/assets/AssetStore.hpp"
#include "shell/platform/Logger.hpp"
#include "shell/platform/StartupError.hpp"
#include "shell/render/DrawUtil.hpp"
#include "shell/render/Theme.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/View.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <utility>

namespace shell::render {

using gallery::AssetId;
using gallery::DrawPass;
using gallery::Fit;
using gallery::LayerGroup;
using gallery::Rect;
using gallery::Seconds;
using gallery::Vec2;
namespace L = gallery::layout;

namespace {

const sf::Color kLetterbox{0, 0, 0};
const sf::Color kOverlayText{236, 230, 214};
const sf::Color kDecoyTint{235, 92, 78};     // "don't shoot" red
const sf::Color kGoldenTint{255, 206, 70};   // power-up gold

float smoothstep(float e0, float e1, float x) {
    const float t = std::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// D-039: the rifle is drawn through gallery::rifleImageToLogical (a perspective
// map), so a photograph becomes a finely subdivided textured mesh.
void drawRifleMesh(sf::RenderTarget& t, const sf::Texture& tex, const gallery::RiflePose& pose) {
    constexpr int kCols = 48;   // along the gun
    constexpr int kRows = 12;   // across it
    const sf::Vector2u size = tex.getSize();
    const float w = static_cast<float>(size.x);
    const float h = static_cast<float>(size.y);
    // The manifest records pivot and muzzle in the 900x500 spec frame.
    const Vec2 k{w / L::kRifleImageSize.width, h / L::kRifleImageSize.height};
    auto vertex = [&](int c, int r) {
        const Vec2 img{w * static_cast<float>(c) / kCols, h * static_cast<float>(r) / kRows};
        // Image units are rescaled to the spec frame so the on-screen size
        // does not depend on the photo's resolution.
        const Vec2 spec{img.x / k.x, img.y / k.y};
        return sf::Vertex{toSf(gallery::rifleImageToLogical(spec, pose)), sf::Color::White,
                          toSf(img)};
    };
    sf::VertexArray mesh(sf::PrimitiveType::Triangles);
    for (int c = 0; c < kCols; ++c) {
        for (int r = 0; r < kRows; ++r) {
            const sf::Vertex a = vertex(c, r), b = vertex(c + 1, r), d = vertex(c, r + 1),
                             e = vertex(c + 1, r + 1);
            mesh.append(a);
            mesh.append(b);
            mesh.append(e);
            mesh.append(a);
            mesh.append(e);
            mesh.append(d);
        }
    }
    t.draw(mesh, sf::RenderStates(&tex));
}

sf::Color groupColour(LayerGroup g) {
    switch (g) {
    case LayerGroup::Environment: return {120, 200, 255};
    case LayerGroup::Decorations: return {255, 170, 60};
    case LayerGroup::Track: return {170, 255, 120};
    case LayerGroup::Targets: return {255, 240, 80};
    case LayerGroup::Counter: return {200, 140, 255};
    case LayerGroup::Rifle: return {255, 110, 110};
    case LayerGroup::Interface: return {255, 255, 255};
    case LayerGroup::Effects: return {80, 255, 230};
    case LayerGroup::Count: break;
    }
    return sf::Color::White;
}

} // namespace

// ---------------------------------------------------------------------------
// EffectsState
// ---------------------------------------------------------------------------

namespace {
// 1 at t = 0 easing to 0 at t = life (quadratic ease-out); 0 outside.
float decay(Seconds t, Seconds life) noexcept {
    const double x = t / life;
    if (x >= 1.0 || x < 0.0) {
        return 0.0f;
    }
    return static_cast<float>((1.0 - x) * (1.0 - x));
}
} // namespace

void EffectsState::onShot(Vec2 muzzle) {
    sinceShot_ = 0.0;
    puffAge_ = 0.0;
    puffPos_ = muzzle;
}

void EffectsState::onMiss(Vec2 at) {
    // A pellet dent only where there is backboard to dent (G-10); a miss into
    // the sky above the booth or the counter leaves no mark.
    if (L::kMissMarkRegion.contains(at)) {
        marks_[nextMark_] = Mark{at, 0.0};
        nextMark_ = (nextMark_ + 1) % kMaxMissMarks;
    }
}

void EffectsState::onHit(Vec2 at, int points, gallery::TargetKind kind) {
    sinceHit_ = 0.0;
    tags_[nextTag_] = Tag{at, points, kind, 0.0};
    nextTag_ = (nextTag_ + 1) % kMaxTags;
}

void EffectsState::update(Seconds dt) {
    sinceShot_ += dt;
    sinceHit_ += dt;
    puffAge_ = std::min(puffAge_ + dt, kPuffLife);
    for (Mark& m : marks_) {
        m.age = std::min(m.age + dt, kMissMarkLife);
    }
    for (Tag& g : tags_) {
        g.age = std::min(g.age + dt, kTagLife);
    }
}

void EffectsState::clear() {
    *this = EffectsState{};
}

float EffectsState::recoil() const noexcept {
    return decay(sinceShot_, kRecoilTime);
}

float EffectsState::kick() const noexcept {
    return decay(sinceShot_, kKickTime);
}

float EffectsState::hitFlash() const noexcept {
    return decay(sinceHit_, kHitFlashTime);
}

// ---------------------------------------------------------------------------
// Renderer
// ---------------------------------------------------------------------------

Renderer::Renderer(const AssetStore& assets, const DuckVisuals& ducks, Logger& log)
    : assets_(assets),
      ducks_(ducks),
      log_(log),
      post_(assets.postProcessSource(), assets.texture(AssetId::OvGrain), log),
      screens_(assets) {
    groupsOn_.fill(true);
}

gallery::Occluder Renderer::buildForegroundOccluder() {
    // One texel per logical unit. The static passes drawn after the ducks;
    // the rest that follow them cannot reach the lane and are excluded on
    // purpose: the rifle moves but never covers the lane, the muzzle puff is
    // momentary at the muzzle, the bunting and bulbs hang from the header
    // beam far above the targets, and the HUD boards are off the lane
    // (all checked in tests/scene_layout_tests.cpp).
    const sf::Vector2u size{static_cast<unsigned>(gallery::kLogicalSize.width),
                            static_cast<unsigned>(gallery::kLogicalSize.height)};
    std::optional<sf::RenderTexture> rt;
    try {
        rt.emplace(size);
    } catch (const std::exception& e) {
        throw StartupError(std::string("Could not create the foreground hit-mask target: ") +
                           e.what());
    }
    rt->clear(sf::Color::Transparent);
    const float savedScale = pixelScale_;
    pixelScale_ = 1.0f;
    for (const DrawPass pass :
         {DrawPass::WaveStrip, DrawPass::BoothFrame, DrawPass::Sign, DrawPass::Counter}) {
        drawPass(*rt, pass);
    }
    pixelScale_ = savedScale;
    rt->display();
    const sf::Image img = rt->getTexture().copyToImage();
    const sf::Vector2u s = img.getSize();
    gallery::AlphaMask mask = gallery::AlphaMask::fromRgba(
        std::span<const std::uint8_t>(img.getPixelsPtr(), static_cast<std::size_t>(s.x) * s.y * 4),
        static_cast<int>(s.x), static_cast<int>(s.y));
    log_.info(std::format("Foreground in front of the targets (wave cutout, booth frame, sign, "
                          "counter): {} of {} logical texels opaque; shots there hit the scenery",
                          mask.solidCount(), static_cast<std::size_t>(s.x) * s.y));
    return gallery::Occluder(std::move(mask));
}

void Renderer::consume(const std::vector<gallery::GameEvent>& events) {
    for (const gallery::GameEvent& e : events) {
        if (std::holds_alternative<gallery::event::RoundReset>(e)) {
            effects_.clear();   // a fresh round starts with a clean booth (G-32)
        } else if (std::holds_alternative<gallery::event::ShotFired>(e)) {
            effects_.onShot(muzzleNow());   // flash + recoil start this frame (G-9)
        } else if (const auto* h = std::get_if<gallery::event::DuckHit>(&e)) {
            effects_.onHit(h->aim, h->points, h->kind);
        } else if (const auto* m = std::get_if<gallery::event::ShotMissed>(&e)) {
            effects_.onMiss(m->aim);
        }
    }
}

void Renderer::update(Seconds dt, bool effectsRun) {
    time_ += dt;
    if (!mountStarted_) {
        mount_.reset(gallery::rifleHeadingToward(aim_));
        mountStarted_ = true;
    }
    mount_.update(dt, gallery::rifleHeadingToward(aim_));
    noteAge_ += dt;
    if (effectsRun) {
        effects_.update(dt);
    }
}

void Renderer::showNote(std::string text) {
    note_ = std::move(text);
    noteAge_ = 0.0;
}

float Renderer::lightLevel() const noexcept {
    return flicker_ ? gallery::ambientLight(time_) : 1.0f;
}

void Renderer::toggleGroup(LayerGroup group) {
    auto& on = groupsOn_[static_cast<std::size_t>(group)];
    on = !on;
    log_.info(std::format("Layer {} ({}) {}", static_cast<int>(group) + 1,
                          gallery::layerGroupName(group), on ? "ON" : "OFF"));
}

void Renderer::togglePostProcess() {
    postEnabled_ = !postEnabled_;
    log_.info(std::format("Post-process {}", postEnabled_ ? "ON" : "OFF (plain blit)"));
}

gallery::RiflePose Renderer::rifleNow() const noexcept {
    // The stiff mount's heading (D-039), not the raw aim: the gun swings
    // after the reticle like a heavy rifle on a swivel.
    return {mount_.heading(), effects_.recoil()};
}

Vec2 Renderer::muzzleNow() const noexcept {
    return gallery::rifleImageToLogical(L::kRifleMuzzleInImage, rifleNow());
}

void Renderer::draw(sf::RenderWindow& window, const gallery::Viewport& viewport,
                    const gallery::Game& game, const FrameInfo& info) {
    game_ = &game;
    round_ = &game.round();
    lane_ = &game.round().lane();
    const gallery::AppState state = game.state();
    pixelScale_ = viewport.scale();
    window.clear(kLetterbox);

    const Rect px = viewport.pixelRect();
    const sf::Vector2u size{static_cast<unsigned>(std::lround(px.width)),
                            static_cast<unsigned>(std::lround(px.height))};
    if (!post_.prepare(size)) {
        return;
    }

    // World layers 1-6 and world effects -> offscreen, post-processed.
    sf::RenderTarget& world = post_.target();
    for (const gallery::PassInfo& p : gallery::kDrawPasses) {
        if (p.postProcessed && groupOn(p.group)) {
            drawPass(world, p.pass);
        }
    }
    // The booth's light: incandescent sag/flicker times the warm lift of a
    // shot (D-031). Tungsten-warm: red lifts most, blue least.
    const float lightNow = lightLevel();
    const float flash = gallery::muzzleFlash(effects_.sinceShot());
    const sf::Vector3f light{lightNow * (1.0f + 0.070f * flash), lightNow * (1.0f + 0.052f * flash),
                             lightNow * (1.0f + 0.025f * flash)};
    post_.present(window, px, time_, postEnabled_, light);

    // Interface layer: logical camera on the letterboxed canvas, no post-process.
    const gallery::Size win = viewport.window();
    sf::View view(sf::FloatRect({0.0f, 0.0f},
                                {gallery::kLogicalSize.width, gallery::kLogicalSize.height}));
    view.setViewport(sf::FloatRect({std::round(px.x) / win.width, std::round(px.y) / win.height},
                                   {static_cast<float>(size.x) / win.width,
                                    static_cast<float>(size.y) / win.height}));
    window.setView(view);
    for (const gallery::PassInfo& p : gallery::kDrawPasses) {
        if (!p.postProcessed && groupOn(p.group) && p.pass != DrawPass::Crosshair) {
            drawPass(window, p.pass);
        }
    }
    if (groupOn(LayerGroup::Effects)) {
        drawHitTags(window);   // "+10" pop-ups: crisp, above the scene, below the reticle
    }
    if (groupOn(LayerGroup::Interface)) {
        if (state == gallery::AppState::Playing) {
            screens_.drawCue(window, *round_, pixelScale_);   // Ready / Go! / Time!
        }
        // Title, pause and results boards over the dimmed booth.
        screens_.drawOverlay(window, game, pixelScale_);
    }
    drawNote(window);
    if (inspect_ != InspectMode::Off) {
        drawInspectOverlay(window, viewport, info);
    } else if (info.showHint && state == gallery::AppState::Title) {
        drawHint(window);
    }
    // The reticle exists only while a round is being played; menus use the
    // system pointer (brief §5 controls).
    if (groupOn(LayerGroup::Interface) && state == gallery::AppState::Playing) {
        drawPass(window, DrawPass::Crosshair);   // always the very top
    }
}

void Renderer::drawPass(sf::RenderTarget& t, DrawPass pass) {
    switch (pass) {
    case DrawPass::FarPlate:
        if (const sf::Texture* tex = assets_.texture(AssetId::BgFar)) {
            drawPlaced(t, *tex, gallery::placeImage(sizeOf(*tex), L::kFarPlate, Fit::Cover));
        } else {
            standIn_.sky(t, L::kFarPlate);
        }
        break;
    case DrawPass::MidPlate:
        if (const sf::Texture* tex = assets_.texture(AssetId::BgMid)) {
            drawPlaced(t, *tex, gallery::placeImage(sizeOf(*tex), L::kMidPlate, Fit::Cover));
        } else {
            standIn_.midground(t, L::kMidPlate);
        }
        break;
    case DrawPass::Backboard:
        if (const sf::Texture* tex = assets_.texture(AssetId::FgBackboard)) {
            drawPlaced(t, *tex, gallery::placeImage(sizeOf(*tex), L::kBackboard, Fit::Cover));
        } else {
            standIn_.backboard(t, L::kBackboard);
        }
        break;
    case DrawPass::MissMarks:
        drawMissMarks(t);
        break;
    case DrawPass::TrackRail:
        drawTrackRail(t);
        break;
    case DrawPass::Ducks:
        drawDucks(t);
        break;
    case DrawPass::WaveStrip:
        if (const sf::Texture* tex = assets_.texture(AssetId::FgWaveStrip)) {
            drawPlaced(t, *tex, gallery::placeImage(sizeOf(*tex), L::kWaveStrip, Fit::FitWidth));
        } else {
            standIn_.waveStrip(t, L::kWaveStrip);
        }
        break;
    case DrawPass::BoothFrame:
        if (const sf::Texture* tex = assets_.texture(AssetId::FgBoothFrame)) {
            drawPlaced(t, *tex, gallery::placeImage(sizeOf(*tex), L::kBoothFrame, Fit::Cover));
        } else {
            standIn_.boothFrame(t);
        }
        break;
    case DrawPass::Bunting:
        drawBunting(t);
        break;
    case DrawPass::Sign:
        // Drawn only when its sprite exists (D-042: no blank card; D-044:
        // the baked sign carries the booth's name).
        if (const sf::Texture* tex = assets_.texture(AssetId::DecSignHeader)) {
            drawPlaced(t, *tex, gallery::placeImage(sizeOf(*tex), L::kSignSlot, Fit::Contain));
        }
        break;
    case DrawPass::Bulbs:
        drawBulbs(t);
        break;
    case DrawPass::Counter:
        if (const sf::Texture* tex = assets_.texture(AssetId::FgCounter)) {
            drawPlaced(t, *tex, gallery::placeImage(sizeOf(*tex), L::kCounter, Fit::FitWidth));
        } else {
            standIn_.counter(t, L::kCounter);
        }
        break;
    case DrawPass::Rifle:
        drawRifle(t);
        break;
    case DrawPass::MuzzlePuff:
        drawMuzzlePuff(t);
        break;
    case DrawPass::ScoreBoard:
        drawBoard(t, AssetId::UiScoreBoard, L::kScoreBoardSlot, "SCORE",
                  std::format("{:03}", round_ ? round_->stats().score() : 0),
                  render::theme::kInk);
        drawShotTally(t);
        break;
    case DrawPass::TimerBoard:
        drawTimerBoard(t);
        drawAmmo(t);
        break;
    case DrawPass::Crosshair:
        drawCrosshair(t);
        break;
    case DrawPass::Count:
        break;
    }
}

void Renderer::drawTrackRail(sf::RenderTarget& t) {
    const Rect r = L::kTrackRail;
    if (const sf::Texture* tex = assets_.texture(AssetId::FgTrackRail)) {
        // Tile horizontally at the rail's height; uniform scale.
        const float scale = r.height / static_cast<float>(tex->getSize().y);
        sf::Sprite sprite(*tex, sf::IntRect({0, 0}, {static_cast<int>(std::ceil(r.width / scale)),
                                                     static_cast<int>(tex->getSize().y)}));
        sprite.setScale({scale, scale});
        sprite.setPosition({r.x, r.y});
        t.draw(sprite);
    } else {
        standIn_.trackRail(t, r);
    }
}

void Renderer::drawDucks(sf::RenderTarget& t) {
    if (!lane_) {
        return;
    }
    // A soft contact shadow from the header bulbs above-front: the silhouette,
    // darkened, offset down onto the backboard, built from a few slightly
    // spread copies so its edge is soft without a blur pass.
    const std::array<Vec2, 4> spread{{{-1.5f, -1.0f}, {1.5f, -1.0f}, {-1.0f, 1.5f}, {1.0f, 1.5f}}};
    const sf::Color shade = withAlpha(sf::Color(12, 8, 4), L::kDuckShadowAlpha / 2.2f);

    // Back to front: later targets are drawn in front (hitTest agrees).
    for (std::size_t i = 0; i < lane_->size(); ++i) {
        const gallery::DuckPose pose = lane_->pose(i);
        // THE transform: the same function hit testing inverts (R-3).
        const sf::Transform xf = toSf(gallery::duckImageToLogical(pose, lane_->shape(i)));
        sf::Sprite sprite(ducks_.image(lane_->config(i).variant));
        for (const Vec2 s : spread) {
            sf::Transform shadow;
            shadow.translate(toSf(L::kDuckShadowOffset + s));
            sprite.setColor(shade);
            t.draw(sprite, sf::RenderStates(shadow * xf));
        }
        // D-035: decoys painted red, the power-up target gold: a tint over
        // the one duck sprite.
        switch (lane_->kind(i)) {
        case gallery::TargetKind::Duck: sprite.setColor(sf::Color::White); break;
        case gallery::TargetKind::Decoy: sprite.setColor(kDecoyTint); break;
        case gallery::TargetKind::Golden: sprite.setColor(kGoldenTint); break;
        }
        t.draw(sprite, sf::RenderStates(xf));
    }
}

void Renderer::drawTargetOverlay(sf::RenderTarget& t) {
    if (!lane_) {
        return;
    }
    const sf::Color accent = groupColour(LayerGroup::Targets);
    const sf::Color live(90, 255, 120);
    const sf::Color dead(255, 90, 70);
    // Exactly what a shot would hit: scenery in front of the lane included.
    const std::optional<std::size_t> under = round_->targetUnder(aim_);

    for (std::size_t i = 0; i < lane_->size(); ++i) {
        const gallery::DuckConfig& c = lane_->config(i);
        const gallery::DuckPose pose = lane_->pose(i);
        const gallery::DuckShape& shape = lane_->shape(i);
        const gallery::MotionSample m = lane_->motion(i);

        // Movement bounds: a bracket under the track line.
        const float top = c.trackY - shape.size.height - 8.0f;
        const float y = c.trackY + 10.0f;
        const float x0 = c.boundsMin;
        const float x1 = c.mode == gallery::MotionMode::Wrap ? c.boundsMax + shape.size.width
                                                              : c.boundsMax;
        fillRect(t, {x0, y, x1 - x0, 2.0f}, withAlpha(accent, 0.9f));
        fillRect(t, {x0, top, 2.0f, y - top + 2.0f}, withAlpha(accent, 0.6f));
        fillRect(t, {x1 - 2.0f, top, 2.0f, y - top + 2.0f}, withAlpha(accent, 0.6f));

        // Hit region: the mask itself, posed exactly like the sprite.
        const sf::Transform xf = toSf(gallery::duckImageToLogical(pose, shape));
        sf::Sprite mask(ducks_.maskImage(c.variant));
        const bool hover = under == i;
        mask.setColor(hover ? withAlpha(sf::Color(255, 255, 255), 0.75f)
                            : withAlpha(pose.hittable ? live : dead, pose.hittable ? 0.42f : 0.28f));
        t.draw(mask, sf::RenderStates(xf));

        // Posed bounding quad and hinge.
        const gallery::Quad q = lane_->quad(i);
        sf::VertexArray box(sf::PrimitiveType::LineStrip, 5);
        for (std::size_t k = 0; k < 5; ++k) {
            box[k].position = toSf(q.p[k % 4]);
            box[k].color = pose.hittable ? live : dead;
        }
        t.draw(box);
        fillRect(t, {pose.hinge.x - 3.0f, pose.hinge.y - 3.0f + pose.sink, 6.0f, 6.0f}, accent);

        sf::Text label = crispText(
            assets_.utilityFont(),
            std::format("#{} {}{}\n{:+.0f} u/s {}", i, gallery::duckStateName(lane_->state(i)),
                        hover ? "  <aim>" : "", m.velocity, m.direction > 0 ? "->" : "<-"),
            12.0f, pixelScale_);
        label.setFillColor(pose.hittable ? live : dead);
        label.setPosition({c.mode == gallery::MotionMode::Wrap ? m.left : x0 + 4.0f, top - 34.0f});
        t.draw(label);
    }
}

void Renderer::drawBulbs(sf::RenderTarget& t) {
    const sf::Texture* lit = assets_.texture(AssetId::DecBulbLit);
    const sf::Texture* glow = assets_.texture(AssetId::DecBulbGlow);
    for (std::size_t i = 0; i < L::kBulbCount; ++i) {
        const Vec2 socket = gallery::bulbPosition(i);
        const Rect r{socket.x - L::kBulbSize.width / 2.0f, socket.y, L::kBulbSize.width,
                     L::kBulbSize.height};
        // Each bulb's own filament, times the booth's supply (a generator
        // sag dims every bulb together). Steady with --no-flicker.
        const float g = flicker_ ? gallery::bulbGlow(i, time_) * lightLevel() : 1.0f;
        if (glow) {
            // The soft halo sprite, added on top of the scene.
            const float d = L::kBulbGlowDiameter;
            const Rect gr{socket.x - d / 2.0f, socket.y + r.height * 0.6f - d / 2.0f, d, d};
            drawPlaced(t, *glow, gallery::placeImage(sizeOf(*glow), gr, Fit::Contain),
                       withAlpha(sf::Color::White, 0.85f * g), sf::BlendAdd);
        }
        if (lit) {
            const auto v = static_cast<std::uint8_t>(std::lround(255.0f * (0.82f + 0.18f * g)));
            drawPlaced(t, *lit, gallery::placeImage(sizeOf(*lit), r, Fit::Contain),
                       sf::Color(v, v, v));
        } else {
            standIn_.bulb(t, r);
        }
    }
}

void Renderer::drawRifle(sf::RenderTarget& t) {
    const gallery::RiflePose pose = rifleNow();
    if (const sf::Texture* tex = assets_.texture(AssetId::FgRifle)) {
        drawRifleMesh(t, *tex, pose);
    } else {
        standIn_.rifle(t, [&](float along, float across) {
            return toSf(gallery::rifleAxisToLogical(along, across, pose));
        });
    }
}

void Renderer::drawMissMarks(sf::RenderTarget& t) {
    const sf::Texture* tex = assets_.texture(AssetId::OvMissMark);
    for (const auto& m : effects_.marks()) {
        if (m.age >= EffectsState::kMissMarkLife) {
            continue;
        }
        const float alpha = 1.0f - smoothstep(1.4f, 2.0f, static_cast<float>(m.age));
        // Impact: the mark lands slightly large and settles over ~100 ms.
        const float pop = 1.0f + 0.7f * (1.0f - smoothstep(0.0f, static_cast<float>(
                                                             EffectsState::kMarkPopTime),
                                                         static_cast<float>(m.age)));
        const float s = L::kMissMarkSize * pop;
        const Rect r{m.pos.x - s / 2.0f, m.pos.y - s / 2.0f, s, s};
        if (tex) {
            drawPlaced(t, *tex, gallery::placeImage(sizeOf(*tex), r, Fit::Contain),
                       withAlpha(sf::Color::White, alpha));
        } else {
            standIn_.missMark(t, m.pos, 6.0f * pop, alpha);
        }
    }
}

void Renderer::drawMuzzlePuff(sf::RenderTarget& t) {
    // Photographed flash (E1, OV_MUZZLE_FLASH), only if one exists: a few
    // frames, additive, at the muzzle. Without it the flash is the exposure
    // lift in the post-process alone; nothing is drawn in its place (D-026).
    const float flash = gallery::muzzleFlash(effects_.sinceShot());
    if (flash > 0.0f) {
        if (const sf::Texture* tex = assets_.texture(AssetId::OvMuzzleFlash)) {
            const float s = L::kMuzzlePuffSize * 0.55f;
            const Vec2 c = effects_.puffPos();
            drawPlaced(t, *tex,
                       gallery::placeImage(sizeOf(*tex), {c.x - s / 2.0f, c.y - s / 2.0f, s, s},
                                           Fit::Contain),
                       withAlpha(sf::Color::White, flash), sf::BlendAdd);
        }
    }

    const Seconds age = effects_.puffAge();
    if (age >= EffectsState::kPuffLife) {
        return;
    }
    const float k = static_cast<float>(age / EffectsState::kPuffLife);   // 0 -> 1
    const float size = L::kMuzzlePuffSize * (0.45f + 0.55f * std::sqrt(k));
    const float alpha = std::pow(1.0f - k, 1.6f);
    // The puff drifts up and slightly into the scene as it expands.
    const Vec2 c = effects_.puffPos() + Vec2{-18.0f * k, -26.0f * k};
    const Rect r{c.x - size / 2.0f, c.y - size / 2.0f, size, size};
    if (const sf::Texture* tex = assets_.texture(AssetId::OvMuzzlePuff)) {
        // The sprite is a translucent grey disc (D-044), blended normally.
        drawPlaced(t, *tex, gallery::placeImage(sizeOf(*tex), r, Fit::Contain),
                   withAlpha(sf::Color::White, alpha));
    } else {
        standIn_.muzzlePuff(t, r, alpha);
    }
}

void Renderer::drawBoard(sf::RenderTarget& t, AssetId id, Rect slot, const char* caption,
                         const std::string& value, sf::Color valueInk) {
    namespace th = render::theme;
    const sf::Texture* tex = assets_.texture(id);
    const gallery::Size spec = id == AssetId::UiScoreBoard ? gallery::Size{420.0f, 180.0f}
                                                           : gallery::Size{300.0f, 180.0f};
    const auto p = gallery::placeImage(tex ? sizeOf(*tex) : spec, slot, Fit::Contain);
    if (tex) {
        drawPlaced(t, *tex, p);
    } else {
        standIn_.board(t, p.dest);
    }
    // Lettering painted at runtime with the display font (Asset Guide §4.1),
    // from the shared theme so the boards match the screens.
    const float cx = p.dest.x + p.dest.width / 2.0f;
    th::letter(t, assets_.font(), caption, th::kSizeCaption, {cx, p.dest.y + 18.0f},
               th::Align::Centre, th::kInk, pixelScale_, th::kCaptionSpacing);
    th::letter(t, assets_.font(), value, th::kSizeNumeral, {cx, p.dest.y + 52.0f},
               th::Align::Centre, valueInk, pixelScale_);
}

void Renderer::drawTimerBoard(sf::RenderTarget& t) {
    namespace th = render::theme;
    const gallery::RoundClock* clock = round_ ? &round_->clock() : nullptr;
    const int secs = clock ? clock->displaySeconds() : 60;
    const bool low = clock && clock->lowTime();
    drawBoard(t, AssetId::UiTimerBoard, L::kTimerBoardSlot, "TIME",
              std::format("{}:{:02}", secs / 60, secs % 60), low ? th::kRed : th::kInk);

    // Illuminated indicator on the board's corner: steady while the clock
    // runs, flashing once a second in the final ten (G-27), dark otherwise.
    // Its flash follows the round clock, so it freezes with a pause.
    bool lit = false;
    if (clock && clock->phase() == gallery::RoundPhase::Live &&
        game_ && game_->state() == gallery::AppState::Playing) {
        lit = !low || std::fmod(clock->remaining(), 1.0) >= 0.5;
    }
    const Rect s = L::kTimerBoardSlot;
    drawLamp(t, {s.x + s.width - 30.0f, s.y + 30.0f}, 18.0f, lit);
}

void Renderer::drawAmmo(sf::RenderTarget& t) {
    // The magazine as a row of brass pellets on a small board on the counter
    // (D-035). Spent rounds are empty outlines; while reloading they fill in
    // left to right; during the golden power-up every round is lit.
    namespace th = render::theme;
    if (!round_ || round_->tuning().magazine <= 0) {
        return;
    }
    const gallery::Round& r = *round_;
    const int mag = r.tuning().magazine;
    const bool freeAmmo = r.freeAmmoLeft() > 0.0;
    const int shown = freeAmmo           ? mag
                      : r.reloading()  ? static_cast<int>(std::floor(r.reloadProgress() * mag))
                                       : r.ammo();
    const Rect s = L::kAmmoSlot;
    const float cx = s.x + s.width / 2.0f;
    fillRect(t, s, standin::kBoard);

    const std::string caption =
        freeAmmo ? std::format("FREE AMMO  {}", static_cast<int>(std::ceil(r.freeAmmoLeft()))) : "AMMO";
    th::letter(t, assets_.font(), caption, th::kSizeCaption, {cx, s.y + 10.0f}, th::Align::Centre,
               freeAmmo ? th::kRed : th::kInk, pixelScale_, th::kCaptionSpacing);

    constexpr float kW = 16.0f;
    constexpr float kH = 40.0f;
    constexpr float kPitch = 36.0f;
    const float x0 = cx - (static_cast<float>(mag - 1) * kPitch + kW) / 2.0f;
    const float y0 = s.y + 40.0f;
    for (int i = 0; i < mag; ++i) {
        const Rect body{x0 + static_cast<float>(i) * kPitch, y0, kW, kH};
        if (i < shown) {
            fillRect(t, body, freeAmmo ? sf::Color(255, 214, 90) : th::kGold);   // brass case
            fillRect(t, {body.x + 3.0f, body.y - 6.0f, kW - 6.0f, 8.0f}, sf::Color(120, 104, 92));   // pellet
            outline(t, body, th::kInk, 1.5f);
        } else {
            outline(t, body, withAlpha(th::kInk, 0.45f), 1.5f);   // spent
        }
    }

    const char* status = nullptr;
    sf::Color ink = th::kInk;
    if (!freeAmmo && r.reloading()) {
        status = "RELOADING";
    } else if (!freeAmmo && r.ammo() <= 0 && game_ && game_->state() == gallery::AppState::Playing) {
        status = "RELOAD:  R  or  RIGHT-CLICK";
        ink = th::kRed;
    }
    if (status) {
        th::letter(t, assets_.font(), status, th::kSizeSmall, {cx, s.y + s.height - 24.0f},
                   th::Align::Centre, ink, pixelScale_, 1.1f);
    }
}

void Renderer::drawCrosshair(sf::RenderTarget& t) {
    // Shot feedback (G-9): the reticle opens up briefly on every shot and
    // warms on a hit. Its centre never moves: the aim point stays exact.
    const float kick = effects_.kick();
    const float flash = effects_.hitFlash();
    const float s = L::kReticleSize * (1.0f + 0.22f * kick);
    const sf::Color hitColour(255, 196, 64);
    auto mix = [&](sf::Color a, sf::Color b, float k) {
        auto ch = [&](std::uint8_t x, std::uint8_t y) {
            return static_cast<std::uint8_t>(std::lround(x + (y - x) * k));
        };
        return sf::Color(ch(a.r, b.r), ch(a.g, b.g), ch(a.b, b.b), a.a);
    };
    if (const sf::Texture* tex = assets_.texture(AssetId::UiReticle)) {
        drawPlaced(t, *tex,
                   gallery::placeImage(sizeOf(*tex), {aim_.x - s / 2.0f, aim_.y - s / 2.0f, s, s},
                                       Fit::Contain),
                   mix(sf::Color::White, hitColour, flash));
        return;
    }
    standIn_.reticle(t, aim_, s, kick, mix(sf::Color(232, 228, 218), hitColour, flash));
}

std::string Renderer::shotSummary() const {
    if (!round_) {
        return {};
    }
    const gallery::RoundStats& s = round_->stats();
    const auto acc = s.accuracyPercent();
    const Seconds wait = round_->rifle().remaining(round_->time());
    return std::format("SHOTS {}  hits {}  misses {}  score {}  accuracy {}  rifle {}\n", s.shots(),
                       s.hits(), s.misses(), s.score(),
                       acc ? std::format("{}%", *acc) : std::string("—"),
                       wait > 0.0 ? std::format("cooling {:.0f} ms", wait * 1000.0)
                                  : std::string("ready"));
}

void Renderer::drawHitTags(sf::RenderTarget& t) {
    // "+10" where the shot landed, rising and fading (brief G-17). Runtime
    // lettering, like the HUD numerals; the painted tag photo (OV_HIT_TAG) is E1.
    for (const auto& g : effects_.tags()) {
        if (g.age >= EffectsState::kTagLife) {
            continue;
        }
        const float k = static_cast<float>(g.age / EffectsState::kTagLife);   // 0 -> 1
        const float alpha = 1.0f - smoothstep(0.55f, 1.0f, k);
        const float grow = 0.85f + 0.15f * smoothstep(0.0f, 0.15f, k);
        const std::string label = g.kind == gallery::TargetKind::Golden
                                      ? std::format("{:+}  FREE AMMO", g.points)
                                      : std::format("{:+}", g.points);
        sf::Text tag = crispText(assets_.font(), label, 34.0f * grow, pixelScale_);
        tag.setStyle(sf::Text::Bold);
        const sf::FloatRect b = tag.getLocalBounds();
        tag.setOrigin({b.position.x + b.size.x / 2.0f, b.position.y + b.size.y});
        tag.setPosition({g.pos.x, g.pos.y - 24.0f - 46.0f * std::sqrt(k)});
        const sf::Color fill = g.points < 0 ? sf::Color(255, 120, 100)
                               : g.kind == gallery::TargetKind::Golden ? sf::Color(255, 214, 90)
                                                                       : sf::Color(255, 226, 150);
        tag.setFillColor(withAlpha(fill, alpha));
        tag.setOutlineColor(withAlpha(sf::Color(40, 24, 10), alpha));
        tag.setOutlineThickness(2.0f);
        t.draw(tag);
    }
}

void Renderer::drawShotTally(sf::RenderTarget& t) {
    if (!round_) {
        return;
    }
    // Shots, hits, misses and accuracy under the score (G-22, G-23: "—" at 0 shots).
    const gallery::RoundStats& s = round_->stats();
    const auto acc = s.accuracyPercent();
    sf::Text line = crispText(
        assets_.font(),
        std::format("HITS {}     MISSES {}     ACC {}", s.hits(), s.misses(),
                    acc ? std::format("{}%", *acc) : std::string("—")),
        render::theme::kSizeSmall, pixelScale_);
    line.setFillColor(render::theme::kInk);
    const sf::FloatRect b = line.getLocalBounds();
    line.setOrigin({b.position.x + b.size.x / 2.0f, b.position.y});
    line.setPosition({L::kScoreBoardSlot.x + L::kScoreBoardSlot.width / 2.0f,
                      L::kScoreBoardSlot.y + L::kScoreBoardSlot.height - 38.0f});
    t.draw(line);
}

void Renderer::drawBunting(sf::RenderTarget& t) {
    // E1: drawn only when the photograph exists; no stand-in. It stirs in the
    // evening air (D-031): under half a degree about its top edge, so it can
    // never be mistaken for a target or pull the eye from the lane.
    const sf::Texture* tex = assets_.texture(AssetId::DecBunting);
    if (!tex) {
        return;
    }
    const gallery::Sway sway = flicker_ ? gallery::buntingSway(time_) : gallery::Sway{0.0f, 0.0f};
    const sf::Vector2f pivot{L::kBuntingSlot.x + L::kBuntingSlot.width / 2.0f, L::kBuntingSlot.y};
    sf::Transform xf;
    xf.translate({sway.dx, 0.0f});
    xf.rotate(sf::degrees(sway.angleDeg), pivot);
    const auto placed = gallery::placeImage(sizeOf(*tex), L::kBuntingSlot, Fit::FitWidth);
    sf::Sprite sprite(*tex);
    sprite.setPosition(toSf(Vec2{placed.dest.x, placed.dest.y}));
    sprite.setScale({placed.dest.width / static_cast<float>(tex->getSize().x),
                     placed.dest.height / static_cast<float>(tex->getSize().y)});
    t.draw(sprite, sf::RenderStates(xf));
}

void Renderer::drawNote(sf::RenderTarget& t) {
    // Volume / mute feedback: small painted lettering low on the counter,
    // clear of the lane and the HUD boards, gone after about 1.6 s.
    constexpr float kHold = 1.1f;
    constexpr float kFade = 0.5f;
    if (note_.empty() || noteAge_ >= kHold + kFade) {
        return;
    }
    namespace th = render::theme;
    const float alpha = 1.0f - smoothstep(kHold, kHold + kFade, static_cast<float>(noteAge_));
    const float w = th::letterWidth(assets_.font(), note_, th::kSizeBody, pixelScale_) + 44.0f;
    const Rect plate{960.0f - w / 2.0f, 1010.0f, w, 46.0f};
    fillRect(t, plate, withAlpha(th::kCream, 0.92f * alpha));
    fillRect(t, {plate.x + 4.0f, plate.y + 4.0f, plate.width - 8.0f, 2.0f}, withAlpha(th::kInk, alpha));
    fillRect(t, {plate.x + 4.0f, plate.y + plate.height - 6.0f, plate.width - 8.0f, 2.0f},
             withAlpha(th::kInk, alpha));
    th::letter(t, assets_.font(), note_, th::kSizeBody, {960.0f, plate.y + 11.0f}, th::Align::Centre,
               withAlpha(th::kInk, alpha), pixelScale_);
}

void Renderer::drawHint(sf::RenderTarget& t) {
    const float alpha = 1.0f - smoothstep(8.0f, 10.0f, static_cast<float>(time_));
    if (alpha <= 0.0f) {
        return;
    }
    sf::Text hint = crispText(assets_.utilityFont(),
                              "Debug keys:   F3 inspect   ·   1–8 toggle layers   ·   F4 post-process"
                              "   ·   K knock down a target",
                              20.0f, pixelScale_);
    const sf::FloatRect b = hint.getLocalBounds();
    hint.setOrigin({b.position.x + b.size.x / 2.0f, b.position.y + b.size.y / 2.0f});
    hint.setPosition({960.0f, 1046.0f});
    const sf::FloatRect g = hint.getGlobalBounds();   // logical units
    fillRect(t, {g.position.x - 16.0f, g.position.y - 9.0f, g.size.x + 32.0f, g.size.y + 18.0f},
             withAlpha(sf::Color(0, 0, 0, 150), alpha));
    hint.setFillColor(withAlpha(kOverlayText, alpha));
    t.draw(hint);
}

void Renderer::drawInspectOverlay(sf::RenderTarget& t, const gallery::Viewport& viewport,
                                  const FrameInfo& info) {
    // Targets: bounds, hit regions, states. Shown in both inspect modes.
    drawTargetOverlay(t);
    if (lane_ && inspect_ == InspectMode::Lane) {
        std::string tt = std::format(
            "TARGETS  {} preset · {} reaction · {} · {} · lane {:.2f} s · image: {} · aim: {}\n",
            info.lanePreset, info.reaction, gallery::appStateName(game_->state()),
            gallery::roundPhaseName(round_->clock().phase()), lane_->time(),
            ducks_.isStandIn(0) ? "stand-in (TGT_DUCK_A missing)" : "photograph",
            [&] {
                if (round_->foreground().covers(aim_)) {
                    return std::string("scenery in front (a shot here misses)");
                }
                const auto under = round_->targetUnder(aim_);
                return under ? std::format("over #{} hit region", *under) : std::string("no target");
            }());
        tt += shotSummary();
        for (std::size_t i = 0; i < lane_->size(); ++i) {
            const gallery::DuckConfig& c = lane_->config(i);
            const gallery::MotionSample m = lane_->motion(i);
            tt += std::format("  #{} {:<7} {:>5.2f}s  x {:7.1f}  v {:+6.1f}  bounds [{:.0f}, {:.0f}]  "
                              "speed {:.0f}  edges {}\n",
                              i, gallery::duckStateName(lane_->state(i)), lane_->stateTime(i),
                              m.left, m.velocity, c.boundsMin, c.boundsMax, c.speed, m.edges);
        }
        sf::Text panel = crispText(assets_.utilityFont(), tt, 13.0f, pixelScale_);
        panel.setFillColor(kOverlayText);
        panel.setPosition({244.0f, 738.0f});
        const sf::FloatRect g = panel.getGlobalBounds();
        fillRect(t, {230.0f, 730.0f, g.size.x + 28.0f, g.position.y + g.size.y - 730.0f + 8.0f},
                 sf::Color(0, 0, 0, 190));
        t.draw(panel);
    }
    if (inspect_ != InspectMode::Full) {
        return;
    }

    // Lane band and horizon.
    fillRect(t, L::kLaneBand, sf::Color(255, 220, 0, 22));
    outline(t, L::kLaneBand, sf::Color(255, 220, 0, 150), 1.5f);
    fillRect(t, {0.0f, L::kHorizonY - 1.0f, 1920.0f, 2.0f}, sf::Color(0, 200, 255, 200));

    struct Slot {
        Rect r;
        LayerGroup g;
        const char* name;
    };
    const Slot slots[] = {
        {L::kOpening, LayerGroup::Environment, "opening / FG_BACKBOARD"},
        {L::kLeftPost, LayerGroup::Environment, "post"},
        {L::kRightPost, LayerGroup::Environment, "post"},
        {L::kHeaderBeam, LayerGroup::Environment, "header beam"},
        {L::kSignSlot, LayerGroup::Decorations, "DEC_SIGN_HEADER"},
        {L::kBuntingSlot, LayerGroup::Decorations, "DEC_BUNTING (E1)"},
        {L::kTrackRail, LayerGroup::Track, "FG_TRACK_RAIL"},
        {L::kWaveStrip, LayerGroup::Track, "FG_WAVE_STRIP"},
        {L::kCounter, LayerGroup::Counter, "FG_COUNTER"},
        {gallery::rifleBounds(rifleNow()), LayerGroup::Rifle, "FG_RIFLE bounds"},
        {L::kScoreBoardSlot, LayerGroup::Interface, "UI_SCORE_BOARD"},
        {L::kTimerBoardSlot, LayerGroup::Interface, "UI_TIMER_BOARD"},
        {L::kMissMarkRegion, LayerGroup::Effects, "miss-mark region"},
    };
    for (const Slot& s : slots) {
        const sf::Color c = groupColour(s.g);
        outline(t, s.r, withAlpha(c, groupOn(s.g) ? 0.9f : 0.3f), 2.0f);
        sf::Text n = crispText(assets_.utilityFont(), s.name, 12.0f, pixelScale_);
        n.setFillColor(c);
        n.setPosition({s.r.x + 4.0f, s.r.y + s.r.height - 18.0f});
        t.draw(n);
    }
    // Screen board and its clickable placards (the same rects the game hit-tests).
    if (const auto buttons = gallery::ui::buttons(game_->state()); !buttons.empty()) {
        const sf::Color c = groupColour(LayerGroup::Interface);
        outline(t, gallery::ui::kPanel, withAlpha(c, 0.9f), 2.0f);
        for (const gallery::Button& b : buttons) {
            outline(t, b.rect, withAlpha(c, 0.9f), 1.5f);
        }
    }
    const Vec2 pivot = L::kRiflePivot;
    const Vec2 muzzle = muzzleNow();
    fillRect(t, {pivot.x - 4.0f, pivot.y - 4.0f, 8.0f, 8.0f}, groupColour(LayerGroup::Rifle));
    fillRect(t, {muzzle.x - 4.0f, muzzle.y - 4.0f, 8.0f, 8.0f}, groupColour(LayerGroup::Effects));

    // Info panel.
    std::string text = "INSPECT (F3)\n\nLayers (key):\n";
    for (std::size_t g = 0; g < gallery::kLayerGroupCount; ++g) {
        const auto group = static_cast<LayerGroup>(g);
        text += std::format("  {}  {:<24} {}\n", g + 1, gallery::layerGroupName(group),
                            groupOn(group) ? "on" : "OFF");
    }
    const Rect px = viewport.pixelRect();
    text += std::format("\nPost-process (F4): {}{}\n", postEnabled_ ? "on" : "OFF",
                        post_.shaderAvailable() ? "" : " (no shader: plain blit)");
    text += std::format("  grain: {}\n", post_.grainAvailable() ? "scanned plate" : "off (OV_GRAIN missing)");
    text += "\nTargets: press F3 again for the target view (table, no panel); K: debug knock-down\n";
    text += shotSummary();
    text += std::format("\nWindow {:.0f}x{:.0f}  canvas {:.0f}x{:.0f} @ ({:.0f},{:.0f})\n",
                        viewport.window().width, viewport.window().height, px.width, px.height,
                        px.x, px.y);
    text += std::format("Scale {:.3f} px/unit   target {}x{}\n", viewport.scale(), post_.size().x,
                        post_.size().y);
    text += std::format("Aim {:.0f}, {:.0f}   dt {:.1f} ms   {}\n", aim_.x, aim_.y,
                        info.dt * 1000.0, info.focused ? "focused" : "unfocused");
    text += std::format("\nAssets: {} loaded, {} missing{}\n", assets_.loadedCount(),
                        assets_.missing().size(),
                        assets_.fontIsStandIn() ? "  (font: Windows system)" : "");
    std::size_t col = 0;
    for (const AssetId id : assets_.missing()) {
        text += std::format("{}{:<16}", col == 0 ? "  " : " ", gallery::assetEntry(id).key);
        if (++col == 3) {
            text += "\n";
            col = 0;
        }
    }

    sf::Text panel = crispText(assets_.utilityFont(), text, 14.0f, pixelScale_);
    panel.setFillColor(kOverlayText);
    panel.setLineSpacing(1.05f);
    panel.setPosition({244.0f, 312.0f});
    const sf::FloatRect g = panel.getGlobalBounds();   // logical units
    fillRect(t, {230.0f, 300.0f, g.size.x + 28.0f, g.position.y + g.size.y - 300.0f + 14.0f},
             sf::Color(0, 0, 0, 190));
    t.draw(panel);
}

} // namespace shell::render
