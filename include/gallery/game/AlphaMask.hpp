#pragma once

// Per-pixel hit mask built from a target image's alpha channel (D-009).
// One bit per texel, packed. Built once at load; immutable afterwards.

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace gallery {

class AlphaMask {
public:
    // Alpha at or above this counts as solid (D-009, Asset Guide §4: 128).
    static constexpr std::uint8_t kDefaultThreshold = 128;

    AlphaMask() = default;

    // `rgba` holds width*height*4 bytes, row-major, top row first (the
    // layout of sf::Image::getPixelsPtr()). A short buffer yields an empty mask.
    [[nodiscard]] static AlphaMask fromRgba(std::span<const std::uint8_t> rgba, int width,
                                            int height,
                                            std::uint8_t threshold = kDefaultThreshold);

    // Out of bounds is never solid.
    [[nodiscard]] bool solidAt(int x, int y) const noexcept;

    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }
    [[nodiscard]] bool empty() const noexcept { return width_ == 0 || height_ == 0; }
    [[nodiscard]] std::size_t solidCount() const noexcept { return solid_; }

private:
    int width_ = 0;
    int height_ = 0;
    std::size_t solid_ = 0;
    std::vector<std::uint64_t> bits_;
};

} // namespace gallery
