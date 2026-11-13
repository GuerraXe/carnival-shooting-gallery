// Viewport is the single window<->logical mapping (D-006). ARCHITECTURE §2.4
// lists the required cases: exact 16:9, 4:3, 21:9, tiny, odd sizes,
// round-trip within 0.5 px, and points inside the bars.

#include "gallery/view/Viewport.hpp"

#include "support/test_framework.hpp"

using gallery::Rect;
using gallery::Size;
using gallery::Vec2;
using gallery::Viewport;

namespace {
constexpr double kEps = 1e-3;
}

TEST_CASE("viewport: default is identity-sized 1920x1080") {
    const Viewport v;
    CHECK_EQ(v.scale(), 1.0f);
    CHECK(v.pixelRect() == (Rect{0, 0, 1920, 1080}));
    CHECK(!v.degenerate());
}

TEST_CASE("viewport: exact 1920x1080 window maps 1:1, no bars") {
    const auto v = Viewport::fit({1920, 1080});
    CHECK_EQ(v.scale(), 1.0f);
    CHECK(v.pixelRect() == (Rect{0, 0, 1920, 1080}));
    const Vec2 l = v.toLogical({123, 456});
    CHECK_NEAR(l.x, 123, kEps);
    CHECK_NEAR(l.y, 456, kEps);
}

TEST_CASE("viewport: 1280x720 (16:9) scales uniformly, no bars") {
    const auto v = Viewport::fit({1280, 720});
    CHECK_NEAR(v.scale(), 2.0 / 3.0, 1e-6);
    const Rect r = v.pixelRect();
    CHECK_NEAR(r.x, 0, kEps);
    CHECK_NEAR(r.y, 0, kEps);
    CHECK_NEAR(r.width, 1280, kEps);
    CHECK_NEAR(r.height, 720, kEps);
    const Vec2 centre = v.toLogical({640, 360});
    CHECK_NEAR(centre.x, 960, kEps);
    CHECK_NEAR(centre.y, 540, kEps);
}

TEST_CASE("viewport: 3840x2160 (4K) scales up by 2") {
    const auto v = Viewport::fit({3840, 2160});
    CHECK_NEAR(v.scale(), 2.0, 1e-6);
    const Vec2 l = v.toLogical({3839, 2159});
    CHECK_NEAR(l.x, 1919.5, kEps);
    CHECK_NEAR(l.y, 1079.5, kEps);
}

TEST_CASE("viewport: 4:3 window gets top and bottom bars") {
    const auto v = Viewport::fit({1024, 768});
    const double scale = 1024.0 / 1920.0;
    CHECK_NEAR(v.scale(), scale, 1e-6);
    const Rect r = v.pixelRect();
    CHECK_NEAR(r.x, 0, kEps);
    CHECK_NEAR(r.width, 1024, kEps);
    CHECK_NEAR(r.height, 1080 * scale, kEps);   // 576
    CHECK_NEAR(r.y, (768 - 576) / 2.0, kEps);   // 96
    // Top-left of the canvas maps to logical origin.
    const Vec2 origin = v.toLogical({0, 96});
    CHECK_NEAR(origin.x, 0, kEps);
    CHECK_NEAR(origin.y, 0, kEps);
}

TEST_CASE("viewport: 21:9 window gets left and right bars") {
    const auto v = Viewport::fit({2560, 1080});
    CHECK_NEAR(v.scale(), 1.0, 1e-6);
    const Rect r = v.pixelRect();
    CHECK_NEAR(r.x, (2560 - 1920) / 2.0, kEps);   // 320
    CHECK_NEAR(r.y, 0, kEps);
    CHECK_NEAR(r.width, 1920, kEps);
    const Vec2 l = v.toLogical({320, 0});
    CHECK_NEAR(l.x, 0, kEps);
}

TEST_CASE("viewport: odd pixel sizes centre with fractional offsets") {
    const auto v = Viewport::fit({1001, 777});
    const double scale = 1001.0 / 1920.0;   // width-limited: 1001/1920 < 777/1080
    CHECK_NEAR(v.scale(), scale, 1e-6);
    const Rect r = v.pixelRect();
    CHECK_NEAR(r.y, (777 - 1080 * scale) / 2.0, 1e-3);
    CHECK_NEAR(r.x + r.width / 2, 1001 / 2.0, 1e-3);
    CHECK_NEAR(r.y + r.height / 2, 777 / 2.0, 1e-3);
}

TEST_CASE("viewport: tiny window still maps without dividing by zero") {
    const auto v = Viewport::fit({16, 9});
    CHECK(!v.degenerate());
    CHECK_NEAR(v.scale(), 16.0 / 1920.0, 1e-9);
    const Vec2 l = v.toLogical({8, 4.5f});
    CHECK_NEAR(l.x, 960, 0.01);
    CHECK_NEAR(l.y, 540, 0.01);
}

