#pragma once

// The duck targets (ARCHITECTURE §2.2, §4.3; D-027).
//
//   DuckLane owns the targets' behaviour: motion clocks, the state machine,
//   hit-reaction timing, re-entry. It knows nothing about textures or SFML.
//   DuckPose is the single description of where and how a target is shown;
//   rendering and hit testing both derive from duckImageToLogical(), so the
//   drawn duck and the hittable duck cannot disagree (risk R-3).
//
// State machine (per target):
//
//   Upright ──knockDown──► Falling ──fallTime──► Down ──rise rule──► Rising ──riseTime──► Upright
//   (active: hittable)     (inactive; the carriage keeps moving throughout)
//
//   rise rule: minDownTime elapsed, then (RiseTrigger::AtLaneEnd) the next
//   end stop / wrap, or (AfterDownTime) immediately.
//
// Transitions are resolved at their exact time inside a frame (the leftover
// time carries into the next state), so outcomes do not depend on frame rate.

#include "gallery/core/Affine.hpp"
#include "gallery/core/Geometry.hpp"
#include "gallery/core/Time.hpp"
#include "gallery/game/AlphaMask.hpp"
#include "gallery/game/DuckConfig.hpp"
#include "gallery/game/DuckMotion.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace gallery {

enum class DuckState : std::uint8_t { Upright, Falling, Down, Rising };

[[nodiscard]] std::string_view duckStateName(DuckState s) noexcept;

// One paint variant's silhouette: its logical size and the hit mask built
// from the very image that is drawn. An empty mask means "whole box".
struct DuckShape {
    Size size{};            // logical width x height of the upright silhouette
    Size imageSize{};       // texel size the mask (and texture) is in
    AlphaMask mask;
};

// Where and how a target is shown this instant.
struct DuckPose {
    Vec2 hinge{};           // bottom-centre of the silhouette, on the track line
    float fold = 0.0f;      // 0 upright .. 1 folded flat (backward rotation on the hinge)
    float tiltDeg = 0.0f;   // in-plane rotation about the hinge (clockwise +)
    float sink = 0.0f;      // drop below the track, logical units
    bool hittable = false;
};

// Image texel -> logical. Used by the renderer to draw, and inverted by hit
// testing. The fold is a projected backward rotation: height × cos(fold·90°).
[[nodiscard]] Affine2 duckImageToLogical(const DuckPose& pose, const DuckShape& shape) noexcept;

// The four logical corners of the posed silhouette box (TL, TR, BR, BL).
struct Quad {
    Vec2 p[4];
};
[[nodiscard]] Quad duckQuad(const DuckPose& pose, const DuckShape& shape) noexcept;

struct DuckTransition {
    std::size_t id;
    DuckState from;
    DuckState to;
    Seconds at;             // lane time of the transition
};

class DuckLane {
public:
    // `shapes[variant]` for every variant the config uses. Call validateLane()
    // first; invalid numbers are sanitised here, never trusted.
    DuckLane(LaneConfig config, std::vector<DuckShape> shapes);

    // Advance by dt seconds (negative or NaN = 0).
    void update(Seconds dt);

    // Back to the configured start: positions, states, time; transitions cleared.
    void reset();

    // Hit reaction trigger. Only an upright target can be knocked down;
    // returns false otherwise (and for an invalid id).
    bool knockDown(std::size_t id);

    // Front-most hittable target whose silhouette covers `p` (logical), if any.
    [[nodiscard]] std::optional<std::size_t> hitTest(Vec2 p) const noexcept;

    [[nodiscard]] std::size_t size() const noexcept { return ducks_.size(); }
    [[nodiscard]] Seconds time() const noexcept { return time_; }

    [[nodiscard]] DuckState state(std::size_t id) const noexcept { return ducks_[id].state; }
    // What the target is worth right now (D-035).
    [[nodiscard]] TargetKind kind(std::size_t id) const noexcept { return ducks_[id].kind; }
    [[nodiscard]] bool active(std::size_t id) const noexcept {
        return ducks_[id].state == DuckState::Upright;
    }
    [[nodiscard]] Seconds stateTime(std::size_t id) const noexcept { return ducks_[id].stateTime; }
    [[nodiscard]] MotionSample motion(std::size_t id) const noexcept;
    // Where the carriage was (or will be) at another lane time; e.g. the
    // exact position at a logged transition.
    [[nodiscard]] MotionSample motionAt(std::size_t id, Seconds laneTime) const noexcept;
    [[nodiscard]] DuckPose pose(std::size_t id) const noexcept;
    [[nodiscard]] const DuckShape& shape(std::size_t id) const noexcept;
    [[nodiscard]] const DuckConfig& config(std::size_t id) const noexcept {
        return config_.ducks[id];
    }
    [[nodiscard]] const DuckMotion& motionModel(std::size_t id) const noexcept {
        return ducks_[id].motion;
    }
    // The posed silhouette box (for inspection; hit testing uses the mask).
    [[nodiscard]] Quad quad(std::size_t id) const noexcept { return duckQuad(pose(id), shape(id)); }

    // Transitions since the last clearTransitions(), oldest first.
    [[nodiscard]] const std::vector<DuckTransition>& transitions() const noexcept {
        return transitions_;
    }
    void clearTransitions() noexcept { transitions_.clear(); }

private:
    struct Duck {
        DuckMotion motion;
        double clock = 0.0;
        DuckState state = DuckState::Upright;
        Seconds stateTime = 0.0;
        int hitDirection = +1;   // travel direction when knocked down (TiltDrop tips against it)
        TargetKind kind = TargetKind::Duck;
        double rate = 1.0;       // motion-clock rate (D-036: ordinary ducks run faster)
        Seconds upLimit = 0.0;   // drops by itself after this long upright (0 = never)
        Seconds dropAt = 0.0;    // lane time it is scheduled to leave by itself (when upLimit > 0)
    };

    // Time until target `id`'s next state change, and that state.
    [[nodiscard]] Seconds untilNext(std::size_t id, DuckState& next) const noexcept;
    void enter(std::size_t id, DuckState next, Seconds at);
    // Draw a new kind for target `id` from the mix (limits counted over the
    // other targets). Golden only when `allowGolden`.
    void drawKind(std::size_t id, bool allowGolden, Seconds at);
    float unit() noexcept;   // uniform [0, 1) from the lane's own xorshift

    LaneConfig config_;
    std::vector<DuckShape> shapes_;
    std::vector<Duck> ducks_;
    Seconds time_ = 0.0;
    std::vector<DuckTransition> transitions_;
    std::uint32_t rng_ = 1u;   // seeded once from config_.mix; NOT reset per round (variety)
    Seconds lastGolden_ = 0.0;   // lane time a golden target last came up (round start counts)
};

} // namespace gallery
