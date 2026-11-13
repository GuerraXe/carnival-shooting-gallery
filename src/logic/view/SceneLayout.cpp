#include "gallery/view/SceneLayout.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <vector>

namespace gallery {

std::string_view layerGroupName(LayerGroup group) noexcept {
    switch (group) {
    case LayerGroup::Environment: return "Background & booth";
    case LayerGroup::Decorations: return "Decorations & signage";
    case LayerGroup::Track: return "Target track";
    case LayerGroup::Targets: return "Duck targets";
    case LayerGroup::Counter: return "Shooting counter";
    case LayerGroup::Rifle: return "Rifle prop";
    case LayerGroup::Interface: return "Crosshair & HUD";
    case LayerGroup::Effects: return "Shot & impact effects";
    case LayerGroup::Count: break;
    }
    return "?";
}

Rect duckRect(float left, float width) noexcept {
    return {left, layout::kTrackY - layout::kDuckHeight, width, layout::kDuckHeight};
}

namespace {
constexpr float kDegToRad = std::numbers::pi_v<float> / 180.0f;

float clampRecoil(float r) noexcept {
    return std::isfinite(r) ? std::clamp(r, 0.0f, 1.0f) : 0.0f;
}
} // namespace

float rifleHeadingToward(Vec2 aim) noexcept {
    using namespace layout;
    const Vec2 v = aim - kRiflePivot;
    if (!std::isfinite(v.x) || !std::isfinite(v.y) || (v.x == 0.0f && v.y == 0.0f)) {
        return -90.0f;
    }
    // Screen angle, y down: -90 is straight up. Aims below the swivel's
    // horizon would point the barrel at the counter; clamp to the limits.
    float h = std::atan2(v.y, v.x) / kDegToRad;
    if (h > 90.0f) {
        h -= 360.0f;   // keep the left half continuous: (-180, 90]
    }
    return std::clamp(h, kRifleHeadingMinDeg, kRifleHeadingMaxDeg);
}

RiflePose riflePose(Vec2 aim, float recoil) noexcept {
    return {rifleHeadingToward(aim), clampRecoil(recoil)};
}

Vec2 rifleImageToLogical(Vec2 imagePoint, const RiflePose& pose, Vec2 pivotInImage,
                         Vec2 muzzleInImage) noexcept {
    using namespace layout;
    // Image frame: axis from the swivel to the muzzle, and its perpendicular
    // (same rotation sense as the screen frame below: a pure rotation, no flip).
    const Vec2 axis = muzzleInImage - pivotInImage;
    const float len = std::sqrt(axis.x * axis.x + axis.y * axis.y);
    if (!(len > 0.0f)) {
        return kRiflePivot;
    }
    const Vec2 a{axis.x / len, axis.y / len};
    const Vec2 aPerp{-a.y, a.x};
    const Vec2 rel = imagePoint - pivotInImage;
    const float along = (rel.x * a.x + rel.y * a.y) / len;   // 0 swivel .. 1 muzzle
    const float across = rel.x * aPerp.x + rel.y * aPerp.y;  // image units
    return rifleAxisToLogical(along, across, pose);
}

Vec2 rifleAxisToLogical(float along, float across, const RiflePose& pose) noexcept {
    using namespace layout;
    // Recoil: the muzzle climbs (toward straight up) and the whole gun comes
    // back toward the eye.
    const float r = clampRecoil(pose.recoil);
    const float climb = kRifleRecoilClimbDeg * r * (pose.headingDeg < -90.0f ? 1.0f : -1.0f);
    const float h = (pose.headingDeg + climb) * kDegToRad;
    const Vec2 d{std::cos(h), std::sin(h)};
    const Vec2 n{-d.y, d.x};
    const Vec2 base = kRiflePivot - d * (kRifleRecoilBack * r);

    // Perspective along the barrel: farther along = smaller. Projective, so
    // straight edges stay straight. The denominator is kept positive for the
    // stock end, which is behind the swivel (toward the viewer).
    const float denom = std::max(0.25f, 1.0f + kRifleDepth * along);
    const float alongPx = kRifleReach * (1.0f + kRifleDepth) * along / denom;
    const float acrossPx = kRifleScale * across / denom;
    return base + d * alongPx + n * acrossPx;
}

Rect rifleBounds(const RiflePose& pose, Size imageSize, Vec2 pivotInImage,
                 Vec2 muzzleInImage) noexcept {
    // The silhouette lies in a corridor of +/- kRifleCorridor image units
    // around the swivel->muzzle line (asset rule), spanning the frame's full
    // length along it. The map is projective, so the corridor's image is a
    // convex quad and its four corners bound it.
    const Vec2 axis = muzzleInImage - pivotInImage;
    const float len = std::sqrt(axis.x * axis.x + axis.y * axis.y);
    if (!(len > 0.0f)) {
        return {layout::kRiflePivot.x, layout::kRiflePivot.y, 0.0f, 0.0f};
    }
    const Vec2 a{axis.x / len, axis.y / len};
    const Vec2 aPerp{-a.y, a.x};
    float lo = std::numeric_limits<float>::infinity();
    float hi = -lo;
    for (const Vec2 c : {Vec2{0.0f, 0.0f}, Vec2{imageSize.width, 0.0f}, Vec2{0.0f, imageSize.height},
                         Vec2{imageSize.width, imageSize.height}}) {
        const Vec2 rel = c - pivotInImage;
        const float along = rel.x * a.x + rel.y * a.y;   // image units
        lo = std::min(lo, along);
        hi = std::max(hi, along);
    }
    const float w = layout::kRifleCorridor;
    const std::array<Vec2, 4> corners{{pivotInImage + a * lo + aPerp * w,
                                       pivotInImage + a * lo - aPerp * w,
                                       pivotInImage + a * hi + aPerp * w,
                                       pivotInImage + a * hi - aPerp * w}};
    // Only what is on screen matters: the stock runs off the bottom edge, and
    // an axis-aligned box around that off-screen part would claim screen area
    // the rifle never covers. Clip the quad to the canvas first
    // (Sutherland-Hodgman against its four edges), then bound it.
    std::vector<Vec2> poly;
    // Corner order around the quad: lo+, hi+, hi-, lo-.
    for (const int i : {0, 2, 3, 1}) {
        poly.push_back(rifleImageToLogical(corners[static_cast<std::size_t>(i)], pose, pivotInImage,
                                           muzzleInImage));
    }
    auto clip = [&](auto inside, auto cut) {
        std::vector<Vec2> out;
        for (std::size_t i = 0; i < poly.size(); ++i) {
            const Vec2 p = poly[i];
            const Vec2 q = poly[(i + 1) % poly.size()];
            if (inside(p)) {
                out.push_back(p);
            }
            if (inside(p) != inside(q)) {
                out.push_back(cut(p, q));
            }
        }
        poly = std::move(out);
    };
    const float W = kLogicalSize.width;
    const float H = kLogicalSize.height;
    auto atX = [](Vec2 p, Vec2 q, float x) { return p + (q - p) * ((x - p.x) / (q.x - p.x)); };
    auto atY = [](Vec2 p, Vec2 q, float y) { return p + (q - p) * ((y - p.y) / (q.y - p.y)); };
    clip([](Vec2 p) { return p.x >= 0.0f; }, [&](Vec2 p, Vec2 q) { return atX(p, q, 0.0f); });
    clip([&](Vec2 p) { return p.x <= W; }, [&](Vec2 p, Vec2 q) { return atX(p, q, W); });
    clip([](Vec2 p) { return p.y >= 0.0f; }, [&](Vec2 p, Vec2 q) { return atY(p, q, 0.0f); });
    clip([&](Vec2 p) { return p.y <= H; }, [&](Vec2 p, Vec2 q) { return atY(p, q, H); });
    if (poly.empty()) {
        return {layout::kRiflePivot.x, H, 0.0f, 0.0f};   // entirely off screen
    }
    constexpr float kInf = std::numeric_limits<float>::infinity();
    float minX = kInf, minY = kInf, maxX = -kInf, maxY = -kInf;
    for (const Vec2 p : poly) {
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
    }
    return {minX, minY, maxX - minX, maxY - minY};
}

void RifleMount::update(Seconds dt, float targetHeadingDeg) noexcept {
    if (!(dt > 0.0) || !std::isfinite(dt) || !std::isfinite(targetHeadingDeg)) {
        return;
    }
    // Damped spring, semi-implicit Euler in fixed 2 ms steps: stable and the
    // same feel at any frame rate.
    constexpr Seconds kStep = 0.002;
    const float w = layout::kRifleStiffness;
    const float z = layout::kRifleDamping;
    carry_ += std::min(dt, 0.25);
    while (carry_ >= kStep) {
        const float accel = w * w * (targetHeadingDeg - heading_) - 2.0f * z * w * velocity_;
        velocity_ += accel * static_cast<float>(kStep);
        heading_ += velocity_ * static_cast<float>(kStep);
        carry_ -= kStep;
    }
}

float bulbGlow(std::size_t index, Seconds t) noexcept {
    // Bulbs 3 and 8 have a tired filament: a slow, irregular dip built from
    // two incommensurate sines. Everyone else burns steady.
    if (index != 3 && index != 8) {
        return 1.0f;
    }
    const double phase = static_cast<double>(index) * 1.7;
    const double a = std::sin(t * 2.3 + phase);
    const double b = std::sin(t * 7.1 + phase * 2.0);
    const double dip = std::max(0.0, a * b);                 // [0, 1], mostly near 0
    return static_cast<float>(1.0 - 0.22 * dip);
}

Vec2 bulbPosition(std::size_t index) noexcept {
    return {layout::kBulbFirstX + static_cast<float>(index) * layout::kBulbSpacing,
            layout::kBulbRowY};
}

namespace {

// Stable pseudo-random value in [0, 1) for an integer slot (no state, no
// runtime randomness: the same t always gives the same light).
double slotHash(std::int64_t k, std::uint32_t salt) noexcept {
    auto x = static_cast<std::uint32_t>(k) * 2654435761u ^ salt;
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return static_cast<double>(x & 0xFFFFFFu) / static_cast<double>(0x1000000u);
}

constexpr double kSagSlot = 25.0;       // one possible sag per 25 s slot
constexpr double kSagDuration = 0.26;   // seconds
constexpr double kSagDepthMin = 0.025;
constexpr double kSagDepthMax = 0.055;
constexpr double kTiredBulbShare = 0.004;   // scene light lost per tired bulb at its dimmest (kept tiny: the bulb itself flickers, the booth barely does)

} // namespace

float ambientLight(Seconds t) noexcept {
    if (!std::isfinite(t)) {
        return 1.0f;
    }
    // Generator sag: one event per slot, placed in the middle 60 % of the
    // slot so consecutive sags are 15-35 s apart.
    double sag = 0.0;
    const auto slot = static_cast<std::int64_t>(std::floor(t / kSagSlot));
    const double start = (static_cast<double>(slot) + 0.2 + 0.6 * slotHash(slot, 0xA11CEu)) * kSagSlot;
    const double x = (t - start) / kSagDuration;
    if (x >= 0.0 && x < 1.0) {
        const double depth = kSagDepthMin + (kSagDepthMax - kSagDepthMin) * slotHash(slot, 0xB0B5u);
        // Fast drop (first 15 %), slower recovery: filaments cool quickly
        // and take longer to come back to full heat.
        const double shape = x < 0.15 ? x / 0.15 : std::pow((1.0 - x) / 0.85, 1.6);
        sag = depth * shape;
    }
    // Tired bulbs: bulbGlow dips to 0.78 at worst (a 0.22 dip).
    const double tired = kTiredBulbShare * ((1.0 - bulbGlow(3, t)) + (1.0 - bulbGlow(8, t))) / 0.22;
    const double level = 1.0 - sag - tired;
    return static_cast<float>(std::clamp(level, static_cast<double>(kAmbientLightMin), 1.0));
}

float muzzleFlash(Seconds sinceShot) noexcept {
    if (!std::isfinite(sinceShot) || sinceShot < 0.0 || sinceShot >= kMuzzleFlashTime) {
        return 0.0f;
    }
    const double k = 1.0 - sinceShot / kMuzzleFlashTime;
    return static_cast<float>(k * k);
}

Sway buntingSway(Seconds t) noexcept {
    if (!std::isfinite(t)) {
        return {0.0f, 0.0f};
    }
    // Two slow, incommensurate periods (about 6.1 s and 9.7 s) so the motion
    // never visibly repeats; amplitudes far below anything that would draw
    // the eye away from the lane.
    const double a = std::sin(t * (2.0 * std::numbers::pi / 6.1));
    const double b = std::sin(t * (2.0 * std::numbers::pi / 9.7) + 1.3);
    return {static_cast<float>(1.2 * a + 0.6 * b), static_cast<float>(0.22 * a + 0.12 * b)};
}

bool overlaps(Rect a, Rect b) noexcept {
    return a.x < b.x + b.width && b.x < a.x + a.width && a.y < b.y + b.height &&
           b.y < a.y + a.height;
}

} // namespace gallery
