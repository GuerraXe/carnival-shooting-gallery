#include "gallery/audio/Soundscape.hpp"

#include <algorithm>
#include <cmath>
#include <variant>

namespace gallery {

const char* busName(Bus b) noexcept {
    switch (b) {
    case Bus::Effects: return "effects";
    case Bus::Interface: return "interface";
    case Bus::Ambience: return "ambience";
    case Bus::Music: return "music";
    case Bus::Count: break;
    }
    return "?";
}

void cuesForEvent(const GameEvent& e, std::vector<CueRequest>& out) {
    if (std::holds_alternative<event::Go>(e)) {
        out.push_back({Cue::BellStart});
    } else if (std::holds_alternative<event::ShotFired>(e)) {
        out.push_back({Cue::Shot});
    } else if (std::holds_alternative<event::DuckHit>(e)) {
        out.push_back({Cue::DuckHit});
        out.push_back({Cue::ScoreTick, kScoreTickDelay});
    } else if (std::holds_alternative<event::ShotMissed>(e)) {
        out.push_back({Cue::Miss});
    } else if (std::holds_alternative<event::LowTimeTick>(e)) {
        out.push_back({Cue::Tick});
    } else if (std::holds_alternative<event::TimeUp>(e)) {
        out.push_back({Cue::EndBell});
        out.push_back({Cue::EndBell, kEndBellRepeat});
    } else if (std::holds_alternative<event::RoundFinished>(e)) {
        out.push_back({Cue::Results});
    } else if (std::holds_alternative<event::DryFire>(e)) {
        out.push_back({Cue::DryFire});
    } else if (std::holds_alternative<event::ReloadStarted>(e)) {
        out.push_back({Cue::Reload});
    } else if (std::holds_alternative<event::Reloaded>(e)) {
        out.push_back({Cue::Reloaded});
    } else if (std::holds_alternative<event::PowerUp>(e)) {
        out.push_back({Cue::PowerUp});
    }
    // RoundReset: silent.
}

void cuesForTransition(const DuckTransition& t, std::vector<CueRequest>& out) {
    if (t.from == DuckState::Falling && t.to == DuckState::Down) {
        out.push_back({Cue::DuckFlip});
    } else if (t.from == DuckState::Down && t.to == DuckState::Rising) {
        out.push_back({Cue::DuckReset});
    }
}

bool isUiAction(AppState before, AppState after, const InputEvent& e) noexcept {
    if (before == after || after == AppState::Exiting) {
        return false;   // nothing happened, or the game is closing
    }
    return std::holds_alternative<input::PrimaryPressed>(e) ||
           std::holds_alternative<input::KeyPressed>(e);
}

BedLevels bedLevels(AppState state, RoundPhase phase, bool focused) noexcept {
    BedLevels b;
    switch (state) {
    case AppState::Title:
    case AppState::Results:
        b.ambience = 1.0f;
        b.music = 1.0f;
        break;
    case AppState::Playing:
        b.ambience = 0.85f;   // a touch lower while aiming
        b.music = 0.6f;
        b.track = (phase == RoundPhase::Countdown || phase == RoundPhase::Live) ? 1.0f : 0.0f;
        break;
    case AppState::Paused:
        b.ambience = 0.5f;
        b.music = 0.4f;
        break;
    case AppState::Exiting:
        return BedLevels{};
    }
    if (!focused) {
        // Another window has the user's attention: the booth fades back.
        b.ambience = std::min(b.ambience, 0.35f);
        b.music = std::min(b.music, 0.3f);
        b.track = 0.0f;
    }
    return b;
}

float Fader::step(float target, Seconds dt) noexcept {
    if (!std::isfinite(dt) || dt <= 0.0 || !std::isfinite(target)) {
        return value_;
    }
    target = std::clamp(target, 0.0f, 1.0f);
    const auto maxStep = rampTime_ > 0.0 ? static_cast<float>(dt / rampTime_) : 1.0f;
    if (value_ < target) {
        value_ = std::min(target, value_ + maxStep);
    } else {
        value_ = std::max(target, value_ - maxStep);
    }
    return value_;
}

void Fader::snap(float v) noexcept {
    value_ = std::isfinite(v) ? std::clamp(v, 0.0f, 1.0f) : 0.0f;
}

void Mixer::setMasterPercent(int percent) noexcept {
    const int clamped = std::clamp(percent, 0, 100);
    masterStep_ = (clamped * kSteps + 50) / 100;
}

bool Mixer::nudge(int steps) noexcept {
    const int before = masterStep_;
    masterStep_ = std::clamp(masterStep_ + steps, 0, kSteps);
    return masterStep_ != before;
}

float Mixer::gain(Bus bus) const noexcept {
    if (muted_ || (bus == Bus::Music && !musicOn_)) {
        return 0.0f;
    }
    const float x = static_cast<float>(masterStep_) / static_cast<float>(kSteps);
    return x * x * trim_[static_cast<std::size_t>(bus)];
}

float CueVariation::unit() noexcept {
    // xorshift32: tiny, fast, deterministic.
    state_ ^= state_ << 13;
    state_ ^= state_ >> 17;
    state_ ^= state_ << 5;
    return static_cast<float>(state_ & 0xFFFFFFu) / static_cast<float>(0xFFFFFFu) * 2.0f - 1.0f;
}

PlayParams CueVariation::next(const CueSpec& spec) noexcept {
    // Always draw both numbers so one cue's settings never shift the
    // sequence another cue sees.
    const float p = unit();
    const float g = unit();
    PlayParams out;
    out.pitch = 1.0f + spec.pitchJitter * p;
    out.gain = std::clamp(spec.gain * (1.0f + spec.gainJitter * g), 0.0f, 1.0f);
    return out;
}

} // namespace gallery
