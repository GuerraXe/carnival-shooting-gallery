#include "shell/render/StandIn.hpp"

#include "gallery/view/SceneLayout.hpp"
#include "shell/render/DrawUtil.hpp"
#include "shell/render/Theme.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <numbers>
#include <vector>

namespace shell::render {

using gallery::Rect;
using gallery::Vec2;
namespace L = gallery::layout;

namespace {

void polygon(sf::RenderTarget& t, std::initializer_list<sf::Vector2f> pts, sf::Color fill,
             const sf::RenderStates& states = sf::RenderStates::Default) {
    sf::ConvexShape shape(pts.size());
    std::size_t i = 0;
    for (const auto& p : pts) {
        shape.setPoint(i++, p);
    }
    shape.setFillColor(fill);
    t.draw(shape, states);
}

void ellipse(sf::RenderTarget& t, sf::Vector2f centre, sf::Vector2f radii, sf::Color fill,
             const sf::RenderStates& states) {
    sf::CircleShape c(1.0f, 40);
    c.setOrigin({1.0f, 1.0f});
    c.setScale(radii);
    c.setPosition(centre);
    c.setFillColor(fill);
    t.draw(c, states);
}

} // namespace

// ---------------------------------------------------------------------------
// Scenery
// ---------------------------------------------------------------------------

void StandInPainter::sky(sf::RenderTarget& t, Rect r) const {
    fillRect(t, r, standin::kSky);
}

void StandInPainter::midground(sf::RenderTarget& t, Rect r) const {
    // Midway skyline: the plate is transparent above the tent line. In the
    // 1960x1100 plate the tents start 46 units down and run past the bottom.
    const float k = r.height / L::kMidPlate.height;
    const float top = r.y + 46.0f * k;
    fillRect(t, {r.x, top, r.width, std::min(1060.0f * k, r.y + r.height - top)},
             standin::kMidground);
}

void StandInPainter::backboard(sf::RenderTarget& t, Rect r) const {
    fillRect(t, r, standin::kBackboard);
}

void StandInPainter::trackRail(sf::RenderTarget& t, Rect r) const {
    fillRect(t, r, standin::kRail);
}

void StandInPainter::boothFrame(sf::RenderTarget& t) const {
    // Posts, header beam, and the awning's scalloped edge: the opaque parts of
    // FG_BOOTH_FRAME. The opening and the outer slivers stay transparent.
    fillRect(t, L::kLeftPost, standin::kFrame);
    fillRect(t, L::kRightPost, standin::kFrame);
    fillRect(t, L::kHeaderBeam, standin::kFrame);
    fillRect(t, L::kAwning, standin::kAwning);
    const float scallop = 64.0f;
    for (float x = L::kAwning.x; x < L::kAwning.x + L::kAwning.width; x += scallop) {
        sf::CircleShape c(scallop / 2.0f, 24);
        c.setPosition({x, L::kAwning.y + L::kAwning.height - scallop / 2.0f});
        c.setFillColor(standin::kAwning);
        t.draw(c);
    }
}

void StandInPainter::waveStrip(sf::RenderTarget& t, Rect r) const {
    // Wave crests along the top edge; flat bottom.
    sf::VertexArray strip(sf::PrimitiveType::TriangleStrip);
    const float crest = L::kWaveTroughDepth;
    for (float x = r.x; x <= r.x + r.width + 0.5f; x += 12.0f) {
        const float phase = (x - r.x) / 190.0f * 2.0f * std::numbers::pi_v<float>;
        const float top = r.y + crest * (0.5f - 0.5f * std::cos(phase));
        strip.append(sf::Vertex{{x, top}, standin::kWave});
        strip.append(sf::Vertex{{x, r.y + r.height}, standin::kWave});
    }
    t.draw(strip);
}

void StandInPainter::counter(sf::RenderTarget& t, Rect r) const {
    // Top surface seen from 30-45 cm above, then the front face.
    const float top = 70.0f;
    fillRect(t, {r.x, r.y, r.width, top}, standin::kCounterTop);
    fillRect(t, {r.x, r.y + top, r.width, r.height - top}, standin::kCounterFront);
}

// ---------------------------------------------------------------------------
// Decorations
// ---------------------------------------------------------------------------

void StandInPainter::bulb(sf::RenderTarget& t, Rect r) const {
    // Dark socket at the top, round bulb below it (24x32 slot).
    const float kx = r.width / L::kBulbSize.width;
    const float ky = r.height / L::kBulbSize.height;
    const float cx = r.x + r.width / 2.0f;
    fillRect(t, {cx - 5.0f * kx, r.y, 10.0f * kx, 9.0f * ky}, standin::kFrame);
    sf::CircleShape globe(r.width / 2.0f, 24);
    globe.setPosition({r.x, r.y + 8.0f * ky});
    globe.setFillColor(standin::kBulb);
    t.draw(globe);
}

void StandInPainter::bulbGlow(sf::RenderTarget& t, Rect r) const {
    // A soft round halo, brightest at the centre. Drawn additively by the
    // renderer, so it lifts what is behind the bulb a little.
    const sf::Vector2f c{r.x + r.width / 2.0f, r.y + r.height / 2.0f};
    const float radius = std::min(r.width, r.height) / 2.0f;
    constexpr int kRings = 16;
    constexpr int kSegments = 64;
    auto alphaAt = [](float u) {   // u: 0 centre .. 1 edge
        const float f = 1.0f - u;
        return static_cast<std::uint8_t>(std::lround(255.0f * f * f));
    };
    sf::VertexArray tris(sf::PrimitiveType::Triangles);
    for (int ring = 0; ring < kRings; ++ring) {
        const float u0 = static_cast<float>(ring) / kRings;
        const float u1 = static_cast<float>(ring + 1) / kRings;
        sf::Color c0 = standin::kGlow;
        sf::Color c1 = standin::kGlow;
        c0.a = alphaAt(u0);
        c1.a = alphaAt(u1);
        for (int s = 0; s < kSegments; ++s) {
            const float a0 = 2.0f * std::numbers::pi_v<float> * static_cast<float>(s) / kSegments;
            const float a1 = 2.0f * std::numbers::pi_v<float> * static_cast<float>(s + 1) / kSegments;
            auto at = [&](float u, float a) {
                return sf::Vector2f{c.x + std::cos(a) * u * radius, c.y + std::sin(a) * u * radius};
            };
            const sf::Vertex p00{at(u0, a0), c0}, p01{at(u0, a1), c0};
            const sf::Vertex p10{at(u1, a0), c1}, p11{at(u1, a1), c1};
            tris.append(p00);
            tris.append(p10);
            tris.append(p11);
            tris.append(p00);
            tris.append(p11);
            tris.append(p01);
        }
    }
    t.draw(tris);
}

void StandInPainter::sign(sf::RenderTarget& t, Rect r, const sf::Font& font,
                          float pixelScale) const {
    // Painted header board on the beam: a plank with clipped corners, a thin
    // sign-writer's border, two bolt heads, and the booth's name.
    const float ch = 18.0f;   // corner clip
    polygon(t,
            {{r.x + ch, r.y},
             {r.x + r.width - ch, r.y},
             {r.x + r.width, r.y + ch},
             {r.x + r.width, r.y + r.height - ch},
             {r.x + r.width - ch, r.y + r.height},
             {r.x + ch, r.y + r.height},
             {r.x, r.y + r.height - ch},
             {r.x, r.y + ch}},
            standin::kBoard);
    sf::RectangleShape border({r.width - 32.0f, r.height - 32.0f});
    border.setPosition({r.x + 16.0f, r.y + 16.0f});
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineThickness(3.0f);
    border.setOutlineColor(standin::kMark);
    t.draw(border);
    for (const float bx : {r.x + 40.0f, r.x + r.width - 40.0f}) {
        sf::CircleShape bolt(7.0f, 20);
        bolt.setOrigin({7.0f, 7.0f});
        bolt.setPosition({bx, r.y + r.height / 2.0f});
        bolt.setFillColor(standin::kMark);
        t.draw(bolt);
    }

    constexpr const char* kName = "SHOOTING GALLERY";
    constexpr float kSpacing = 1.5f;
    float size = 74.0f;
    const float room = r.width - 150.0f;
    const float w = theme::letterWidth(font, kName, size, pixelScale, kSpacing);
    if (w > room) {
        size *= room / w;
    }
    // `letter` anchors at the cap height; Rockwell's caps are ~0.7 em.
    theme::letter(t, font, kName, size, {r.x + r.width / 2.0f, r.y + r.height / 2.0f - 0.36f * size},
                  theme::Align::Centre, theme::kInk, pixelScale, kSpacing);
}

void StandInPainter::bunting(sf::RenderTarget& t, Rect r) const {
    // A string of pennants hung between the bulb sockets along the beam, in
    // shallow swags, two flags per swag so the bulbs stay clear.
    const float kx = r.width / L::kBuntingSlot.width;
    const float ky = r.height / L::kBuntingSlot.height;
    auto X = [&](float slotX) { return r.x + (slotX - L::kBuntingSlot.x) * kx; };
    const float y0 = r.y + 3.0f * ky;
    const float sag = 12.0f * ky;

    // String: a straight run from the left post to the first socket, swags
    // between sockets, a straight run to the right post.
    std::vector<float> anchors;
    anchors.push_back(X(L::kBuntingSlot.x));
    for (std::size_t i = 0; i < L::kBulbCount; ++i) {
        anchors.push_back(X(gallery::bulbPosition(i).x));
    }
    anchors.push_back(X(L::kBuntingSlot.x + L::kBuntingSlot.width));
    const float half = 1.0f * ky;   // string half-thickness
    for (std::size_t s = 0; s + 1 < anchors.size(); ++s) {
        const bool swag = s > 0 && s + 2 < anchors.size();
        const float xa = anchors[s];
        const float xb = anchors[s + 1];
        sf::VertexArray line(sf::PrimitiveType::TriangleStrip);
        constexpr int kSteps = 24;
        for (int k = 0; k <= kSteps; ++k) {
            const float u = static_cast<float>(k) / kSteps;
            const float x = xa + (xb - xa) * u;
            const float y = y0 + (swag ? sag * 4.0f * u * (1.0f - u) : 0.0f);
            line.append(sf::Vertex{{x, y - half}, standin::kFrame});
            line.append(sf::Vertex{{x, y + half}, standin::kFrame});
        }
        t.draw(line);
        if (!swag) {
            continue;
        }
        int flag = static_cast<int>(s) * 2;
        for (const float u : {0.3f, 0.7f}) {
            const float cx = xa + (xb - xa) * u;
            const float top = y0 + sag * 4.0f * u * (1.0f - u);
            const float hw = 19.0f * kx;
            polygon(t, {{cx - hw, top}, {cx + hw, top}, {cx, top + 44.0f * ky}},
                    (flag++ % 2 == 0) ? standin::kPennantA : standin::kPennantB);
        }
    }
}

// ---------------------------------------------------------------------------
// Targets and rifle
// ---------------------------------------------------------------------------

void StandInPainter::duck(sf::RenderTarget& t, Rect r, sf::Color fill,
                          const sf::RenderStates& states) const {
    // Flat side-on cutout: body, head, bill, tail, hinge bracket.
    const float w = r.width;
    const float h = r.height;
    auto P = [&](float fx, float fy) { return sf::Vector2f{r.x + fx * w, r.y + fy * h}; };
    ellipse(t, P(0.47f, 0.64f), {0.40f * w, 0.24f * h}, fill, states);   // body
    ellipse(t, P(0.70f, 0.28f), {0.16f * w, 0.19f * h}, fill, states);   // head
    polygon(t, {P(0.80f, 0.24f), P(1.00f, 0.32f), P(0.82f, 0.40f)}, fill, states);   // bill
    polygon(t, {P(0.02f, 0.40f), P(0.18f, 0.55f), P(0.12f, 0.70f)}, fill, states);   // tail
    polygon(t, {P(0.58f, 0.40f), P(0.80f, 0.40f), P(0.74f, 0.60f), P(0.56f, 0.60f)}, fill,
            states);                                                                  // neck
    polygon(t, {P(0.38f, 0.84f), P(0.58f, 0.84f), P(0.58f, 1.00f), P(0.38f, 1.00f)}, fill,
            states);                                                                  // hinge
}

void StandInPainter::rifle(sf::RenderTarget& t, const RifleMap& toLogical) const {
    // The same flat silhouette parts as the side view (barrel, forend,
    // receiver, stock), seen from behind along the barrel. Symmetric about the
    // barrel line, so it reads correctly at every heading. Widths are in image
    // units at the swivel; the perspective map narrows them toward the muzzle
    // and widens the stock toward the eye, where it runs off the screen.
    auto part = [&](std::initializer_list<sf::Vector2f> pts, sf::Color fill) {
        sf::ConvexShape shape(pts.size());
        std::size_t i = 0;
        for (const auto& p : pts) {
            shape.setPoint(i++, toLogical(p.x, p.y));
        }
        shape.setFillColor(fill);
        t.draw(shape);
    };
    // {along, half-width} tapered outlines.
    auto slab = [&](float a0, float w0, float a1, float w1, sf::Color fill) {
        part({{a0, -w0}, {a1, -w1}, {a1, w1}, {a0, w0}}, fill);
    };
    slab(-0.45f, 54.0f, -0.06f, 30.0f, standin::kRifleWood);   // stock, butt toward the eye
    slab(0.22f, 22.0f, 0.62f, 18.0f, standin::kRifleWood);     // forend under the barrel
    slab(0.10f, 11.0f, 1.00f, 9.0f, standin::kRifle);          // barrel
    slab(0.955f, 14.0f, 1.00f, 14.0f, standin::kRifle);        // muzzle band
    part({{0.93f, -3.0f}, {0.99f, -3.0f}, {0.99f, 3.0f}, {0.93f, 3.0f}},
         standin::kRifleWood);                                  // front sight bead, a lighter fleck
    slab(-0.08f, 30.0f, 0.24f, 24.0f, standin::kRifle);        // receiver
    // Rear sight: a notched leaf at the back of the receiver.
    part({{0.04f, -22.0f}, {0.07f, -22.0f}, {0.07f, -5.0f}, {0.04f, -5.0f}}, standin::kRifleWood);
    part({{0.04f, 5.0f}, {0.07f, 5.0f}, {0.07f, 22.0f}, {0.04f, 22.0f}}, standin::kRifleWood);
}

// ---------------------------------------------------------------------------
// Interface and effects
// ---------------------------------------------------------------------------

void StandInPainter::board(sf::RenderTarget& t, Rect r) const {
    fillRect(t, r, standin::kBoard);
}

void StandInPainter::reticle(sf::RenderTarget& t, Vec2 centre, float s, float kick,
                             sf::Color light) const {
    // Light ring with a dark edge so it reads on both the backboard and the
    // sky; four ticks that open up with `kick`; a centre dot.
    const sf::Color dark(20, 16, 12, 200);
    sf::CircleShape ring(s * 0.34f, 48);
    ring.setOrigin({s * 0.34f, s * 0.34f});
    ring.setPosition(toSf(centre));
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(3.5f);
    ring.setOutlineColor(dark);
    t.draw(ring);
    ring.setOutlineThickness(1.8f);
    ring.setOutlineColor(light);
    t.draw(ring);
    const float gap = 7.0f + 7.0f * kick;
    const float len = s * 0.5f;
    const Rect ticks[4] = {{centre.x - len, centre.y - 1.0f, len - gap, 2.0f},
                           {centre.x + gap, centre.y - 1.0f, len - gap, 2.0f},
                           {centre.x - 1.0f, centre.y - len, 2.0f, len - gap},
                           {centre.x - 1.0f, centre.y + gap, 2.0f, len - gap}};
    for (const Rect& r : ticks) {
        fillRect(t, {r.x - 0.8f, r.y - 0.8f, r.width + 1.6f, r.height + 1.6f}, dark);
        fillRect(t, r, light);
    }
    fillRect(t, {centre.x - 1.2f, centre.y - 1.2f, 2.4f, 2.4f}, light);
}

void StandInPainter::missMark(sf::RenderTarget& t, Vec2 centre, float radius, float alpha) const {
    sf::CircleShape dot(radius, 20);
    dot.setOrigin({radius, radius});
    dot.setPosition(toSf(centre));
    dot.setFillColor(withAlpha(standin::kMark, alpha));
    t.draw(dot);
}

void StandInPainter::muzzlePuff(sf::RenderTarget& t, Rect r, float alpha) const {
    sf::CircleShape puff(r.width / 2.0f, 32);
    puff.setPosition({r.x, r.y});
    puff.setFillColor(withAlpha(standin::kPuff, 0.5f * alpha));
    t.draw(puff);
}

} // namespace shell::render
