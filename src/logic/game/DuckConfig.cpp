#include "gallery/game/DuckConfig.hpp"

#include "gallery/view/SceneLayout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>

namespace gallery {

std::string_view lanePresetName(LanePreset p) noexcept {
    switch (p) {
    case LanePreset::Sections: return "sections";
    case LanePreset::Chain: return "chain";
    }
    return "?";
}

const char* targetKindName(TargetKind k) noexcept {
    switch (k) {
    case TargetKind::Duck: return "duck";
    case TargetKind::Decoy: return "decoy";
    case TargetKind::Golden: return "golden";
    }
    return "?";
}

TargetMix standardMix() noexcept {
    TargetMix m;
    m.decoyChance = 0.25f;
    m.goldenChance = 0.05f;   // D-036: rarer (was 0.08)
    m.duckSpeed = 1.35f;      // D-036: ordinary ducks run faster
    // D-038 pacing: ducks 1.8-4.2 s, gold 1.3-3.1 s (fleeting), decoys 2.4-5.6 s;
    // duck and gold windows shrink up to ~25 % over a 60 s round, never below
    // 1.3 s, and no two targets leave within 0.4 s of each other.
    m.upTime = 3.0;
    m.goldenUpTime = 2.2;
    m.decoyUpTime = 4.0;
    m.upTimeRamp = 0.25f;
    m.minUpTime = 1.3;
    m.dropGap = 0.4;
    m.goldenEvery = 15.0;     // D-037: an ammo (gold) duck at least every ~15 s
    return m;
}

namespace {

// ---------------------------------------------------------------------------
// Sections preset (D-027): the rail across the booth opening is split into
// five equal sections, one shuttle carriage each, with a small gap between
// sections so neighbours never touch. Per-target numbers vary by a few
// percent only: same machine, slightly different wear (brief §7.2 "not
// cloned"). All values are deterministic; there is no randomness at runtime.
// ---------------------------------------------------------------------------
constexpr std::size_t kSections = 5;
constexpr float kSectionGap = 20.0f;
//                                       #0     #1     #2     #3     #4
constexpr std::array<float, 5> kSpeed{118.0f, 131.0f, 112.0f, 126.0f, 121.0f};   // ±8 % of ~122
constexpr std::array<float, 5> kTurn{0.30f, 0.34f, 0.28f, 0.32f, 0.30f};
constexpr std::array<float, 5> kDwell{0.18f, 0.30f, 0.12f, 0.24f, 0.20f};
constexpr std::array<double, 5> kPhase{0.00, 0.37, 0.71, 0.18, 0.55};
constexpr std::array<int, 5> kDirection{+1, -1, +1, -1, +1};
constexpr std::array<double, 5> kMinDown{2.0, 2.3, 2.1, 2.4, 2.2};

// TiltDrop: sink until the top of the silhouette is below the wave troughs,
// i.e. fully behind the solid part of the wave cutout (layout::kWaveSolidTop).
constexpr float kDropDepth = layout::kWaveSolidTop - (layout::kTrackY - layout::kDuckHeight) + 2.0f;

LaneConfig sections() {
    using namespace layout;
    LaneConfig lane;
    const float sectionWidth = kOpening.width / static_cast<float>(kSections);
    for (std::size_t i = 0; i < kSections; ++i) {
        DuckConfig c;
        c.mode = MotionMode::Shuttle;
        c.boundsMin = kOpening.x + static_cast<float>(i) * sectionWidth + kSectionGap / 2.0f;
        c.boundsMax = kOpening.x + static_cast<float>(i + 1) * sectionWidth - kSectionGap / 2.0f;
        c.trackY = kTrackY;
        c.speed = kSpeed[i];
        c.turnTime = kTurn[i];
        c.dwell = kDwell[i];
        c.phase = kPhase[i];
        c.startDirection = kDirection[i];
        c.reaction = HitReaction::FlipBack;
        c.rise = RiseTrigger::AtLaneEnd;   // pops back up when its carriage reaches a stop
        c.minDownTime = kMinDown[i];
        c.dropDepth = kDropDepth;
        c.variant = static_cast<std::uint8_t>(i % 3);
        lane.ducks.push_back(c);
    }
    return lane;
}

// ---------------------------------------------------------------------------
// Chain preset: brief G-12..G-15 / ARCHITECTURE §4.3 exactly. One chain, five
// evenly spaced ducks, 25 % of the lane width per second, left to right,
// wrapping while hidden behind the posts.
// ---------------------------------------------------------------------------
LaneConfig chain() {
    using namespace layout;
    LaneConfig lane;
    for (std::size_t i = 0; i < kSections; ++i) {
        DuckConfig c;
        c.mode = MotionMode::Wrap;
        c.boundsMin = kLoopStart;
        c.boundsMax = kLoopStart + kLoopLength;
        c.trackY = kTrackY;
        c.speed = 0.25f * kOpening.width;
        c.phase = static_cast<double>(i) / static_cast<double>(kSections);
        c.startDirection = +1;
        c.reaction = HitReaction::FlipBack;
        c.rise = RiseTrigger::AtLaneEnd;
        c.minDownTime = 2.0;
        c.dropDepth = kDropDepth;
        c.variant = static_cast<std::uint8_t>(i % 3);
        lane.ducks.push_back(c);
    }
    return lane;
}

bool finite(double v) noexcept {
    return std::isfinite(v);
}

} // namespace

LaneConfig makeLaneConfig(LanePreset preset) {
    return preset == LanePreset::Chain ? chain() : sections();
}

void setReaction(LaneConfig& config, HitReaction reaction) noexcept {
    for (DuckConfig& c : config.ducks) {
        c.reaction = reaction;
    }
}

std::vector<std::string> validateLane(const LaneConfig& config,
                                      const std::vector<float>& duckWidths, float duckHeight,
                                      Rect visible) {
    std::vector<std::string> issues;
    if (config.ducks.empty()) {
        issues.emplace_back("lane has no targets");
        return issues;
    }
    const float visRight = visible.x + visible.width;
    auto widthOf = [&](const DuckConfig& c) {
        return c.variant < duckWidths.size() ? duckWidths[c.variant] : 0.0f;
    };

    for (std::size_t i = 0; i < config.ducks.size(); ++i) {
        const DuckConfig& c = config.ducks[i];
        auto bad = [&](std::string what) { issues.push_back(std::format("duck #{}: {}", i, what)); };
        if (c.variant >= duckWidths.size()) {
            bad(std::format("paint variant {} has no shape", c.variant));
            continue;
        }
        const float w = widthOf(c);
        if (!finite(c.speed) || c.speed <= 0.0f) bad("speed must be > 0");
        if (!finite(c.turnTime) || c.turnTime < 0.0f) bad("turnTime must be >= 0");
        if (!finite(c.dwell) || c.dwell < 0.0f) bad("dwell must be >= 0");
        if (!finite(c.phase) || c.phase < 0.0 || c.phase >= 1.0) bad("phase must be in [0, 1)");
        if (c.startDirection != 1 && c.startDirection != -1) bad("startDirection must be +1 or -1");
        if (!finite(c.fallTime) || c.fallTime <= 0.0) bad("fallTime must be > 0");
        if (!finite(c.riseTime) || c.riseTime <= 0.0) bad("riseTime must be > 0");
        if (!finite(c.minDownTime) || c.minDownTime < 0.0) bad("minDownTime must be >= 0");
        if (!finite(c.dropDepth) || c.dropDepth < 0.0f) bad("dropDepth must be >= 0");
        if (!finite(c.wobbleDeg) || std::fabs(c.wobbleDeg) > 3.0f) bad("wobbleDeg must be within ±3°");
        if (c.trackY - duckHeight < visible.y || c.trackY > visible.y + visible.height) {
            bad("track line puts the silhouette outside the gallery opening");
        }
        if (c.mode == MotionMode::Shuttle) {
            if (c.boundsMin < visible.x || c.boundsMax > visRight) {
                bad(std::format("bounds [{:.0f}, {:.0f}] leave the visible gallery [{:.0f}, {:.0f}]",
                                c.boundsMin, c.boundsMax, visible.x, visRight));
            }
            if (c.boundsMax - c.boundsMin <= w) {
                bad(std::format("bounds width {:.0f} leaves no travel for a {:.0f}-wide target",
                                c.boundsMax - c.boundsMin, w));
            }
        } else {
            if (c.boundsMax <= c.boundsMin) bad("loop has no length");
            if (c.boundsMin + w > visible.x) bad("wrap entry point is visible (must start hidden)");
            if (c.boundsMax < visRight) bad("wrap exit point is visible (must end hidden)");
        }
    }

    // Pairwise: targets that share a track must never collide.
    for (std::size_t i = 0; i < config.ducks.size(); ++i) {
        for (std::size_t j = i + 1; j < config.ducks.size(); ++j) {
            const DuckConfig& a = config.ducks[i];
            const DuckConfig& b = config.ducks[j];
            if (std::fabs(a.trackY - b.trackY) > 1.0f || a.variant >= duckWidths.size() ||
                b.variant >= duckWidths.size()) {
                continue;
            }
            const auto pair = std::format("ducks #{} and #{}", i, j);
            if (a.mode != b.mode) {
                issues.push_back(pair + ": shuttle and chain targets share a track and can collide");
            } else if (a.mode == MotionMode::Shuttle) {
                if (a.boundsMin < b.boundsMax && b.boundsMin < a.boundsMax) {
                    issues.push_back(pair + ": shuttle sections overlap, so they can collide");
                }
            } else {
                if (std::fabs(a.speed - b.speed) > 1e-3f || a.startDirection != b.startDirection ||
                    a.boundsMin != b.boundsMin || a.boundsMax != b.boundsMax) {
                    issues.push_back(pair + ": chain targets must share one loop, speed and "
                                            "direction (a chain cannot stretch)");
                    continue;
                }
                const double loop = static_cast<double>(a.boundsMax) - a.boundsMin;
                double gap = std::fabs(a.phase - b.phase);
                gap = std::min(gap, 1.0 - gap) * loop;
                if (gap < std::max(widthOf(a), widthOf(b))) {
                    issues.push_back(pair + ": chain spacing is narrower than a target");
                }
            }
        }
    }
    return issues;
}

} // namespace gallery
