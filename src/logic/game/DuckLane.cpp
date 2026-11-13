#include "gallery/game/DuckLane.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <utility>

namespace gallery {

namespace {

constexpr float kPi = std::numbers::pi_v<float>;
constexpr Seconds kNever = std::numeric_limits<Seconds>::infinity();

// FlipBack: the cutout slams flat against its stop and bounces once.
constexpr Seconds kReboundTime = 0.12;
constexpr float kReboundLift = 0.12f;      // fraction of the fold
// TiltDrop: the hit knocks the cutout askew; it straightens as it drops
// into the slot behind the wave board (a tipped box would not fit there).
// 4° keeps the top corner within half a section gap (10 units) of its bounds,
// so a reacting target never swings into its neighbour or behind a post.
constexpr float kTipDeg = 4.0f;

float smoothstep01(float u) noexcept {
    u = std::clamp(u, 0.0f, 1.0f);
    return u * u * (3.0f - 2.0f * u);
}

Seconds sane(Seconds v, Seconds lo, Seconds fallback) noexcept {
    return std::isfinite(v) ? std::max(v, lo) : fallback;
}

} // namespace

std::string_view duckStateName(DuckState s) noexcept {
    switch (s) {
    case DuckState::Upright: return "Upright";
    case DuckState::Falling: return "Falling";
    case DuckState::Down: return "Down";
    case DuckState::Rising: return "Rising";
    }
    return "?";
}

Affine2 duckImageToLogical(const DuckPose& pose, const DuckShape& shape) noexcept {
    const float w = shape.size.width;
    const float h = shape.size.height;
    const float iw = shape.imageSize.width > 0.0f ? shape.imageSize.width : w;
    const float ih = shape.imageSize.height > 0.0f ? shape.imageSize.height : h;
    // Image texel -> local frame: origin at the hinge (bottom centre), y down,
    // the silhouette spans x in [-w/2, w/2], y in [-h, 0].
    const Affine2 imageToLocal = Affine2::translation({-w / 2.0f, -h}) *
                                 Affine2::scaling(w / iw, h / ih);
    // Backward fold on the hinge, seen from the front: height x cos(angle).
    const float fold = std::clamp(pose.fold, -1.0f, 1.0f);
    const float k = std::cos(fold * kPi / 2.0f);
    const Affine2 localToLogical = Affine2::translation(pose.hinge + Vec2{0.0f, pose.sink}) *
                                   Affine2::rotationDeg(pose.tiltDeg) *
                                   Affine2::scaling(1.0f, k);
    return localToLogical * imageToLocal;
}

Quad duckQuad(const DuckPose& pose, const DuckShape& shape) noexcept {
    const Affine2 xf = duckImageToLogical(pose, shape);
    const float iw = shape.imageSize.width > 0.0f ? shape.imageSize.width : shape.size.width;
    const float ih = shape.imageSize.height > 0.0f ? shape.imageSize.height : shape.size.height;
    return Quad{{xf.apply({0.0f, 0.0f}), xf.apply({iw, 0.0f}), xf.apply({iw, ih}),
                 xf.apply({0.0f, ih})}};
}

DuckLane::DuckLane(LaneConfig config, std::vector<DuckShape> shapes)
    : config_(std::move(config)), shapes_(std::move(shapes)) {
    if (shapes_.empty()) {
        shapes_.push_back(DuckShape{{100.0f, 100.0f}, {100.0f, 100.0f}, {}});
    }
    for (DuckConfig& c : config_.ducks) {
        if (c.variant >= shapes_.size()) {
            c.variant = 0;
        }
        c.fallTime = sane(c.fallTime, 1e-3, 0.2);
        c.riseTime = sane(c.riseTime, 1e-3, 0.25);
        c.minDownTime = sane(c.minDownTime, 0.0, 2.0);
        c.startDirection = c.startDirection < 0 ? -1 : +1;
    }
    auto chance = [](float c) { return std::isfinite(c) ? std::clamp(c, 0.0f, 1.0f) : 0.0f; };
    config_.mix.decoyChance = chance(config_.mix.decoyChance);
    config_.mix.goldenChance = chance(config_.mix.goldenChance);
    config_.mix.duckSpeed = std::isfinite(config_.mix.duckSpeed) ? std::clamp(config_.mix.duckSpeed, 0.1f, 4.0f) : 1.0f;
    config_.mix.upTime = sane(config_.mix.upTime, 0.0, 0.0);
    config_.mix.decoyUpTime = sane(config_.mix.decoyUpTime, 0.0, 0.0);
    config_.mix.goldenEvery = sane(config_.mix.goldenEvery, 0.0, 0.0);
    config_.mix.goldenUpTime = sane(config_.mix.goldenUpTime, 0.0, 0.0);
    config_.mix.minUpTime = sane(config_.mix.minUpTime, 0.0, 0.0);
    config_.mix.dropGap = sane(config_.mix.dropGap, 0.0, 0.0);
    config_.mix.upTimeRamp = chance(config_.mix.upTimeRamp);
    rng_ = config_.mix.seed != 0u ? config_.mix.seed : 1u;
    ducks_.resize(config_.ducks.size());
    transitions_.reserve(64);
    reset();
}

void DuckLane::reset() {
    for (std::size_t i = 0; i < ducks_.size(); ++i) {
        const DuckConfig& c = config_.ducks[i];
        Duck& d = ducks_[i];
        d.motion = DuckMotion(c, shapes_[c.variant].size.width);
        d.clock = d.motion.startClock();
        d.state = DuckState::Upright;
        d.stateTime = 0.0;
        d.hitDirection = +1;
    }
    // Fresh kinds for the new round (no golden at the start: it is a reward
    // that turns up later). Drawn in order, so the per-lane limits hold.
    for (std::size_t i = 0; i < ducks_.size(); ++i) {
        ducks_[i].kind = TargetKind::Duck;
        ducks_[i].upLimit = 0.0;   // nothing scheduled yet: draws below see a clean lane
    }
    for (std::size_t i = 0; i < ducks_.size(); ++i) {
        drawKind(i, false, 0.0);
    }
    time_ = 0.0;
    lastGolden_ = 0.0;
    transitions_.clear();
}

void DuckLane::update(Seconds dt) {
    if (!(dt > 0.0) || !std::isfinite(dt)) {
        return;
    }
    // All targets advance together, from one transition to the next in time
    // order (D-036). Transitions at the same instant resolve in id order. So
    // the shared kind generator is drawn in the same order at any frame rate,
    // and every transition happens at its exact time.
    constexpr Seconds kDueEps = 1e-12;
    Seconds elapsed = 0.0;
    const std::size_t guardMax = 64 + 16 * ducks_.size();
    for (std::size_t guard = 0; guard < guardMax; ++guard) {
        Seconds step = dt - elapsed;
        for (std::size_t i = 0; i < ducks_.size(); ++i) {
            DuckState n{};
            step = std::min(step, untilNext(i, n));
        }
        step = std::max(step, 0.0);
        for (Duck& d : ducks_) {
            d.clock += step * d.rate;   // motion clock runs at the target's own rate
            d.stateTime += step;
        }
        elapsed += step;
        for (std::size_t i = 0; i < ducks_.size(); ++i) {
            DuckState n{};
            if (untilNext(i, n) <= kDueEps) {
                if (n == DuckState::Falling) {   // escaped unhit: tips against its travel
                    ducks_[i].hitDirection = ducks_[i].motion.sample(ducks_[i].clock).direction;
                }
                enter(i, n, time_ + elapsed);
            }
        }
        if (elapsed >= dt) {
            break;
        }
    }
    time_ += dt;
}

Seconds DuckLane::untilNext(std::size_t id, DuckState& next) const noexcept {
    const Duck& d = ducks_[id];
    const DuckConfig& c = config_.ducks[id];
    switch (d.state) {
    case DuckState::Upright:
        next = DuckState::Falling;   // not hit in time: ducks down unscored (D-036)
        return d.upLimit > 0.0 ? std::max(0.0, d.upLimit - d.stateTime) : kNever;
    case DuckState::Falling:
        next = DuckState::Down;
        return std::max(0.0, c.fallTime - d.stateTime);
    case DuckState::Down: {
        next = DuckState::Rising;
        const Seconds eligibleIn = std::max(0.0, c.minDownTime - d.stateTime);
        return c.rise == RiseTrigger::AfterDownTime
                   ? eligibleIn
                   : eligibleIn + d.motion.timeToEdge(d.clock + eligibleIn * d.rate) / d.rate;
    }
    case DuckState::Rising:
        next = DuckState::Upright;
        return std::max(0.0, c.riseTime - d.stateTime);
    }
    next = d.state;
    return kNever;
}

void DuckLane::enter(std::size_t id, DuckState next, Seconds at) {
    Duck& d = ducks_[id];
    transitions_.push_back({id, d.state, next, at});
    d.state = next;
    d.stateTime = 0.0;
    if (next == DuckState::Rising) {
        drawKind(id, true, at);   // it comes back up as a new target
    }
}

bool DuckLane::knockDown(std::size_t id) {
    if (id >= ducks_.size() || ducks_[id].state != DuckState::Upright) {
        return false;
    }
    ducks_[id].hitDirection = motion(id).direction;
    enter(id, DuckState::Falling, time_);
    return true;
}

MotionSample DuckLane::motion(std::size_t id) const noexcept {
    const Duck& d = ducks_[id];
    return d.motion.sample(d.clock);
}

MotionSample DuckLane::motionAt(std::size_t id, Seconds laneTime) const noexcept {
    const Duck& d = ducks_[id];
    return d.motion.sample(d.clock + (laneTime - time_) * d.rate);
}

const DuckShape& DuckLane::shape(std::size_t id) const noexcept {
    return shapes_[config_.ducks[id].variant];
}

DuckPose DuckLane::pose(std::size_t id) const noexcept {
    const Duck& d = ducks_[id];
    const DuckConfig& c = config_.ducks[id];
    const DuckShape& s = shape(id);
    const MotionSample m = d.motion.sample(d.clock);

    DuckPose p;
    p.hinge = {m.left + s.size.width / 2.0f, c.trackY};
    p.tiltDeg = d.motion.wobbleAt(m.left);
    p.hittable = d.state == DuckState::Upright;

    const float t = static_cast<float>(d.stateTime);
    const float fallU = static_cast<float>(d.stateTime / c.fallTime);
    const float riseU = static_cast<float>(d.stateTime / c.riseTime);
    if (c.reaction == HitReaction::FlipBack) {
        switch (d.state) {
        case DuckState::Upright: p.fold = 0.0f; break;
        case DuckState::Falling: p.fold = std::min(1.0f, fallU * fallU); break;   // gravity
        case DuckState::Down: {
            const float r = std::min(1.0f, t / static_cast<float>(kReboundTime));
            p.fold = 1.0f - kReboundLift * std::sin(kPi * r);                     // one bounce
            break;
        }
        case DuckState::Rising: p.fold = 1.0f - smoothstep01(riseU); break;
        }
    } else {   // TiltDrop
        const float tip = -static_cast<float>(d.hitDirection) * kTipDeg;   // top lags the travel
        const float drop = c.dropDepth;
        switch (d.state) {
        case DuckState::Upright: break;
        case DuckState::Falling: {
            const float u = std::min(1.0f, fallU);
            p.tiltDeg += tip * std::sin(kPi * u);   // jolts askew, straightens into the slot
            p.sink = drop * u * u;
            break;
        }
        case DuckState::Down: p.sink = drop; break;
        case DuckState::Rising: p.sink = drop * (1.0f - smoothstep01(riseU)); break;
        }
    }
    return p;
}

float DuckLane::unit() noexcept {
    rng_ ^= rng_ << 13;
    rng_ ^= rng_ >> 17;
    rng_ ^= rng_ << 5;
    return static_cast<float>(rng_ & 0xFFFFFFu) / 16777216.0f;
}

void DuckLane::drawKind(std::size_t id, bool allowGolden, Seconds at) {
    const TargetMix& m = config_.mix;
    int decoys = 0;
    int golden = 0;
    for (std::size_t i = 0; i < ducks_.size(); ++i) {
        if (i != id) {
            decoys += ducks_[i].kind == TargetKind::Decoy ? 1 : 0;
            golden += ducks_[i].kind == TargetKind::Golden ? 1 : 0;
        }
    }
    const float u = unit();   // always one draw per call: the sequence never depends on limits
    TargetKind k = TargetKind::Duck;
    const bool overdue = m.goldenEvery > 0.0 && at - lastGolden_ >= m.goldenEvery;
    if (allowGolden && golden < m.maxGolden && (u < m.goldenChance || overdue)) {
        k = TargetKind::Golden;
        lastGolden_ = at;
    } else if (decoys < m.maxDecoys && u >= m.goldenChance && u < m.goldenChance + m.decoyChance) {
        k = TargetKind::Decoy;
    }
    Duck& d = ducks_[id];
    d.kind = k;
    d.rate = k == TargetKind::Duck ? static_cast<double>(m.duckSpeed) : 1.0;
    const float v = unit();   // second draw, always taken
    d.upLimit = 0.0;
    const Seconds base = k == TargetKind::Decoy                          ? m.decoyUpTime
                         : k == TargetKind::Golden && m.goldenUpTime > 0.0 ? m.goldenUpTime
                                                                         : m.upTime;
    if (base <= 0.0) {
        return;
    }
    // D-038 pacing. The window shrinks as the round goes on (not for decoys),
    // keeps a fair floor, and is placed so that no two targets on the lane
    // are due to leave within dropGap of each other.
    const double ramp = k == TargetKind::Decoy
                            ? 0.0
                            : std::min(0.4, static_cast<double>(m.upTimeRamp) * std::max(0.0, at) / 60.0);
    const Seconds lo = std::max(m.minUpTime, base * 0.6 * (1.0 - ramp));
    const Seconds hi = std::max(lo, base * 1.4 * (1.0 - ramp));
    const Seconds uprightAt = d.state == DuckState::Rising ? at + config_.ducks[id].riseTime : at;
    auto clashes = [&](Seconds dropAt) {
        for (std::size_t i = 0; i < ducks_.size(); ++i) {
            const Duck& o = ducks_[i];
            const bool scheduled = i != id && o.upLimit > 0.0 &&
                                   (o.state == DuckState::Upright || o.state == DuckState::Rising);
            if (scheduled && std::abs(o.dropAt - dropAt) < m.dropGap) {
                return true;
            }
        }
        return false;
    };
    // Candidates walk the window in golden-ratio steps from the random draw:
    // well spread, deterministic, and no extra draws from the generator.
    Seconds chosen = lo + v * (hi - lo);
    for (int n = 0; n < 16; ++n) {
        double f = static_cast<double>(v) + 0.6180339887498949 * n;
        f -= std::floor(f);
        const Seconds cand = lo + f * (hi - lo);
        if (!clashes(uprightAt + cand)) {
            chosen = cand;
            break;
        }
    }
    d.upLimit = chosen;
    d.dropAt = uprightAt + chosen;
}

std::optional<std::size_t> DuckLane::hitTest(Vec2 p) const noexcept {
    for (std::size_t n = ducks_.size(); n-- > 0;) {   // front-most first
        const DuckPose pose = this->pose(n);
        if (!pose.hittable) {
            continue;
        }
        const DuckShape& s = shape(n);
        const auto inv = duckImageToLogical(pose, s).inverse();
        if (!inv) {
            continue;
        }
        const Vec2 q = inv->apply(p);
        bool solid = false;
        if (s.mask.empty()) {
            const float iw = s.imageSize.width > 0.0f ? s.imageSize.width : s.size.width;
            const float ih = s.imageSize.height > 0.0f ? s.imageSize.height : s.size.height;
            solid = q.x >= 0.0f && q.y >= 0.0f && q.x < iw && q.y < ih;
        } else {
            solid = s.mask.solidAt(static_cast<int>(std::floor(q.x)),
                                   static_cast<int>(std::floor(q.y)));
        }
        if (solid) {
            return n;
        }
    }
    return std::nullopt;
}

} // namespace gallery
