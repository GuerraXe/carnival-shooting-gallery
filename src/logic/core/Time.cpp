#include "gallery/core/Time.hpp"

namespace gallery {

Seconds clampFrameDelta(Seconds raw, Seconds maxDelta) noexcept {
    // `!(raw > 0)` also catches NaN.
    if (!(raw > 0.0)) {
        return 0.0;
    }
    return raw < maxDelta ? raw : maxDelta;
}

} // namespace gallery
