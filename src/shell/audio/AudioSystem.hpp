#pragma once

#include "gallery/audio/Soundscape.hpp"
#include "gallery/core/Time.hpp"
#include "gallery/flow/Game.hpp"
#include "gallery/game/DuckLane.hpp"
#include "gallery/game/Shooting.hpp"

#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/Sound.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace shell {

class AssetStore;
class Logger;

struct AudioSettings {
    bool enabled = true;          // --no-audio: never touch the audio device
    int masterPercent = 80;       // --volume N
    bool muted = false;           // --mute
    bool music = true;            // --no-music
    bool logCues = false;         // --log-audio: one log line per cue played
};

// The one owner of everything audible (ARCHITECTURE §2.3, D-019, D-030).
// It asks gallery::Soundscape what to play and plays it; it never changes
// game state. Sounds borrow buffers from the AssetStore, which is declared
// before this object in Application and therefore outlives it.
//
// Per frame, in this order (Application::run):
//   onUiAction()                    for board actions (wooden click)
//   consume(events, transitions)    this frame's game events -> cues, same frame (G-9)
//   update(dt, state, phase, focus) delayed cues, bed fades, volumes
//
// Degrades instead of failing (D-013): no output device, a missing sound, or
// a stream that will not open each log one warning and go silent.
class AudioSystem {
public:
    static constexpr std::size_t kVoices = 16;       // D-019
    static constexpr std::size_t kMaxPending = 16;   // delayed cues in flight

    AudioSystem(const AssetStore& assets, Logger& log, AudioSettings settings);
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    void consume(const std::vector<gallery::GameEvent>& events,
                 const std::vector<gallery::DuckTransition>& transitions);
    void onUiAction();
    void update(gallery::Seconds dt, gallery::AppState state, gallery::RoundPhase phase,
                bool focused);

    // Player controls. Each returns a short description for the on-screen note.
    std::string nudgeVolume(int steps);
    std::string toggleMute();
    std::string toggleMusic();

    [[nodiscard]] bool active() const noexcept { return active_; }
    [[nodiscard]] const gallery::Mixer& mixer() const noexcept { return mixer_; }
    [[nodiscard]] std::uint64_t played(gallery::Cue c) const noexcept {
        return played_[static_cast<std::size_t>(c)];
    }

private:
    void request(const gallery::CueRequest& r);
    void play(gallery::Cue cue);
    [[nodiscard]] std::size_t pickVoice();
    void applyBedVolumes();
    [[nodiscard]] std::string volumeText() const;

    const AssetStore& assets_;
    Logger& log_;
    AudioSettings settings_;
    bool active_ = false;
    gallery::Mixer mixer_;
    gallery::CueVariation variation_;
    gallery::Seconds clock_ = 0.0;   // audio time (wall-clock dt; runs on every screen)

    // One-shot voices, reused round-robin; the oldest is stolen when all are busy.
    std::array<std::optional<sf::Sound>, kVoices> voices_;
    std::array<gallery::Seconds, kVoices> voiceStarted_{};
    std::size_t nextVoice_ = 0;

    struct Pending {
        gallery::Cue cue = gallery::Cue::Shot;
        gallery::Seconds due = 0.0;
        bool live = false;
    };
    std::array<Pending, kMaxPending> pending_{};
    std::vector<gallery::CueRequest> scratch_;   // reserved once; reused every frame

    // Continuous beds.
    std::optional<sf::Sound> track_;     // SFX_CHAIN_LOOP (in memory, looping)
    std::optional<sf::Music> ambience_;  // AMB_MIDWAY (streamed, looping)
    std::optional<sf::Music> music_;     // MUS_CALLIOPE (streamed, looping, optional)
    gallery::Fader trackFader_{0.25};
    gallery::Fader ambienceFader_{1.2};
    gallery::Fader musicFader_{1.5};

    std::array<std::uint64_t, gallery::kCueCount> played_{};
    std::array<bool, gallery::kCueCount> warnedMissing_{};
};

} // namespace shell
