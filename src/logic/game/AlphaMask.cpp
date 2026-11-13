#include "gallery/game/AlphaMask.hpp"

namespace gallery {

AlphaMask AlphaMask::fromRgba(std::span<const std::uint8_t> rgba, int width, int height,
                              std::uint8_t threshold) {
    AlphaMask m;
    if (width <= 0 || height <= 0) {
        return m;
    }
    const std::size_t count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (rgba.size() < count * 4) {
        return m;
    }
    m.width_ = width;
    m.height_ = height;
    m.bits_.assign((count + 63) / 64, 0);
    for (std::size_t i = 0; i < count; ++i) {
        if (rgba[i * 4 + 3] >= threshold) {
            m.bits_[i / 64] |= std::uint64_t{1} << (i % 64);
            ++m.solid_;
        }
    }
    return m;
}

bool AlphaMask::solidAt(int x, int y) const noexcept {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return false;
    }
    const std::size_t i = static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                          static_cast<std::size_t>(x);
    return (bits_[i / 64] >> (i % 64)) & 1u;
}

} // namespace gallery
