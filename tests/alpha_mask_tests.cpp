#include "support/test_framework.hpp"

#include "gallery/core/Affine.hpp"
#include "gallery/game/AlphaMask.hpp"

#include <cstdint>
#include <vector>

using namespace gallery;

namespace {

std::vector<std::uint8_t> rgbaWithAlpha(int w, int h, auto alphaAt) {
    std::vector<std::uint8_t> px(static_cast<std::size_t>(w * h * 4), 255);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            px[static_cast<std::size_t>((y * w + x) * 4 + 3)] = alphaAt(x, y);
        }
    }
    return px;
}

} // namespace

TEST_CASE("alpha mask: threshold boundary 127 transparent, 128 solid") {
    const auto px = rgbaWithAlpha(3, 1, [](int x, int) -> std::uint8_t {
        return x == 0 ? 127 : (x == 1 ? 128 : 0);
    });
    const AlphaMask m = AlphaMask::fromRgba(px, 3, 1);
    CHECK(!m.solidAt(0, 0));
    CHECK(m.solidAt(1, 0));
    CHECK(!m.solidAt(2, 0));
    CHECK_EQ(m.solidCount(), std::size_t{1});
}

TEST_CASE("alpha mask: out of bounds is never solid") {
    const auto px = rgbaWithAlpha(4, 4, [](int, int) -> std::uint8_t { return 255; });
    const AlphaMask m = AlphaMask::fromRgba(px, 4, 4);
    CHECK(m.solidAt(0, 0) && m.solidAt(3, 3));
    CHECK(!m.solidAt(-1, 0));
    CHECK(!m.solidAt(0, -1));
    CHECK(!m.solidAt(4, 0));
    CHECK(!m.solidAt(0, 4));
}

TEST_CASE("alpha mask: odd widths pack bits correctly across word boundaries") {
    const int w = 13, h = 11;   // 143 texels: spans three 64-bit words, rows straddle words
    auto pattern = [](int x, int y) -> std::uint8_t { return ((x * 7 + y * 3) % 5 == 0) ? 255 : 0; };
    const auto px = rgbaWithAlpha(w, h, pattern);
    const AlphaMask m = AlphaMask::fromRgba(px, w, h);
    std::size_t expected = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            CHECK_EQ(m.solidAt(x, y), pattern(x, y) == 255);
            expected += pattern(x, y) == 255 ? 1 : 0;
        }
    }
    CHECK_EQ(m.solidCount(), expected);
}

TEST_CASE("alpha mask: short buffer or bad size gives an empty mask") {
    const std::vector<std::uint8_t> px(10, 255);
    CHECK(AlphaMask::fromRgba(px, 4, 4).empty());
    CHECK(AlphaMask::fromRgba(px, 0, 4).empty());
    CHECK(!AlphaMask::fromRgba(px, 0, 4).solidAt(0, 0));
}

TEST_CASE("affine: compose, apply and inverse round-trip") {
    const Affine2 xf = Affine2::translation({100.0f, 50.0f}) * Affine2::rotationDeg(30.0f) *
                       Affine2::scaling(2.0f, 0.5f);
    const auto inv = xf.inverse();
    CHECK(inv.has_value());
    for (const Vec2 p : {Vec2{0, 0}, Vec2{10, -3}, Vec2{-7.5f, 22}}) {
        const Vec2 q = inv->apply(xf.apply(p));
        CHECK_NEAR(q.x, p.x, 1e-4);
        CHECK_NEAR(q.y, p.y, 1e-4);
    }
    // Rotation is clockwise-positive in y-down: +x rotates toward +y.
    const Vec2 r = Affine2::rotationDeg(90.0f).apply({1.0f, 0.0f});
    CHECK_NEAR(r.x, 0.0, 1e-6);
    CHECK_NEAR(r.y, 1.0, 1e-6);
    // A collapsed axis has no inverse.
    CHECK(!Affine2::scaling(1.0f, 0.0f).inverse().has_value());
}
