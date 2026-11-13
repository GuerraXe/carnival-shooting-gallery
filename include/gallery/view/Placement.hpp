#pragma once

#include "gallery/core/Geometry.hpp"

namespace gallery {

// How a photographic image is fitted into a logical slot. There is no
// "stretch" mode on purpose: a photograph is never scaled non-uniformly.
enum class Fit {
    Cover,     // fill the slot; crop the image symmetrically (source rect shrinks)
    Contain,   // whole image inside the slot, centred; slot may show gaps
    FitWidth,  // match the slot width; height follows the aspect ratio, anchored at the slot top
    FitHeight, // match the slot height; width follows the aspect ratio, anchored at the slot left
};

// Result of fitting an image into a slot.
//   source: the texel rectangle of the image to draw (crop), in image pixels
//   dest:   where that rectangle lands, in logical units
// Always: dest.width / source.width == dest.height / source.height (uniform scale).
struct Placement {
    Rect source;
    Rect dest;

    [[nodiscard]] float scale() const noexcept {
        return source.width > 0.0f ? dest.width / source.width : 0.0f;
    }
};

// Degenerate inputs (zero/negative/NaN sizes) yield an empty placement.
[[nodiscard]] Placement placeImage(Size image, Rect slot, Fit fit) noexcept;

} // namespace gallery
