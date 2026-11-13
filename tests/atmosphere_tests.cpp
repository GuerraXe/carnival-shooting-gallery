// Atmosphere functions (D-031): the booth's light level, the shot's exposure
// flash, and the bunting sway. They must stay subtle, bounded, deterministic,
// and never stack into anything that could hide a target.

#include "support/test_framework.hpp"

#include "gallery/view/SceneLayout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

using namespace gallery;

TEST_CASE("atmosphere: ambient light stays within [min, 1] and is steady almost always") {
    int dimmed = 0;
    int samples = 0;
    float lowest = 1.0f;
    for (double t = 0.0; t < 600.0; t += 0.01) {
        const float l = ambientLight(t);
        CHECK(l >= kAmbientLightMin && l <= 1.0f);
        lowest = std::min(lowest, l);
        dimmed += l < 0.99f ? 1 : 0;   // a change of 1 % or more
        ++samples;
    }
    // Noticeably dimmed (>= 1 %) less than 4 % of the time.
    CHECK(static_cast<double>(dimmed) / samples < 0.04);
    // Never deeper than about 7 %: a sag, not a blackout.
    CHECK(lowest > 0.925f);
}

TEST_CASE("atmosphere: a generator sag happens every 10-40 s and lasts about a quarter second") {
    // Find sag events (light dropping 2 % below the tired-bulb-only level).
    std::vector<double> starts;
    bool in = false;
    double began = 0.0;
    for (double t = 0.0; t < 900.0; t += 0.002) {
        const double base = 1.0 - 0.004 * ((1.0 - bulbGlow(3, t)) + (1.0 - bulbGlow(8, t))) / 0.22;
        const bool sag = ambientLight(t) < base - 0.02;
        if (sag && !in) {
            began = t;
            starts.push_back(t);
        }
        if (!sag && in) {
            CHECK(t - began < 0.3);
        }
        in = sag;
    }
    CHECK(starts.size() >= 25u);   // 900 s / 25 s slots, nearly every slot
    for (std::size_t i = 1; i < starts.size(); ++i) {
        const double gap = starts[i] - starts[i - 1];
        CHECK(gap > 9.0 && gap < 41.0);
    }
}

TEST_CASE("atmosphere: light is a pure function of time and survives bad input") {
    for (double t = 0.0; t < 120.0; t += 0.37) {
        CHECK_EQ(ambientLight(t), ambientLight(t));
    }
    CHECK_EQ(ambientLight(std::numeric_limits<double>::quiet_NaN()), 1.0f);
    CHECK_EQ(ambientLight(std::numeric_limits<double>::infinity()), 1.0f);
    const float neg = ambientLight(-5.0);
    CHECK(neg >= kAmbientLightMin && neg <= 1.0f);
}

TEST_CASE("atmosphere: shot flash is 1 at the shot and gone within about three frames") {
    CHECK_EQ(muzzleFlash(0.0), 1.0f);
    CHECK(muzzleFlash(1.0 / 60.0) < 0.6f);
    CHECK(muzzleFlash(2.0 / 60.0) < 0.2f);
    CHECK_EQ(muzzleFlash(kMuzzleFlashTime), 0.0f);
    CHECK_EQ(muzzleFlash(1.0), 0.0f);
    CHECK_EQ(muzzleFlash(-0.01), 0.0f);
    CHECK_EQ(muzzleFlash(std::numeric_limits<double>::quiet_NaN()), 0.0f);
    float prev = 2.0f;
    for (double s = 0.0; s < kMuzzleFlashTime; s += 0.001) {
        const float f = muzzleFlash(s);
        CHECK(f <= prev);   // only ever fades
        prev = f;
    }
    // The flash ends long before the 250 ms fire cooldown: shots never stack it.
    static_assert(kMuzzleFlashTime < 0.25);
}

TEST_CASE("atmosphere: bunting sways under half a degree and two pixels") {
    float maxAngle = 0.0f;
    float maxDx = 0.0f;
    for (double t = 0.0; t < 300.0; t += 0.05) {
        const Sway s = buntingSway(t);
        maxAngle = std::max(maxAngle, std::fabs(s.angleDeg));
        maxDx = std::max(maxDx, std::fabs(s.dx));
    }
    CHECK(maxAngle < 0.5f);
    CHECK(maxDx <= 2.0f);
    CHECK(maxAngle > 0.1f);   // it does move
    const Sway bad = buntingSway(std::numeric_limits<double>::quiet_NaN());
    CHECK_EQ(bad.dx, 0.0f);
    CHECK_EQ(bad.angleDeg, 0.0f);
}
