#include "gallery/game/DuckMotion.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace gallery {

namespace {

constexpr double kInf = std::numeric_limits<double>::infinity();
constexpr double kEdgeEps = 1e-9;   // an edge "at" the clock counts as reached

double finiteOr(double v, double fallback) noexcept {
    return std::isfinite(v) ? v : fallback;
}

} // namespace

DuckMotion::DuckMotion(const DuckConfig& cfg, float width) noexcept
    : mode_(cfg.mode),
      min_(finiteOr(cfg.boundsMin, 0.0)),
      speed_(std::max(0.0, finiteOr(cfg.speed, 0.0))),
      wrapDir_(cfg.startDirection < 0 ? -1 : +1),
      wobbleDeg_(std::isfinite(cfg.wobbleDeg) ? cfg.wobbleDeg : 0.0f),
      wobblePitch_(cfg.wobblePitch > 1.0f ? cfg.wobblePitch : 64.0f) {
    const double span = finiteOr(static_cast<double>(cfg.boundsMax) - cfg.boundsMin, 0.0);
    double phase = finiteOr(cfg.phase, 0.0);
    phase -= std::floor(phase);   // [0, 1)

    if (mode_ == MotionMode::Wrap) {
        travel_ = std::max(0.0, span);
        if (travel_ > 0.0 && speed_ > 0.0) {
            edge_ = travel_ / speed_;
            period_ = edge_;
            half_ = edge_;
            start_ = phase * period_;
        }
        return;
    }

    travel_ = std::max(0.0, span - static_cast<double>(width));
    if (travel_ <= 0.0 || speed_ <= 0.0) {
        return;   // stationary: period_ stays 0
    }
    turn_ = std::clamp(finiteOr(cfg.turnTime, 0.0), 0.0, travel_ / speed_);
    dwell_ = std::max(0.0, finiteOr(cfg.dwell, 0.0));
    half_ = travel_ / speed_ + turn_;
    edge_ = dwell_ + half_;
    period_ = 2.0 * edge_;
    start_ = std::fmod(phase * period_ + (cfg.startDirection < 0 ? edge_ : 0.0), period_);
}

double DuckMotion::halfDistance(double u) const noexcept {
    u = std::clamp(u, 0.0, half_);
    if (turn_ <= 0.0) {
        return speed_ * u;
    }
    if (u < turn_) {
        return speed_ * u * u / (2.0 * turn_);
    }
    if (u <= half_ - turn_) {
        return speed_ * turn_ / 2.0 + speed_ * (u - turn_);
    }
    const double r = half_ - u;
    return travel_ - speed_ * r * r / (2.0 * turn_);
}

double DuckMotion::halfSpeed(double u) const noexcept {
    u = std::clamp(u, 0.0, half_);
    if (turn_ <= 0.0) {
        return speed_;
    }
    if (u < turn_) {
        return speed_ * u / turn_;
    }
    if (u <= half_ - turn_) {
        return speed_;
    }
    return speed_ * (half_ - u) / turn_;
}

MotionSample DuckMotion::sample(double clock) const noexcept {
    MotionSample s;
    if (period_ <= 0.0) {
        s.left = static_cast<float>(min_);
        s.velocity = 0.0f;
        s.direction = mode_ == MotionMode::Wrap ? wrapDir_ : +1;
        return s;
    }
    clock = std::max(0.0, clock);
    s.edges = static_cast<std::int64_t>(std::floor(clock / edge_));

    if (mode_ == MotionMode::Wrap) {
        const double f = std::fmod(clock * speed_, travel_);
        s.left = static_cast<float>(wrapDir_ > 0 ? min_ + f : min_ + travel_ - f);
        s.velocity = static_cast<float>(wrapDir_ * speed_);
        s.direction = wrapDir_;
        return s;
    }

    const double u = std::fmod(clock, period_);
    if (u < dwell_) {                                   // resting at the left stop
        s.left = static_cast<float>(min_);
        s.direction = +1;
    } else if (u < edge_) {                             // travelling right
        s.left = static_cast<float>(min_ + halfDistance(u - dwell_));
        s.velocity = static_cast<float>(halfSpeed(u - dwell_));
        s.direction = +1;
    } else if (u < edge_ + dwell_) {                    // resting at the right stop
        s.left = static_cast<float>(min_ + travel_);
        s.direction = -1;
    } else {                                            // travelling left
        const double v = u - edge_ - dwell_;
        s.left = static_cast<float>(min_ + travel_ - halfDistance(v));
        s.velocity = static_cast<float>(-halfSpeed(v));
        s.direction = -1;
    }
    return s;
}

double DuckMotion::timeToEdge(double clock) const noexcept {
    if (period_ <= 0.0) {
        return kInf;
    }
    clock = std::max(0.0, clock);
    const double k = std::ceil((clock - kEdgeEps) / edge_);
    return std::max(0.0, k * edge_ - clock);
}

float DuckMotion::wobbleAt(float left) const noexcept {
    if (wobbleDeg_ == 0.0f) {
        return 0.0f;
    }
    return wobbleDeg_ * std::sin(2.0f * std::numbers::pi_v<float> * left / wobblePitch_);
}

} // namespace gallery
