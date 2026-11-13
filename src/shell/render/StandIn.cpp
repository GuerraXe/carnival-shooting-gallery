#include "shell/render/StandIn.hpp"

#include "shell/render/DrawUtil.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <cmath>
#include <initializer_list>
#include <numbers>

namespace shell::render {

using gallery::Rect;
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

} // namespace shell::render
