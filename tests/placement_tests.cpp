// placeImage never distorts a photograph: every mode keeps a uniform scale.

#include "gallery/view/Placement.hpp"

#include "support/test_framework.hpp"

using gallery::Fit;
using gallery::placeImage;
using gallery::Rect;
using gallery::Size;

namespace {

constexpr double kEps = 1e-3;

bool uniform(const gallery::Placement& p) {
    if (p.source.width <= 0 || p.source.height <= 0) {
        return false;
    }
    const double sx = p.dest.width / p.source.width;
    const double sy = p.dest.height / p.source.height;
    return std::fabs(sx - sy) < 1e-4;
}

} // namespace

TEST_CASE("placement: cover fills the slot and crops symmetrically") {
    // 1920x500 plank photo into a 1520x570 opening: height-limited, crops width.
    const auto p = placeImage({1920, 500}, {200, 290, 1520, 570}, Fit::Cover);
    CHECK(uniform(p));
    CHECK(p.dest == (Rect{200, 290, 1520, 570}));
    CHECK_NEAR(p.source.height, 500, kEps);
    CHECK_NEAR(p.source.width, 1520.0 / (570.0 / 500.0), kEps);
    CHECK_NEAR(p.source.x, (1920.0 - p.source.width) / 2.0, kEps);
    CHECK_NEAR(p.source.y, 0, kEps);
}

TEST_CASE("placement: cover with a taller image crops height") {
    const auto p = placeImage({1000, 1000}, {0, 0, 400, 100}, Fit::Cover);
    CHECK(uniform(p));
    CHECK_NEAR(p.source.width, 1000, kEps);
    CHECK_NEAR(p.source.height, 250, kEps);
    CHECK_NEAR(p.source.y, 375, kEps);
}

TEST_CASE("placement: contain keeps the whole image, centred") {
    const auto p = placeImage({1400, 260}, {250, 118, 1040, 164}, Fit::Contain);
    CHECK(uniform(p));
    CHECK(p.source == (Rect{0, 0, 1400, 260}));
    CHECK_NEAR(p.dest.height, 164, kEps);   // height-limited
    CHECK_NEAR(p.dest.width, 1400.0 * 164.0 / 260.0, kEps);
    CHECK_NEAR(p.dest.x + p.dest.width / 2.0, 250 + 1040 / 2.0, kEps);
    CHECK(p.dest.width <= 1040 + kEps);
}

TEST_CASE("placement: fit-width anchors at the slot top and keeps aspect") {
    const auto p = placeImage({1920, 260}, {0, 860, 1920, 220}, Fit::FitWidth);
    CHECK(uniform(p));
    CHECK_NEAR(p.dest.x, 0, kEps);
    CHECK_NEAR(p.dest.y, 860, kEps);
    CHECK_NEAR(p.dest.width, 1920, kEps);
    CHECK_NEAR(p.dest.height, 260, kEps);   // taller than the slot: extends below, never squashed
}

TEST_CASE("placement: fit-height") {
    const auto p = placeImage({96, 128}, {10, 20, 999, 32}, Fit::FitHeight);
    CHECK(uniform(p));
    CHECK_NEAR(p.dest.width, 24, kEps);
    CHECK_NEAR(p.dest.height, 32, kEps);
    CHECK_NEAR(p.scale(), 0.25, kEps);
}

TEST_CASE("placement: degenerate inputs yield an empty placement") {
    CHECK(placeImage({0, 100}, {0, 0, 10, 10}, Fit::Cover).dest == Rect{});
    CHECK(placeImage({100, 100}, {0, 0, 0, 10}, Fit::Contain).dest == Rect{});
    CHECK(placeImage({NAN, 100}, {0, 0, 10, 10}, Fit::FitWidth).dest == Rect{});
    CHECK_EQ(placeImage({-5, 100}, {0, 0, 10, 10}, Fit::Cover).scale(), 0.0f);
}
