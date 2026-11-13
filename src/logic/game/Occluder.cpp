#include "gallery/game/Occluder.hpp"

#include <cmath>
#include <utility>

namespace gallery {

Occluder::Occluder(AlphaMask mask, Size logical) : mask_(std::move(mask)) {
    if (mask_.empty() || !(logical.width > 0.0f) || !(logical.height > 0.0f)) {
        mask_ = AlphaMask{};
        return;
    }
    sx_ = static_cast<float>(mask_.width()) / logical.width;
    sy_ = static_cast<float>(mask_.height()) / logical.height;
}

bool Occluder::covers(Vec2 p) const noexcept {
    if (mask_.empty() || !std::isfinite(p.x) || !std::isfinite(p.y)) {
        return false;
    }
    // AlphaMask::solidAt is false out of bounds, which covers off-canvas points.
    return mask_.solidAt(static_cast<int>(std::floor(p.x * sx_)),
                         static_cast<int>(std::floor(p.y * sy_)));
}

} // namespace gallery
