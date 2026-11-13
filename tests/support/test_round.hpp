#pragma once

// Shared fixtures for round / flow tests: a target silhouette like a real
// cutout and a Round built on the default `sections` preset.

#include "gallery/game/Shooting.hpp"
#include "gallery/view/SceneLayout.hpp"

#include <cstdint>
#include <variant>
#include <vector>

namespace gallery::test {

// A solid ellipse in a 64x52 image: the bounding-box corners are transparent.
inline DuckShape ellipseShape() {
    const int w = 64, h = 52;
    std::vector<std::uint8_t> px(static_cast<std::size_t>(w * h * 4), 255);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const float dx = (static_cast<float>(x) + 0.5f - w / 2.0f) / (w / 2.0f);
            const float dy = (static_cast<float>(y) + 0.5f - h / 2.0f) / (h / 2.0f);
            px[static_cast<std::size_t>((y * w + x) * 4 + 3)] = dx * dx + dy * dy <= 1.0f ? 255 : 0;
        }
    }
    const float lw = layout::kDuckHeight * layout::kDuckStandInAspect;
    return DuckShape{{lw, layout::kDuckHeight}, {64.0f, 52.0f}, AlphaMask::fromRgba(px, w, h)};
}

inline Round makeTimedRound(RoundTuning timing = {}, ShotTuning shot = {}) {
    const DuckShape s = ellipseShape();
    return Round(DuckLane(makeLaneConfig(LanePreset::Sections), {s, s, s}), shot, timing);
}

// Centre of target `id`'s silhouette as currently posed.
inline Vec2 centreOf(const DuckLane& lane, std::size_t id) {
    const Quad q = lane.quad(id);
    return {(q.p[0].x + q.p[2].x) / 2.0f, (q.p[0].y + q.p[2].y) / 2.0f};
}

inline constexpr Vec2 kOpenBackboard{960.0f, 380.0f};   // above the lane: always a miss

template <class E>
std::size_t countEvents(const std::vector<GameEvent>& events) {
    std::size_t n = 0;
    for (const auto& e : events) {
        n += std::holds_alternative<E>(e) ? 1 : 0;
    }
    return n;
}

} // namespace gallery::test
