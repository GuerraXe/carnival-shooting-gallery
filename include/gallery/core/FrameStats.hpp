#pragma once

// Frame-time statistics for the performance acceptance check (brief AV-7:
// "60 FPS sustained for a full round, with no frame over 33 ms", checked
// from a frame-time log). The shell feeds the raw, unclamped wall-clock time
// of every rendered frame; carnival.log gets the summary at exit.
//
// Fixed memory: a 0.1 ms histogram up to 200 ms (slower frames land in the
// last bin; max() stays exact). Percentiles are the upper edge of their bin.

#include "gallery/core/Time.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gallery {

class FrameStats {
public:
    static constexpr Seconds kBinWidth = 0.0001;      // 0.1 ms
    static constexpr std::size_t kBins = 2000;        // 0 .. 200 ms
    static constexpr Seconds kHitch = 1.0 / 30.0;     // AV-7: "no frame over 33 ms"

    // Negative and non-finite times are ignored.
    void add(Seconds frameTime) noexcept;

    [[nodiscard]] std::uint64_t count() const noexcept { return count_; }
    [[nodiscard]] Seconds mean() const noexcept;            // 0 when empty
    [[nodiscard]] Seconds max() const noexcept { return max_; }
    [[nodiscard]] std::uint64_t hitches() const noexcept { return hitches_; }   // frames > kHitch
    // p in [0, 1]: the frame time that fraction p of frames do not exceed.
    [[nodiscard]] Seconds percentile(double p) const noexcept;

private:
    std::array<std::uint32_t, kBins> bins_{};
    std::uint64_t count_ = 0;
    std::uint64_t hitches_ = 0;
    Seconds sum_ = 0.0;
    Seconds max_ = 0.0;
};

} // namespace gallery
