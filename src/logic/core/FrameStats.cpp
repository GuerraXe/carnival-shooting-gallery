#include "gallery/core/FrameStats.hpp"

#include <algorithm>
#include <cmath>

namespace gallery {

void FrameStats::add(Seconds t) noexcept {
    if (!std::isfinite(t) || t < 0.0) {
        return;
    }
    const auto bin = std::min(static_cast<std::size_t>(t / kBinWidth), kBins - 1);
    ++bins_[bin];
    ++count_;
    sum_ += t;
    max_ = std::max(max_, t);
    hitches_ += t > kHitch ? 1 : 0;
}

Seconds FrameStats::mean() const noexcept {
    return count_ > 0 ? sum_ / static_cast<double>(count_) : 0.0;
}

Seconds FrameStats::percentile(double p) const noexcept {
    if (count_ == 0) {
        return 0.0;
    }
    p = std::clamp(std::isfinite(p) ? p : 1.0, 0.0, 1.0);
    // Smallest bin edge with at least ceil(p * count) frames at or below it.
    const auto need = std::max<std::uint64_t>(
        1, static_cast<std::uint64_t>(std::ceil(p * static_cast<double>(count_))));
    std::uint64_t seen = 0;
    for (std::size_t i = 0; i < kBins; ++i) {
        seen += bins_[i];
        if (seen >= need) {
            // The last bin is open-ended (every frame over 200 ms): only the
            // exact maximum describes it. Otherwise the upper bin edge, but
            // never above the maximum.
            if (i == kBins - 1) {
                return max_;
            }
            return std::min(static_cast<double>(i + 1) * kBinWidth, max_);
        }
    }
    return max_;
}

} // namespace gallery
