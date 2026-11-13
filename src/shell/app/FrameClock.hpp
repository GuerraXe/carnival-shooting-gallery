#pragma once

#include "gallery/core/Time.hpp"

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>

namespace shell {

// Monotonic per-frame delta, clamped to gallery::kMaxFrameDelta (D-007).
class FrameClock {
public:
    // Seconds since the previous call (or construction), clamped.
    [[nodiscard]] gallery::Seconds frameDelta() {
        const auto elapsed = clock_.restart();
        raw_ = static_cast<gallery::Seconds>(elapsed.asMicroseconds()) / 1'000'000.0;
        return gallery::clampFrameDelta(raw_);
    }

    // The unclamped wall time behind the last frameDelta(): what the frame
    // really took (frame-time statistics, AV-7). Never fed to the simulation.
    [[nodiscard]] gallery::Seconds lastRaw() const noexcept { return raw_; }

private:
    sf::Clock clock_;
    gallery::Seconds raw_ = 0.0;
};

} // namespace shell
