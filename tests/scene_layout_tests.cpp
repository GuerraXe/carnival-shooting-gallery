// The composition rules of brief §6 / §7.6 and Asset Guide §2.4, checked
// against the numbers in SceneLayout so a layout tweak cannot silently break
// them.

#include "gallery/assets/AssetCatalog.hpp"
#include "gallery/view/SceneLayout.hpp"

#include "support/test_framework.hpp"

#include <cstddef>

using namespace gallery;
using namespace gallery::layout;

namespace {

bool inside(Rect inner, Rect outer) {
    return inner.x >= outer.x && inner.y >= outer.y &&
           inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}

constexpr float kStandInDuckWidth = kDuckHeight * kDuckStandInAspect;

std::size_t passIndex(DrawPass p) {
    for (std::size_t i = 0; i < kDrawPasses.size(); ++i) {
        if (kDrawPasses[i].pass == p) {
            return i;
        }
    }
    return kDrawPasses.size();
}

} // namespace

TEST_CASE("layout: asset catalog order matches AssetId and keys are unique") {
    for (std::size_t i = 0; i < kAssetCatalog.size(); ++i) {
        CHECK_EQ(static_cast<std::size_t>(kAssetCatalog[i].id), i);
        for (std::size_t j = i + 1; j < kAssetCatalog.size(); ++j) {
            CHECK(kAssetCatalog[i].key != kAssetCatalog[j].key);
            CHECK(kAssetCatalog[i].path != kAssetCatalog[j].path);
        }
        CHECK(assetIdFromKey(kAssetCatalog[i].key) == kAssetCatalog[i].id);
    }
    CHECK(!assetIdFromKey("NOPE").has_value());
}

TEST_CASE("layout: draw passes are listed in enum order, back to front") {
    for (std::size_t i = 0; i < kDrawPasses.size(); ++i) {
        CHECK_EQ(static_cast<std::size_t>(kDrawPasses[i].pass), i);
    }
}

TEST_CASE("layout: post-processed passes all come before the clear ones (D-014)") {
    bool seenClear = false;
    for (const PassInfo& p : kDrawPasses) {
        if (!p.postProcessed) {
            seenClear = true;
        }
        CHECK(!(seenClear && p.postProcessed));
    }
    CHECK(kDrawPasses.back().pass == DrawPass::Crosshair);   // reticle always on top
}

TEST_CASE("layout: occlusion order gives the booth its depth") {
    // Miss marks sit on the backboard, behind the ducks.
    CHECK(passIndex(DrawPass::Backboard) < passIndex(DrawPass::MissMarks));
    CHECK(passIndex(DrawPass::MissMarks) < passIndex(DrawPass::Ducks));
    // Rail behind ducks; wave cutout in front of them; posts in front of both.
    CHECK(passIndex(DrawPass::TrackRail) < passIndex(DrawPass::Ducks));
    CHECK(passIndex(DrawPass::Ducks) < passIndex(DrawPass::WaveStrip));
    CHECK(passIndex(DrawPass::WaveStrip) < passIndex(DrawPass::BoothFrame));
    // Counter in front of the booth, rifle on the counter, puff at the muzzle.
    CHECK(passIndex(DrawPass::BoothFrame) < passIndex(DrawPass::Counter));
    CHECK(passIndex(DrawPass::Counter) < passIndex(DrawPass::Rifle));
    CHECK(passIndex(DrawPass::Rifle) < passIndex(DrawPass::MuzzlePuff));
}

TEST_CASE("layout: every one of the eight layer groups owns at least one pass") {
    for (std::size_t g = 0; g < kLayerGroupCount; ++g) {
        bool found = false;
        for (const PassInfo& p : kDrawPasses) {
            found = found || static_cast<std::size_t>(p.group) == g;
        }
        CHECK(found);
        CHECK(layerGroupName(static_cast<LayerGroup>(g)) != "?");
    }
}

TEST_CASE("layout: background plates cover the whole canvas") {
    CHECK(inside(kCanvas, kFarPlate));
    CHECK(inside(kCanvas, kMidPlate));
}

TEST_CASE("layout: ducks sit in the 40-60 % lane band, near the horizon") {
    const Rect duck = duckRect(800.0f, kStandInDuckWidth);
    CHECK(inside(duck, kLaneBand));
    CHECK(duck.y < kHorizonY && kHorizonY < duck.y + duck.height);   // horizon crosses the duck
    // 8-10 % of screen height (Asset Guide §2.4).
    CHECK(kDuckHeight >= 0.08f * kLogicalSize.height);
    CHECK(kDuckHeight <= 0.10f * kLogicalSize.height);
}