TEST_CASE("viewport: zero-size (minimised) window is degenerate and safe") {
    for (const Size s : {Size{0, 0}, Size{0, 720}, Size{1280, 0}, Size{-5, 100}}) {
        const auto v = Viewport::fit(s);
        CHECK(v.degenerate());
        CHECK_EQ(v.scale(), 0.0f);
        const Vec2 l = v.toLogical({100, 100});
        CHECK_EQ(l.x, 0.0f);
        CHECK_EQ(l.y, 0.0f);
    }
}

TEST_CASE("viewport: toPixel(toLogical(p)) round-trips within 0.5 px") {
    const Size windows[] = {{1920, 1080}, {1280, 720}, {1024, 768}, {2560, 1080},
                            {1001, 777},  {3840, 2160}, {800, 1200}};
    for (const Size w : windows) {
        const auto v = Viewport::fit(w);
        const Rect r = v.pixelRect();
        for (int i = 0; i <= 10; ++i) {
            for (int j = 0; j <= 10; ++j) {
                const Vec2 p{r.x + r.width * static_cast<float>(i) / 10.0f,
                             r.y + r.height * static_cast<float>(j) / 10.0f};
                const Vec2 back = v.toPixel(v.toLogical(p));
                CHECK_NEAR(back.x, p.x, 0.5);
                CHECK_NEAR(back.y, p.y, 0.5);
            }
        }
    }
}

TEST_CASE("viewport: points inside the letterbox bars clamp to the canvas edge") {
    const auto v = Viewport::fit({2560, 1080});   // 320 px bars left and right
    const Vec2 leftBar = v.clampToLogical(v.toLogical({10, 540}));
    CHECK_EQ(leftBar.x, 0.0f);
    CHECK_NEAR(leftBar.y, 540, kEps);
    const Vec2 rightBar = v.clampToLogical(v.toLogical({2550, 540}));
    CHECK(rightBar.x < 1920.0f);        // half-open: never equal to the width
    CHECK_NEAR(rightBar.x, 1920, 1e-3);

    const auto tall = Viewport::fit({1024, 768});   // 96 px bars top and bottom
    const Vec2 topBar = tall.clampToLogical(tall.toLogical({512, 5}));
    CHECK_EQ(topBar.y, 0.0f);
    const Vec2 bottomBar = tall.clampToLogical(tall.toLogical({512, 760}));
    CHECK(bottomBar.y < 1080.0f);
    CHECK_NEAR(bottomBar.y, 1080, 1e-3);
}

TEST_CASE("initial window: 100% scaling on a 1080p desktop keeps 1280x720") {
    const Size s = gallery::initialWindowSize({1920, 1080}, 1.0f);
    CHECK_EQ(s.width, 1280.0f);
    CHECK_EQ(s.height, 720.0f);
}

TEST_CASE("initial window: 150% scaling multiplies the preferred size") {
    const Size s = gallery::initialWindowSize({3840, 2160}, 1.5f);
    CHECK_EQ(s.width, 1920.0f);
    CHECK_EQ(s.height, 1080.0f);
}

TEST_CASE("initial window: too big for the desktop shrinks to 85%, keeping 16:9") {
    // 250% on a 2880x1800 laptop panel: 3200x1800 wanted, limit 2448x1530.
    const Size s = gallery::initialWindowSize({2880, 1800}, 2.5f);
    CHECK(s.width <= 2880 * 0.85f);
    CHECK(s.height <= 1800 * 0.85f);
    CHECK_EQ(s.width, 2448.0f);
    CHECK_NEAR(s.width / s.height, 16.0 / 9.0, 1e-3);
}

TEST_CASE("initial window: bad inputs fall back safely") {
    const Size noDesktop = gallery::initialWindowSize({0, 0}, 2.0f);
    CHECK_EQ(noDesktop.width, 2560.0f);   // scaled, but no desktop limit applied
    const Size badDpi = gallery::initialWindowSize({1920, 1080}, -1.0f);
    CHECK_EQ(badDpi.width, 1280.0f);
    const Size tiny = gallery::initialWindowSize({1, 1}, 1.0f);
    CHECK(tiny.width >= 1.0f);
    CHECK(tiny.height >= 1.0f);
}

TEST_CASE("viewport: clamp leaves interior points untouched") {
    const Viewport v;
    const Vec2 p = v.clampToLogical({960.25f, 540.75f});
    CHECK_EQ(p.x, 960.25f);
    CHECK_EQ(p.y, 540.75f);
    CHECK((Rect{0, 0, 1920, 1080}).contains(v.clampToLogical({5000, 5000})));
    CHECK((Rect{0, 0, 1920, 1080}).contains(v.clampToLogical({-5000, -5000})));
}
