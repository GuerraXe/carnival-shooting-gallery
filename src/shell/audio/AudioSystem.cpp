#include "shell/audio/AudioSystem.hpp"

#include "shell/assets/AssetStore.hpp"
#include "shell/platform/Logger.hpp"
#include "shell/platform/Platform.hpp"

#include <SFML/Audio/PlaybackDevice.hpp>
#include <SFML/Audio/SoundSource.hpp>

#include <algorithm>
#include <exception>
#include <format>

namespace shell {

using gallery::AssetId;
using gallery::Bus;
using gallery::Cue;

namespace {

// Bed levels inside their bus, on top of Soundscape's bedLevels(): the
// target mechanism is a background texture under the shots.
constexpr float kTrackGain = 0.30f;
constexpr float kAmbienceGain = 1.0f;
constexpr float kMusicGain = 1.0f;

// SFML volumes are 0..100.
float sfVolume(float linear) {
    return std::clamp(linear, 0.0f, 1.0f) * 100.0f;
}

// Every sound is heard as part of the booth, centred: no 3D panning or
// distance attenuation.
void flat(sf::SoundSource& s) {
    s.setSpatializationEnabled(false);
    s.setRelativeToListener(true);
}

bool playing(const sf::SoundSource& s) {
    return s.getStatus() == sf::SoundSource::Status::Playing;
}

} // namespace

AudioSystem::AudioSystem(const AssetStore& assets, Logger& log, AudioSettings settings)
    : assets_(assets), log_(log), settings_(settings) {
    mixer_.setMasterPercent(settings_.masterPercent);
    mixer_.setMuted(settings_.muted);
    mixer_.setMusicOn(settings_.music);
    scratch_.reserve(64);

    if (!settings_.enabled) {
        log_.info("Audio: disabled (--no-audio)");
        return;
    }
    try {
        const auto devices = sf::PlaybackDevice::getAvailableDevices();
        if (devices.empty()) {
            log_.warn("Audio: no playback device; the game continues silently");
            return;
        }
        const auto device = sf::PlaybackDevice::getDefaultDevice();
        log_.info(std::format("Audio: {} playback device(s), default '{}'", devices.size(),
                              device ? *device : std::string("?")));

        if (const sf::SoundBuffer* b = assets_.sound(AssetId::SfxChainLoop)) {
            track_.emplace(*b);
            flat(*track_);
            track_->setLooping(true);
            track_->setVolume(0.0f);
        }
        auto openStream = [&](std::optional<sf::Music>& m, AssetId id) {
            const auto path = assets_.streamPath(id);
            if (!path) {
                return;   // AssetStore already logged it as missing
            }
            m.emplace();
            if (!m->openFromFile(*path)) {
                log_.warn(std::format("Audio: could not open {}; silent",
                                      gallery::assetEntry(id).key));
                m.reset();
                return;
            }
            flat(*m);
            m->setLooping(true);
            m->setVolume(0.0f);
        };
        openStream(ambience_, AssetId::AmbMidway);
        openStream(music_, AssetId::MusCalliope);

        // SFML opens the output device when the first sound object exists and
        // keeps it while any does. Make sure one exists now (the track, or a
        // voice that stays allocated for the process), then ask whether the
        // device really opened: a refused device (driver, exclusive mode,
        // sandbox) would otherwise fail on every single cue.
        if (!track_ && !ambience_) {
            for (const gallery::CueSpec& s : gallery::kCueTable) {
                if (const sf::SoundBuffer* b = assets_.sound(s.asset)) {
                    voices_[0].emplace(*b);
                    flat(*voices_[0]);
                    break;
                }
            }
        }
        const auto opened = sf::PlaybackDevice::getDevice();
        if (!opened) {
            log_.warn("Audio: the output device could not be opened (see the [SFML] lines above); "
                      "the game continues silently");
            for (auto& v : voices_) {
                v.reset();
            }
            track_.reset();
            ambience_.reset();
            music_.reset();
            return;
        }
        log_.info("Audio: output device open: " + *opened);
        active_ = true;
    } catch (const std::exception& e) {
        log_.warn(std::string("Audio: initialisation failed (") + e.what() + "); silent");
        track_.reset();
        ambience_.reset();
        music_.reset();
        return;
    }

    std::size_t silent = 0;
    for (const gallery::CueSpec& s : gallery::kCueTable) {
        silent += assets_.sound(s.asset) ? 0 : 1;
    }
    log_.info(std::format("Audio: {} voices; {} of {} cues have a recording; track loop {}, "
                          "ambience {}, music {}; {}",
                          kVoices, gallery::kCueCount - silent, gallery::kCueCount,
                          track_ ? "ready" : "MISSING", ambience_ ? "ready" : "MISSING",
                          music_ ? (mixer_.musicOn() ? "ready" : "ready but OFF (--no-music)")
                                 : "not installed (optional)",
                          volumeText()));
}

AudioSystem::~AudioSystem() {
    // Explicit, ordered teardown: stop every source before anything it reads
    // goes away. Voices and the track borrow AssetStore buffers (which outlive
    // this object); the streams own their files and decoder threads.
    for (auto& v : voices_) {
        if (v) {
            v->stop();
        }
        v.reset();
    }
    if (track_) {
        track_->stop();
        track_.reset();
    }
    if (ambience_) {
        ambience_->stop();
        ambience_.reset();
    }
    if (music_) {
        music_->stop();
        music_.reset();
    }
    if (active_) {
        std::string counts;
        for (const gallery::CueSpec& s : gallery::kCueTable) {
            counts += std::format("{}{}={}", counts.empty() ? "" : ", ", s.name,
                                  played_[static_cast<std::size_t>(s.cue)]);
        }
        log_.info("Audio: stopped and released. Cues played: " + counts);
    }
}

std::string AudioSystem::volumeText() const {
    if (mixer_.muted()) {
        return "sound muted";
    }
    return std::format("volume {}%", mixer_.masterPercent());
}

std::string AudioSystem::nudgeVolume(int steps) {
    mixer_.nudge(steps);
    applyBedVolumes();
    const std::string s = volumeText();
    log_.info("Audio: " + s);
    return mixer_.muted() ? s + " (M to unmute)" : s;
}

std::string AudioSystem::toggleMute() {
    mixer_.toggleMute();
    applyBedVolumes();
    const std::string s = mixer_.muted() ? "sound muted" : "sound on, " + volumeText();
    log_.info("Audio: " + s);
    return s;
}

std::string AudioSystem::toggleMusic() {
    mixer_.toggleMusic();
    applyBedVolumes();
    const std::string s = !music_ ? std::string("no music installed")
                          : mixer_.musicOn() ? std::string("music on")
                                             : std::string("music off");
    log_.info("Audio: " + s);
    return s;
}

void AudioSystem::onUiAction() {
    request({Cue::UiClick});
}

void AudioSystem::consume(const std::vector<gallery::GameEvent>& events,
                          const std::vector<gallery::DuckTransition>& transitions) {
    scratch_.clear();
    for (const gallery::GameEvent& e : events) {
        gallery::cuesForEvent(e, scratch_);
    }
    for (const gallery::DuckTransition& t : transitions) {
        gallery::cuesForTransition(t, scratch_);
    }
    for (const gallery::CueRequest& r : scratch_) {
        request(r);
    }
}

void AudioSystem::request(const gallery::CueRequest& r) {
    if (r.delay <= 0.0) {
        play(r.cue);
        return;
    }
    for (Pending& p : pending_) {
        if (!p.live) {
            p = Pending{r.cue, clock_ + r.delay, true};
            return;
        }
    }
    play(r.cue);   // queue full (cannot happen at a 250 ms fire rate): play now
}

std::size_t AudioSystem::pickVoice() {
    for (std::size_t n = 0; n < kVoices; ++n) {
        const std::size_t i = (nextVoice_ + n) % kVoices;
        if (!voices_[i] || !playing(*voices_[i])) {
            nextVoice_ = (i + 1) % kVoices;
            return i;
        }
    }
    // All busy: steal the one that started longest ago.
    std::size_t oldest = 0;
    for (std::size_t i = 1; i < kVoices; ++i) {
        if (voiceStarted_[i] < voiceStarted_[oldest]) {
            oldest = i;
        }
    }
    return oldest;
}

void AudioSystem::play(Cue cue) {
    const gallery::CueSpec& spec = gallery::cueSpec(cue);
    const auto idx = static_cast<std::size_t>(cue);
    if (!active_) {
        return;
    }
    const sf::SoundBuffer* buffer = assets_.sound(spec.asset);
    if (!buffer) {
        if (!warnedMissing_[idx]) {
            warnedMissing_[idx] = true;
            log_.warn(std::format("Audio: cue '{}' is silent ({} missing)", spec.name,
                                  gallery::assetEntry(spec.asset).key));
        }
        return;
    }
    const gallery::PlayParams p = variation_.next(spec);
    const float volume = p.gain * mixer_.gain(spec.bus);
    const std::size_t v = pickVoice();
    if (voices_[v]) {
        voices_[v]->stop();
        voices_[v]->setBuffer(*buffer);
    } else {
        voices_[v].emplace(*buffer);
        flat(*voices_[v]);
    }
    voices_[v]->setPitch(p.pitch);
    voices_[v]->setVolume(sfVolume(volume));
    voices_[v]->play();
    voiceStarted_[v] = clock_;
    ++played_[idx];
    if (settings_.logCues) {
        log_.info(std::format("Audio cue: {} (voice {}, pitch {:.3f}, volume {:.2f}, audio t={:.3f} s)",
                              spec.name, v, p.pitch, volume, clock_));
    }
}

void AudioSystem::update(gallery::Seconds dt, gallery::AppState state, gallery::RoundPhase phase,
                         bool focused) {
    if (!active_) {
        return;
    }
    clock_ += dt;
    for (Pending& p : pending_) {
        if (p.live && clock_ >= p.due) {
            p.live = false;
            play(p.cue);
        }
    }

    const gallery::BedLevels target = gallery::bedLevels(state, phase, focused);
    trackFader_.step(target.track, dt);
    ambienceFader_.step(target.ambience, dt);
    musicFader_.step(mixer_.musicOn() ? target.music : 0.0f, dt);
    applyBedVolumes();
}

void AudioSystem::applyBedVolumes() {
    if (!active_) {
        return;
    }
    // A bed plays while its level is above zero and pauses (keeping its
    // place) at zero, so nothing decodes or mixes when it cannot be heard.
    auto drive = [](sf::SoundSource& s, float level) {
        s.setVolume(sfVolume(level));
        if (level > 0.0f && !playing(s)) {
            s.play();
        } else if (level <= 0.0f && playing(s)) {
            s.pause();
        }
    };
    if (track_) {
        drive(*track_, trackFader_.value() * kTrackGain * mixer_.gain(Bus::Effects));
    }
    if (ambience_) {
        drive(*ambience_, ambienceFader_.value() * kAmbienceGain * mixer_.gain(Bus::Ambience));
    }
    if (music_) {
        drive(*music_, musicFader_.value() * kMusicGain * mixer_.gain(Bus::Music));
    }
}

} // namespace shell
