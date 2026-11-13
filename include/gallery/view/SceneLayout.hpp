#pragma once

// The booth composition: where every photographic layer sits in the fixed
// 1920x1080 logical frame, and the order layers are drawn in.
//
// One fixed first-person camera (brief §4.1, §7.6). Depth comes from layer
// order, occlusion (booth posts hide the lane ends, the wave cutout hides the
// duck hinges, the counter and rifle sit in front of everything) and from the
// photographs themselves (focus, scale, light). There is no 3D projection.
//
// Pure data and math: no SFML (D-005). The shell's SceneRenderer reads it,
// and tests/scene_layout_tests.cpp checks the brief's layout rules (§6.2).

#include "gallery/core/Geometry.hpp"
#include "gallery/core/Time.hpp"
#include "gallery/view/Viewport.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace gallery {

// ---------------------------------------------------------------------------
// Layer groups: the eight visual layers of the M1 request. Each group can be
// toggled independently (keys 1-8) for inspection.
// ---------------------------------------------------------------------------
enum class LayerGroup : std::uint8_t {
    Environment,   // 1 background plates, booth interior (backboard), booth frame
    Decorations,   // 2 header sign, bulbs, bunting
    Track,         // 3 rail (behind targets) and wave cutout (in front of them)
    Targets,       // 4 ducks and their contact shadows
    Counter,       // 5 foreground shooting counter
    Rifle,         // 6 mounted rifle prop
    Interface,     // 7 crosshair and diegetic HUD boards
    Effects,       // 8 miss marks, muzzle puff, recoil
    Count
};
inline constexpr std::size_t kLayerGroupCount = static_cast<std::size_t>(LayerGroup::Count);

[[nodiscard]] std::string_view layerGroupName(LayerGroup group) noexcept;

// ---------------------------------------------------------------------------
// Draw passes, back to front. A group may own several passes at different
// depths (e.g. the track: rail behind the ducks, wave cutout in front).
// Passes with postProcessed=true are drawn into the post-process target;
// the rest are drawn afterwards at full clarity (D-014).
// ---------------------------------------------------------------------------
enum class DrawPass : std::uint8_t {
    FarPlate,      // BG_FAR
    MidPlate,      // BG_MID
    Backboard,     // FG_BACKBOARD (booth interior back wall)
    MissMarks,     // on the backboard, behind the ducks
    TrackRail,     // FG_TRACK_RAIL
    Ducks,         // TGT_DUCK_* with contact shadows
    WaveStrip,     // FG_WAVE_STRIP hides duck hinges
    BoothFrame,    // FG_BOOTH_FRAME posts hide the lane ends (duck wrap-around)
    Bunting,       // DEC_BUNTING (E1)
    Sign,          // DEC_SIGN_HEADER
    Bulbs,         // DEC_BULB_LIT + DEC_BULB_GLOW
    Counter,       // FG_COUNTER
    Rifle,         // FG_RIFLE (swivel + recoil)
    MuzzlePuff,    // OV_MUZZLE_PUFF
    // ---- post-process boundary ----
    ScoreBoard,    // UI_SCORE_BOARD (pre-graded, D-014)
    TimerBoard,    // UI_TIMER_BOARD
    Crosshair,     // UI_RETICLE — always last
    Count
};

struct PassInfo {
    DrawPass pass;
    LayerGroup group;
    bool postProcessed;
    std::string_view name;
};

inline constexpr std::array<PassInfo, static_cast<std::size_t>(DrawPass::Count)> kDrawPasses{{
    {DrawPass::FarPlate, LayerGroup::Environment, true, "far plate"},
    {DrawPass::MidPlate, LayerGroup::Environment, true, "mid plate"},
    {DrawPass::Backboard, LayerGroup::Environment, true, "backboard"},
    {DrawPass::MissMarks, LayerGroup::Effects, true, "miss marks"},
    {DrawPass::TrackRail, LayerGroup::Track, true, "track rail"},
    {DrawPass::Ducks, LayerGroup::Targets, true, "ducks"},
    {DrawPass::WaveStrip, LayerGroup::Track, true, "wave cutout"},
    {DrawPass::BoothFrame, LayerGroup::Environment, true, "booth frame"},
    {DrawPass::Bunting, LayerGroup::Decorations, true, "bunting"},
    {DrawPass::Sign, LayerGroup::Decorations, true, "header sign"},
    {DrawPass::Bulbs, LayerGroup::Decorations, true, "bulbs"},
    {DrawPass::Counter, LayerGroup::Counter, true, "counter"},
    {DrawPass::Rifle, LayerGroup::Rifle, true, "rifle"},
    {DrawPass::MuzzlePuff, LayerGroup::Effects, true, "muzzle puff"},
    {DrawPass::ScoreBoard, LayerGroup::Interface, false, "score board"},
    {DrawPass::TimerBoard, LayerGroup::Interface, false, "timer board"},
    {DrawPass::Crosshair, LayerGroup::Interface, false, "crosshair"},
}};

