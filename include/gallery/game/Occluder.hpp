#pragma once

// What is drawn IN FRONT of the targets (brief G-16, AC-6; risk R-3).
//
// The wave cutout's crests cover the bottom of every duck, and in the chain
// preset the booth posts hide whole ducks while they wrap. A duck's hit mask
// is its full painted silhouette, so without this a click on a post or on a
// wave crest would score a duck the player cannot see. The shell renders the
// static foreground layers once at startup and hands the result over as a
// mask across the logical canvas; Round::fire treats an aim point on an
// opaque foreground texel as a shot into the scenery: a miss.
//
// Pure logic, no SFML (D-005).

#include "gallery/core/Geometry.hpp"
#include "gallery/game/AlphaMask.hpp"
#include "gallery/view/Viewport.hpp"

namespace gallery {

class Occluder {
public:
    // Nothing in front of the targets.
    Occluder() = default;

    // `mask` covers the whole logical canvas (`logical`); its texels may be
    // any resolution. An empty mask or a bad size means nothing in front.
    Occluder(AlphaMask mask, Size logical = kLogicalSize);

    // True when the foreground is opaque at this logical point. Points off
    // the canvas (and non-finite points) are never covered.
    [[nodiscard]] bool covers(Vec2 logical) const noexcept;

    [[nodiscard]] bool empty() const noexcept { return mask_.empty(); }
    [[nodiscard]] const AlphaMask& mask() const noexcept { return mask_; }

private:
    AlphaMask mask_;
    float sx_ = 0.0f;   // texels per logical unit
    float sy_ = 0.0f;
};

} // namespace gallery