TEST_CASE("layout: the wave cutout hides the duck hinge line and the rail") {
    CHECK(kWaveStrip.y < kTrackY);
    CHECK(kWaveStrip.y + kWaveStrip.height > kTrackY);
    CHECK(kTrackRail.y + kTrackRail.height > kWaveStrip.y);
    // It spans the whole opening but never pokes out past the posts.
    CHECK(kWaveStrip.x >= kLeftPost.x);
    CHECK(kWaveStrip.x <= kOpening.x);
    CHECK(kWaveStrip.x + kWaveStrip.width >= kOpening.x + kOpening.width);
    CHECK(kWaveStrip.x + kWaveStrip.width <= kRightPost.x + kRightPost.width);
    // D-040: nothing of the mechanism shows, not even in a trough. The rail
    // lies wholly behind the solid board, and the deepest trough stays above
    // the bottom 12 % of the duck (its hinge bracket and flat base), with a
    // 2-unit margin.
    static_assert(kTrackRail.y >= kWaveSolidTop);
    static_assert(kWaveSolidTop <= kTrackY - 0.12f * kDuckHeight - 2.0f);
    // ...while most of the duck stays visible above the crests.
    CHECK(kWaveStrip.y - (kTrackY - kDuckHeight) >= 0.7f * kDuckHeight);
}

TEST_CASE("layout: chain-loop ducks wrap only while fully hidden behind a post") {
    // Duck at the wrap points: entering at kLoopStart, leaving at the end.
    const Rect entering = duckRect(kLoopStart, kStandInDuckWidth);
    const Rect leaving = duckRect(kLoopStart + kLoopLength, kStandInDuckWidth);
    CHECK(entering.x >= kLeftPost.x);
    CHECK(entering.x + entering.width <= kLeftPost.x + kLeftPost.width);
    CHECK(leaving.x >= kRightPost.x);
    CHECK(leaving.x + leaving.width <= kRightPost.x + kRightPost.width);
    // Posts are wide enough to hide a duck at all.
    CHECK(kLeftPost.width >= kStandInDuckWidth);
    CHECK(kRightPost.width >= kStandInDuckWidth);
}

TEST_CASE("layout: rifle never covers the lane at any aim or recoil (brief §6.2)") {
    for (float x = 0.0f; x <= 1920.0f; x += 40.0f) {
        for (float y : {0.0f, 540.0f, 1079.0f}) {
            for (float recoil : {0.0f, 0.5f, 1.0f}) {
                const Rect b = rifleBounds(riflePose({x, y}, recoil));
                CHECK(b.y >= kLaneBand.y + kLaneBand.height);
            }
        }
    }
}

TEST_CASE("layout: the barrel points straight at the aim (D-039)") {
    for (const Vec2 aim : {Vec2{960, 540}, Vec2{300, 500}, Vec2{1700, 480}, Vec2{1150, 300}}) {
        const RiflePose p = riflePose(aim, 0);
        const Vec2 swivel = rifleImageToLogical(kRiflePivotInImage, p);
        const Vec2 muzzle = rifleImageToLogical(kRifleMuzzleInImage, p);
        CHECK_NEAR(swivel.x, kRiflePivot.x, 1e-3);
        CHECK_NEAR(swivel.y, kRiflePivot.y, 1e-3);
        // swivel, muzzle and aim are collinear, muzzle between them
        const Vec2 a = muzzle - swivel;
        const Vec2 b = aim - swivel;
        const float cross = a.x * b.y - a.y * b.x;
        const float la = std::sqrt(a.x * a.x + a.y * a.y);
        const float lb = std::sqrt(b.x * b.x + b.y * b.y);
        CHECK_NEAR(cross / (la * lb), 0.0f, 1e-4f);
        CHECK(a.x * b.x + a.y * b.y > 0.0f);
        CHECK_NEAR(la, kRifleReach, 0.05f);
        CHECK(la < lb);
    }
    // Aims beside or below the swivel are clamped: the barrel never points down.
    CHECK_NEAR(riflePose({0, 1079}, 0).headingDeg, kRifleHeadingMinDeg, 1e-4f);
    CHECK_NEAR(riflePose({1919, 1079}, 0).headingDeg, kRifleHeadingMaxDeg, 1e-4f);
    CHECK_NEAR(riflePose({960, 500}, 7).recoil, 1.0f, 1e-6f);
    CHECK_NEAR(riflePose({960, 500}, -1).recoil, 0.0f, 1e-6f);
    CHECK_NEAR(riflePose({960, 500}, NAN).recoil, 0.0f, 1e-6f);
}

TEST_CASE("layout: first-person perspective: the gun narrows toward the muzzle, the stock runs off-screen") {
    const RiflePose p = riflePose({960, 500}, 0);
    // Barrel width at the receiver end vs. at the muzzle (same image width).
    auto width = [&](Vec2 top, Vec2 bottom) {
        const Vec2 d = rifleImageToLogical(top, p) - rifleImageToLogical(bottom, p);
        return std::sqrt(d.x * d.x + d.y * d.y);
    };
    const float nearW = width({440, 236}, {430, 270});
    const float farW = width({24, 58}, {16, 84});
    CHECK(farW < nearW * 0.75f);
    // The butt of the stock is below the bottom of the screen.
    CHECK(rifleImageToLogical({900, 500}, p).y > kLogicalSize.height);
    // Recoil pulls the gun back toward the eye (down the screen).
    CHECK(rifleImageToLogical(kRifleMuzzleInImage, riflePose({960, 500}, 1)).y >
          rifleImageToLogical(kRifleMuzzleInImage, p).y);
}