// ---------------------------------------------------------------------------
// Composition, in logical units (1920x1080, y down).
// Numbers follow Asset Guide §2.4 (scale table) and brief §6.
// ---------------------------------------------------------------------------
namespace layout {

inline constexpr Rect kCanvas{0.0f, 0.0f, kLogicalSize.width, kLogicalSize.height};

// Shared horizon of every plate (Asset Guide §2.1): 46 % of frame height.
inline constexpr float kHorizonY = 0.46f * kLogicalSize.height;

// Brief §6.2: the target lane sits in 40-60 % of screen height.
inline constexpr Rect kLaneBand{0.0f, 0.40f * kLogicalSize.height, kLogicalSize.width,
                                0.20f * kLogicalSize.height};

// Background plates are 1960x1100 (2 % margin) and centred, so a plate that is
// a few pixels short never exposes the clear colour at the canvas edge.
inline constexpr Rect kFarPlate{-20.0f, -10.0f, 1960.0f, 1100.0f};
inline constexpr Rect kMidPlate = kFarPlate;

// Booth structure. The frame image is full-canvas with a transparent opening.
// The rectangles below are where its opaque parts are expected; they drive
// the stand-in silhouette and the inspection overlay.
inline constexpr Rect kBoothFrame = kCanvas;
inline constexpr Rect kAwning{0.0f, 50.0f, kLogicalSize.width, 60.0f};       // striped edge
inline constexpr Rect kHeaderBeam{40.0f, 110.0f, 1840.0f, 180.0f};           // carries the sign
inline constexpr Rect kLeftPost{60.0f, 110.0f, 140.0f, 750.0f};
inline constexpr Rect kRightPost{1720.0f, 110.0f, 140.0f, 750.0f};
// The playable opening between the posts, below the header, above the counter.
inline constexpr Rect kOpening{200.0f, 290.0f, 1520.0f, 570.0f};

// Booth interior back wall: fills the opening (Cover crop of a plank photo).
inline constexpr Rect kBackboard = kOpening;

// Header decorations.
inline constexpr Rect kSignSlot{250.0f, 118.0f, 1040.0f, 164.0f};            // Contain
inline constexpr Rect kBuntingSlot{200.0f, 290.0f, 1520.0f, 70.0f};          // E1, FitWidth
inline constexpr float kBulbRowY = 290.0f;                                   // sockets hang from the beam
inline constexpr std::size_t kBulbCount = 11;
inline constexpr float kBulbFirstX = 230.0f;
inline constexpr float kBulbSpacing = 146.0f;
inline constexpr Size kBulbSize{24.0f, 32.0f};                               // ≈ 6 cm bulb
inline constexpr float kBulbGlowDiameter = 150.0f;

// Target lane.
inline constexpr float kDuckHeight = 105.0f;                                 // ≈ 9.7 % of height
inline constexpr float kDuckStandInAspect = 1.22f;                           // width / height
inline constexpr float kTrackY = 600.0f;                                     // duck hinge line
inline constexpr Rect kTrackRail{200.0f, 587.0f, 1520.0f, 18.0f};            // tiled strip; fully behind the solid wave board (D-040)
inline constexpr Rect kWaveStrip{60.0f, 570.0f, 1800.0f, 155.0f};            // FitWidth, post edge to post edge
// Wave crests sit on kWaveStrip.y; troughs dip this far below it. The cutout
// is solid from (kWaveStrip.y + kWaveTroughDepth) to its bottom edge, so a
// target is fully hidden "behind the track" only inside that band.
inline constexpr float kWaveTroughDepth = 15.0f;   // D-040: shallower, so even a trough covers the duck's base
inline constexpr float kWaveSolidTop = kWaveStrip.y + kWaveTroughDepth;
inline constexpr Vec2 kDuckShadowOffset{3.0f, 7.0f};                         // light above-front
inline constexpr float kDuckShadowAlpha = 0.32f;

// Chain preset (brief G-14, ARCHITECTURE §4.3): ducks wrap while fully hidden
// behind a post. Left-edge x runs over [kLoopStart, kLoopStart+kLoopLength).
// Target motion itself lives in gallery/game (DuckLane, D-027).
inline constexpr float kLoopStart = kLeftPost.x + 10.0f;                     // inside the left post
inline constexpr float kLoopLength = kRightPost.x - kLoopStart;              // wraps at the right post

// Foreground.
inline constexpr Rect kCounter{0.0f, 860.0f, kLogicalSize.width, 220.0f};    // FitWidth, ≈ bottom 20 %
inline constexpr Rect kScoreBoardSlot{1400.0f, 116.0f, 400.0f, 168.0f};      // top right, on the booth
inline constexpr Rect kTimerBoardSlot{70.0f, 880.0f, 300.0f, 180.0f};        // bottom left, on the counter
inline constexpr Rect kAmmoSlot{392.0f, 944.0f, 340.0f, 116.0f};             // magazine, on the counter beside the timer (D-035)

// Region where a missed shot leaves a mark on the backboard (above the wave cutout).
inline constexpr Rect kMissMarkRegion{kOpening.x, kOpening.y, kOpening.width,
                                      kWaveStrip.y - kOpening.y};
inline constexpr float kMissMarkSize = 20.0f;
inline constexpr float kMuzzlePuffSize = 150.0f;
inline constexpr float kReticleSize = 64.0f;

// Rifle. The image's pivot (swivel mount) and muzzle are recorded per asset
// in the manifest (FG_RIFLE); these defaults describe the 900x500 spec frame
// and the stand-in silhouette.
inline constexpr Size kRifleImageSize{900.0f, 500.0f};
inline constexpr Vec2 kRiflePivotInImage{620.0f, 380.0f};
inline constexpr Vec2 kRifleMuzzleInImage{30.0f, 70.0f};
// First-person view down the barrel (D-039). The swivel sits low and close to
// the eye, just right of centre; the barrel points at the aim and recedes in
// perspective (it narrows toward the muzzle), and the stock runs off the
// bottom of the screen toward the shooter.
inline constexpr Vec2 kRiflePivot{1170.0f, 1085.0f};          // swivel on screen, at the bottom edge
inline constexpr float kRifleReach = 340.0f;                  // swivel -> muzzle on screen, logical units
inline constexpr float kRifleScale = 1.6f;                    // image units -> logical, across the barrel, at the swivel
inline constexpr float kRifleDepth = 0.9f;                    // perspective: the muzzle is drawn 1/(1+depth) as wide
// Asset rule (D-039): the rifle silhouette lies within this many image units
// (900x500 spec frame) of the swivel->muzzle line; bounds and layout checks
// use this corridor, not the whole frame.
inline constexpr float kRifleCorridor = 70.0f;
inline constexpr float kRifleHeadingMinDeg = -150.0f;         // barrel heading limits on screen (y down;
inline constexpr float kRifleHeadingMaxDeg = -30.0f;          // -90 = straight up), so it never points down
inline constexpr float kRifleRecoilBack = 26.0f;              // kick straight back along the barrel, toward the eye
inline constexpr float kRifleRecoilClimbDeg = 2.5f;           // and the muzzle climbs a little
// The mount is stiff: the barrel follows the aim through a damped spring
// instead of snapping to it (visual only; the shot still lands at the reticle).
inline constexpr float kRifleStiffness = 11.0f;               // natural frequency, rad/s (~0.3 s to settle)
inline constexpr float kRifleDamping = 0.9f;                  // damping ratio: a hint of settle, no wobble

} // namespace layout

