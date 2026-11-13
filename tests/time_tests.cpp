#include "gallery/core/Time.hpp"

#include "support/test_framework.hpp"

#include <limits>

using gallery::clampFrameDelta;
using gallery::kMaxFrameDelta;

TEST_CASE("frame delta: normal values pass through unchanged") {
    CHECK_EQ(clampFrameDelta(1.0 / 60.0), 1.0 / 60.0);
    CHECK_EQ(clampFrameDelta(0.099), 0.099);
}

TEST_CASE("frame delta: exactly the cap is kept") {
    CHECK_EQ(clampFrameDelta(kMaxFrameDelta), kMaxFrameDelta);
}

TEST_CASE("frame delta: hitches are clamped to 100 ms (window drag, debugger)") {
    CHECK_EQ(clampFrameDelta(0.1000001), kMaxFrameDelta);
    CHECK_EQ(clampFrameDelta(5.0), kMaxFrameDelta);
}

TEST_CASE("frame delta: zero, negative and NaN become 0") {
    CHECK_EQ(clampFrameDelta(0.0), 0.0);
    CHECK_EQ(clampFrameDelta(-0.016), 0.0);
    CHECK_EQ(clampFrameDelta(std::numeric_limits<double>::quiet_NaN()), 0.0);
}

TEST_CASE("frame delta: custom cap is honoured") {
    CHECK_EQ(clampFrameDelta(0.5, 0.25), 0.25);
}