TEST_CASE("layout: the rifle mount is stiff: it lags, settles without wobble, at any frame rate") {
    RifleMount m;
    m.reset(-90.0f);
    m.update(1.0 / 60.0, -130.0f);
    CHECK(m.heading() > -110.0f);                 // one frame later it has barely started
    for (int i = 0; i < 59; ++i) {
        m.update(1.0 / 60.0, -130.0f);
    }
    CHECK_NEAR(m.heading(), -130.0f, 0.5f);       // settled within a second
    // No visible overshoot (damping 0.9: under 0.5 %).
    RifleMount o;
    o.reset(-90.0f);
    float most = -90.0f;
    for (int i = 0; i < 240; ++i) {
        o.update(1.0 / 120.0, -130.0f);
        most = std::min(most, o.heading());
    }
    CHECK(most > -130.0f - 0.5f);
    // Same motion at 30 and 144 Hz (fixed internal substeps).
    RifleMount a, b;
    a.reset(-90.0f);
    b.reset(-90.0f);
    for (int i = 0; i < 9; ++i) {
        a.update(1.0 / 30.0, -60.0f);
    }
    for (int i = 0; i < 9 * 144 / 30 + 1; ++i) {
        b.update(1.0 / 144.0, -60.0f);
    }
    CHECK_NEAR(a.heading(), b.heading(), 0.6f);
    // Garbage input changes nothing.
    const float h = a.heading();
    a.update(NAN, -60.0f);
    a.update(0.1, NAN);
    CHECK_NEAR(a.heading(), h, 1e-6f);
}

TEST_CASE("layout: HUD boards stay off the lane and inside the canvas") {
    CHECK(!overlaps(kScoreBoardSlot, kLaneBand));
    CHECK(!overlaps(kTimerBoardSlot, kLaneBand));
    CHECK(inside(kScoreBoardSlot, kCanvas));
    CHECK(inside(kTimerBoardSlot, kCanvas));
    // Timer on the counter, score on the booth header.
    CHECK(kTimerBoardSlot.y >= kCounter.y);
    CHECK(overlaps(kScoreBoardSlot, kHeaderBeam));
    // The rifle (any pose) never covers the timer.
    CHECK(!overlaps(rifleBounds(riflePose({0, 0}, 1)), kTimerBoardSlot));
    CHECK(!overlaps(rifleBounds(riflePose({1920, 0}, 1)), kTimerBoardSlot));
    // The ammo board (D-035): on the counter, clear of the lane, the timer and the rifle.
    CHECK(!overlaps(kAmmoSlot, kLaneBand));
    CHECK(inside(kAmmoSlot, kCanvas));
    CHECK(kAmmoSlot.y >= kCounter.y);
    CHECK(!overlaps(kAmmoSlot, kTimerBoardSlot));
    for (const float x : {0.0f, 480.0f, 960.0f, 1440.0f, 1919.0f}) {
        for (const float y : {0.0f, 540.0f, 1079.0f}) {
            CHECK(!overlaps(rifleBounds(riflePose({x, y}, 1)), kAmmoSlot));
            CHECK(!overlaps(rifleBounds(riflePose({x, y}, 0)), kAmmoSlot));
        }
    }
}

TEST_CASE("layout: header sign and score board share the beam without overlapping") {
    CHECK(!overlaps(kSignSlot, kScoreBoardSlot));
    CHECK(inside(kSignSlot, kHeaderBeam));
}

TEST_CASE("layout: counter fills about the bottom 18-20 % of the frame") {
    constexpr float share = (kLogicalSize.height - kCounter.y) / kLogicalSize.height;
    static_assert(share >= 0.17f);
    static_assert(share <= 0.22f);
}

TEST_CASE("layout: the bulb row spans the opening and bulbs are about 6 cm") {
    CHECK(bulbPosition(0).x > kOpening.x);
    CHECK(bulbPosition(kBulbCount - 1).x < kOpening.x + kOpening.width);
    CHECK(kBulbSize.height >= 20.0f && kBulbSize.height <= 34.0f);
}

TEST_CASE("layout: bulb glow stays in range; only two bulbs flicker") {
    for (std::size_t i = 0; i < kBulbCount; ++i) {
        bool varies = false;
        for (double t = 0.0; t < 20.0; t += 0.01) {
            const float g = bulbGlow(i, t);
            CHECK(g >= 0.78f - 1e-6f);
            CHECK(g <= 1.0f + 1e-6f);
            varies = varies || g < 0.99f;
        }
        CHECK_EQ(varies, i == 3 || i == 8);
    }
}

TEST_CASE("layout: miss marks land only on visible backboard") {
    CHECK(inside(kMissMarkRegion, kBackboard));
    CHECK(kMissMarkRegion.y + kMissMarkRegion.height <= kWaveStrip.y);
}

TEST_CASE("layout: overlaps() is half-open") {
    CHECK(overlaps({0, 0, 10, 10}, {5, 5, 10, 10}));
    CHECK(!overlaps({0, 0, 10, 10}, {10, 0, 10, 10}));   // touching edges
    CHECK(!overlaps({0, 0, 10, 10}, {0, 10, 10, 10}));
}