// Logical rect of an upright duck whose left edge is `left`, given its width.
[[nodiscard]] Rect duckRect(float left, float width) noexcept;

// ---------------------------------------------------------------------------
// Rifle pose (D-039): first person, barrel pointing at the aim, plus recoil.
// ---------------------------------------------------------------------------
struct RiflePose {
    float headingDeg = -90.0f;   // barrel direction on screen (y down; -90 = straight up)
    float recoil = 0.0f;         // 0 at rest .. 1 the instant of firing
};

// Heading from the swivel straight at `aim`, within the heading limits.
[[nodiscard]] float rifleHeadingToward(Vec2 aim) noexcept;

// The barrel pointing exactly at `aim`; `recoil` clamped to [0,1].
[[nodiscard]] RiflePose riflePose(Vec2 aim, float recoil) noexcept;

// Logical position of an image-space point of the rifle under `pose`: the
// image's swivel->muzzle axis is laid along the heading and drawn in
// perspective (a projective map, so straight edges stay straight).
[[nodiscard]] Vec2 rifleImageToLogical(Vec2 imagePoint, const RiflePose& pose,
                                       Vec2 pivotInImage = layout::kRiflePivotInImage,
                                       Vec2 muzzleInImage = layout::kRifleMuzzleInImage) noexcept;

// The same map in barrel coordinates: `along` 0 at the swivel, 1 at the muzzle
// (negative toward the viewer), `across` in image units either side of the
// barrel line. The stand-in is drawn directly in these coordinates.
[[nodiscard]] Vec2 rifleAxisToLogical(float along, float across, const RiflePose& pose) noexcept;

// Axis-aligned bounds of the ON-SCREEN part of the rifle corridor (D-039).
[[nodiscard]] Rect rifleBounds(const RiflePose& pose,
                               Size imageSize = layout::kRifleImageSize,
                               Vec2 pivotInImage = layout::kRiflePivotInImage,
                               Vec2 muzzleInImage = layout::kRifleMuzzleInImage) noexcept;

// The stiff swivel mount: the drawn heading chases the aim heading through a
// critically-ish damped spring. Frame-rate robust (fixed 2 ms substeps).
class RifleMount {
public:
    void reset(float headingDeg) noexcept {
        heading_ = headingDeg;
        velocity_ = 0.0f;
    }
    void update(Seconds dt, float targetHeadingDeg) noexcept;
    [[nodiscard]] float heading() const noexcept { return heading_; }
    [[nodiscard]] float velocity() const noexcept { return velocity_; }   // deg/s

private:
    float heading_ = -90.0f;
    float velocity_ = 0.0f;
    Seconds carry_ = 0.0;   // time not yet simulated (under one substep)
};

// ---------------------------------------------------------------------------
// Bulbs: a lit bulb's glow strength in [0.78, 1]. Most bulbs are steady;
// two flicker subtly (brief §7.2). Deterministic in (index, t).
// ---------------------------------------------------------------------------
[[nodiscard]] float bulbGlow(std::size_t index, Seconds t) noexcept;
[[nodiscard]] Vec2 bulbPosition(std::size_t index) noexcept;   // socket centre

// ---------------------------------------------------------------------------
// Atmosphere (D-031). Deterministic in t; all subtle by construction.
// ---------------------------------------------------------------------------

// Overall level of the booth's incandescent light, in [kAmbientLightMin, 1]:
// 1 almost all the time. Two things lower it slightly:
//   - the two tired bulbs (bulbGlow 3 and 8) take a little of the light with them;
//   - about every 25 s (10-40 s apart) the generator sags for a quarter second,
//     every filament dims a few percent together and recovers with thermal lag.
inline constexpr float kAmbientLightMin = 0.90f;
[[nodiscard]] float ambientLight(Seconds t) noexcept;

// The flash of a shot as a warm exposure lift of the whole booth: 1 at the
// instant of firing, 0 after kMuzzleFlashTime (about three frames at 60 Hz).
// A cork or air rifle makes little visible flame, so it is a brief brightening,
// never a drawn glow (D-026); a photographed flash (OV_MUZZLE_FLASH) is added
// on top only if one exists.
inline constexpr Seconds kMuzzleFlashTime = 0.055;
[[nodiscard]] float muzzleFlash(Seconds sinceShot) noexcept;

// Bunting (E1 photograph) stirring in a light evening breeze: a slow sway
// about the top edge, under half a degree, plus a pixel or two of drift.
struct Sway {
    float dx;         // logical units
    float angleDeg;   // about the top-centre of the bunting slot
};
[[nodiscard]] Sway buntingSway(Seconds t) noexcept;

// Rect overlap test (half-open rects; touching edges do not overlap).
[[nodiscard]] bool overlaps(Rect a, Rect b) noexcept;

} // namespace gallery
