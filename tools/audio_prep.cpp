// audio_prep — reproducible edits of verified audio originals (Asset Guide §5.2
// stage 5–6). Development tool only: never shipped, never run by the game.
//
// Every runtime sound is cut from an untouched original in
// asset-src/originals/audio/ by one command line recorded in
// asset-src/work/audio/PREP.md, so each listed modification can be re-run and
// audited. The only operations are the ones a sound editor would do by hand
// to a real recording: choose a segment, trim leading silence, short fades,
// downmix to mono, peak normalisation, and an equal-power crossfade that
// joins a loop's end to its start. Nothing is synthesised.
//
// Commands (times in seconds):
//   info   <in>
//   play   <in> [--volume 0..100]                         audition on the default device
//   env    <in> [--from S] [--to S] [--step MS]          RMS/peak envelope
//   onsets <in> [--from S] [--to S] [--rise DB] [--floor DB] [--gap MS]
//   quiet  <in> [--below DB] [--min S]                    silent stretches
//   cut    <in> <out> --start S --end S [--trim-lead DB] [--fade-in MS]
//          [--fade-out MS] [--peak DB] [--mono]
//   loop   <in> <out> --start S --length S --xfade S [--peak DB] [--mono]
//
// The output format follows the extension (.wav 16-bit PCM, .ogg Vorbis).

#include <SFML/Audio/InputSoundFile.hpp>
#include <SFML/Audio/OutputSoundFile.hpp>
#include <SFML/Audio/PlaybackDevice.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/SoundChannel.hpp>
#include <SFML/System/Sleep.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;

struct Audio {
    unsigned rate = 0;
    unsigned channels = 0;
    std::vector<float> s;   // interleaved, [-1, 1]
    [[nodiscard]] std::size_t frames() const { return channels ? s.size() / channels : 0; }
    [[nodiscard]] double seconds() const { return rate ? static_cast<double>(frames()) / rate : 0.0; }
};

[[noreturn]] void fail(const std::string& msg) {
    std::fprintf(stderr, "audio_prep: %s\n", msg.c_str());
    std::exit(2);
}

Audio load(const std::filesystem::path& p) {
    sf::InputSoundFile in;
    if (!in.openFromFile(p)) {
        fail("cannot open " + p.string());
    }
    Audio a;
    a.rate = in.getSampleRate();
    a.channels = in.getChannelCount();
    std::vector<std::int16_t> buf(65536);
    std::uint64_t n = 0;
    while ((n = in.read(buf.data(), buf.size())) > 0) {
        for (std::uint64_t i = 0; i < n; ++i) {
            a.s.push_back(static_cast<float>(buf[i]) / 32768.0f);
        }
    }
    return a;
}

void save(const Audio& a, const std::filesystem::path& p) {
    sf::OutputSoundFile out;
    std::vector<sf::SoundChannel> map;
    if (a.channels == 1) {
        map = {sf::SoundChannel::Mono};
    } else if (a.channels == 2) {
        map = {sf::SoundChannel::FrontLeft, sf::SoundChannel::FrontRight};
    } else {
        fail("only mono or stereo output is supported");
    }
    if (!out.openFromFile(p, a.rate, a.channels, map)) {
        fail("cannot write " + p.string());
    }
    std::vector<std::int16_t> pcm(a.s.size());
    for (std::size_t i = 0; i < a.s.size(); ++i) {
        const float v = std::clamp(a.s[i], -1.0f, 1.0f);
        pcm[i] = static_cast<std::int16_t>(std::lround(v * 32767.0f));
    }
    out.write(pcm.data(), pcm.size());
}

double db(double lin) { return lin > 1e-9 ? 20.0 * std::log10(lin) : -180.0; }
double lin(double dbv) { return std::pow(10.0, dbv / 20.0); }

// Mono magnitude of one frame (max over channels).
float frameAbs(const Audio& a, std::size_t f) {
    float m = 0.0f;
    for (unsigned c = 0; c < a.channels; ++c) {
        m = std::max(m, std::fabs(a.s[f * a.channels + c]));
    }
    return m;
}

double windowRms(const Audio& a, std::size_t f0, std::size_t f1) {
    double sum = 0.0;
    std::size_t n = 0;
    for (std::size_t f = f0; f < f1 && f < a.frames(); ++f) {
        for (unsigned c = 0; c < a.channels; ++c) {
            const double v = a.s[f * a.channels + c];
            sum += v * v;
            ++n;
        }
    }
    return n ? std::sqrt(sum / static_cast<double>(n)) : 0.0;
}

float peakOf(const Audio& a) {
    float m = 0.0f;
    for (float v : a.s) {
        m = std::max(m, std::fabs(v));
    }
    return m;
}

Audio slice(const Audio& a, double start, double end) {
    const auto f0 = static_cast<std::size_t>(std::max(0.0, start) * a.rate);
    const auto f1 = std::min(a.frames(), static_cast<std::size_t>(std::max(start, end) * a.rate));
    if (f1 <= f0) {
        fail("empty segment");
    }
    Audio o{a.rate, a.channels, {}};
    o.s.assign(a.s.begin() + static_cast<std::ptrdiff_t>(f0 * a.channels),
               a.s.begin() + static_cast<std::ptrdiff_t>(f1 * a.channels));
    return o;
}

Audio toMono(const Audio& a) {
    if (a.channels == 1) {
        return a;
    }
    Audio o{a.rate, 1, std::vector<float>(a.frames())};
    for (std::size_t f = 0; f < a.frames(); ++f) {
        float sum = 0.0f;
        for (unsigned c = 0; c < a.channels; ++c) {
            sum += a.s[f * a.channels + c];
        }
        o.s[f] = sum / static_cast<float>(a.channels);
    }
    return o;
}

void fade(Audio& a, double inMs, double outMs) {
    const std::size_t n = a.frames();
    const auto fi = std::min(n, static_cast<std::size_t>(inMs / 1000.0 * a.rate));
    const auto fo = std::min(n, static_cast<std::size_t>(outMs / 1000.0 * a.rate));
    for (std::size_t f = 0; f < fi; ++f) {
        const float g = static_cast<float>(std::sin(0.5 * kPi * static_cast<double>(f) / fi));
        for (unsigned c = 0; c < a.channels; ++c) {
            a.s[f * a.channels + c] *= g;
        }
    }
    for (std::size_t k = 0; k < fo; ++k) {
        const std::size_t f = n - 1 - k;
        const float g = static_cast<float>(std::sin(0.5 * kPi * static_cast<double>(k) / fo));
        for (unsigned c = 0; c < a.channels; ++c) {
            a.s[f * a.channels + c] *= g;
        }
    }
}

void normalise(Audio& a, double peakDb) {
    const float p = peakOf(a);
    if (p <= 0.0f) {
        return;
    }
    const auto g = static_cast<float>(lin(peakDb) / p);
    for (float& v : a.s) {
        v *= g;
    }
}

struct Args {
    std::vector<std::string> pos;
    std::map<std::string, std::string> opt;
    [[nodiscard]] bool has(const std::string& k) const { return opt.count(k) > 0; }
    [[nodiscard]] double num(const std::string& k, double def) const {
        const auto it = opt.find(k);
        return it == opt.end() ? def : std::atof(it->second.c_str());
    }
    [[nodiscard]] double need(const std::string& k) const {
        if (!has(k)) {
            fail("missing --" + k);
        }
        return num(k, 0.0);
    }
};

Args parse(int argc, char** argv) {
    Args a;
    for (int i = 2; i < argc; ++i) {
        std::string s = argv[i];
        if (s.rfind("--", 0) == 0) {
            const std::string key = s.substr(2);
            if (key == "mono") {
                a.opt[key] = "1";
            } else if (i + 1 < argc) {
                a.opt[key] = argv[++i];
            } else {
                fail("--" + key + " needs a value");
            }
        } else {
            a.pos.push_back(s);
        }
    }
    return a;
}

void report(const char* what, const Audio& a) {
    std::printf("%s: %.3f s, %u Hz, %u ch, peak %.1f dBFS, rms %.1f dBFS\n", what, a.seconds(),
                a.rate, a.channels, db(peakOf(a)), db(windowRms(a, 0, a.frames())));
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: audio_prep info|env|onsets|quiet|cut|loop <in> [...]\n");
        return 2;
    }
    const std::string cmd = argv[1];
    const Args args = parse(argc, argv);
    if (args.pos.empty()) {
        fail("missing input file");
    }
    const Audio a = load(args.pos[0]);

    if (cmd == "info") {
        report("input", a);
        return 0;
    }

    if (cmd == "play") {
        // Audition through the default output device (listening review), and a
        // device check that involves no game code.
        sf::SoundBuffer buffer;
        if (!buffer.loadFromFile(args.pos[0])) {
            fail("cannot load " + args.pos[0]);
        }
        sf::Sound sound(buffer);
        sound.setVolume(static_cast<float>(args.num("volume", 100.0)));
        sound.play();
        std::printf("playing %s (%.2f s)... status %s\n", args.pos[0].c_str(), a.seconds(),
                    sound.getStatus() == sf::Sound::Status::Playing ? "PLAYING" : "NOT PLAYING");
        const auto device = sf::PlaybackDevice::getDevice();
        if (!device) {
            fail("the output device could not be opened");
        }
        std::printf("device: %s\n", device->c_str());
        // Bounded wait: never hang on a sound that failed to start.
        const int limitMs = static_cast<int>(a.seconds() * 1000.0) + 500;
        for (int waited = 0; sound.getStatus() == sf::Sound::Status::Playing && waited < limitMs;
             waited += 20) {
            sf::sleep(sf::milliseconds(20));
        }
        return 0;
    }

    if (cmd == "env") {
        const double from = args.num("from", 0.0);
        const double to = std::min(args.num("to", a.seconds()), a.seconds());
        const double step = args.num("step", 50.0) / 1000.0;
        for (double t = from; t < to; t += step) {
            const auto f0 = static_cast<std::size_t>(t * a.rate);
            const auto f1 = static_cast<std::size_t>((t + step) * a.rate);
            float pk = 0.0f;
            for (std::size_t f = f0; f < f1 && f < a.frames(); ++f) {
                pk = std::max(pk, frameAbs(a, f));
            }
            std::printf("%8.3f  rms %6.1f  peak %6.1f\n", t, db(windowRms(a, f0, f1)), db(pk));
        }
        return 0;
    }

    if (cmd == "onsets") {
        // Transients: 5 ms energy jumping `rise` dB above the previous 30 ms
        // and above an absolute floor; at least `gap` ms apart.
        const double from = args.num("from", 0.0);
        const double to = std::min(args.num("to", a.seconds()), a.seconds());
        const double rise = args.num("rise", 12.0);
        const double floorDb = args.num("floor", -40.0);
        const double gap = args.num("gap", 150.0) / 1000.0;
        const auto hop = static_cast<std::size_t>(0.005 * a.rate);
        const auto back = static_cast<std::size_t>(0.030 * a.rate);
        double last = -1e9;
        for (auto f = static_cast<std::size_t>(from * a.rate) + back;
             f + hop < a.frames() && f < static_cast<std::size_t>(to * a.rate); f += hop) {
            const double now = db(windowRms(a, f, f + hop));
            const double before = db(windowRms(a, f - back, f));
            const double t = static_cast<double>(f) / a.rate;
            if (now > floorDb && now - before > rise && t - last > gap) {
                // Peak and decay (time to fall 30 dB below that peak).
                float pk = 0.0f;
                const auto pf1 = std::min(a.frames(), f + static_cast<std::size_t>(0.05 * a.rate));
                for (std::size_t k = f; k < pf1; ++k) {
                    pk = std::max(pk, frameAbs(a, k));
                }
                double decay = 0.0;
                for (std::size_t k = f; k + hop < a.frames(); k += hop) {
                    if (db(windowRms(a, k, k + hop)) < db(pk) - 30.0) {
                        decay = static_cast<double>(k - f) / a.rate;
                        break;
                    }
                }
                std::printf("onset %8.3f  peak %6.1f dBFS  rise %+5.1f dB  decay-30dB %.3f s\n", t,
                            db(pk), now - before, decay);
                last = t;
            }
        }
        return 0;
    }

    if (cmd == "quiet") {
        const double below = args.num("below", -45.0);
        const double minLen = args.num("min", 1.0);
        const auto hop = static_cast<std::size_t>(0.05 * a.rate);
        double startQ = -1.0;
        for (std::size_t f = 0; f + hop <= a.frames(); f += hop) {
            const double t = static_cast<double>(f) / a.rate;
            const bool q = db(windowRms(a, f, f + hop)) < below;
            if (q && startQ < 0.0) {
                startQ = t;
            } else if (!q && startQ >= 0.0) {
                if (t - startQ >= minLen) {
                    std::printf("quiet %9.2f .. %9.2f  (%.2f s)\n", startQ, t, t - startQ);
                }
                startQ = -1.0;
            }
        }
        return 0;
    }

    if (args.pos.size() < 2) {
        fail("missing output file");
    }

    if (cmd == "cut") {
        double start = args.need("start");
        const double end = args.need("end");
        if (args.has("trim-lead")) {
            // Move the start to just before the first sample above the threshold
            // (R-9: no leading silence before a transient).
            const double thr = lin(args.num("trim-lead", -40.0));
            for (auto f = static_cast<std::size_t>(start * a.rate);
                 f < static_cast<std::size_t>(end * a.rate) && f < a.frames(); ++f) {
                if (frameAbs(a, f) >= thr) {
                    start = std::max(start, static_cast<double>(f) / a.rate - 0.002);
                    break;
                }
            }
        }
        Audio o = slice(a, start, end);
        if (args.has("mono")) {
            o = toMono(o);
        }
        fade(o, args.num("fade-in", 2.0), args.num("fade-out", 10.0));
        if (args.has("peak")) {
            normalise(o, args.num("peak", -1.0));
        }
        save(o, args.pos[1]);
        std::printf("cut %.3f .. %.3f s -> %s\n", start, end, args.pos[1].c_str());
        report("output", o);
        return 0;
    }

    if (cmd == "loop") {
        // Equal-power crossfade: the first `xfade` seconds of the loop are
        // blended with the `xfade` seconds that follow its end in the
        // original, so the last sample flows into the first with no seam.
        const double start = args.need("start");
        const double length = args.need("length");
        const double xf = args.need("xfade");
        if (xf <= 0.0 || xf * 2.0 > length) {
            fail("xfade must be > 0 and at most half the length");
        }
        Audio body = slice(a, start, start + length);
        const Audio tail = slice(a, start + length, start + length + xf);
        const auto n = std::min(tail.frames(), static_cast<std::size_t>(xf * a.rate));
        for (std::size_t f = 0; f < n; ++f) {
            const double x = static_cast<double>(f) / static_cast<double>(n);
            const auto gin = static_cast<float>(std::sin(0.5 * kPi * x));
            const auto gout = static_cast<float>(std::cos(0.5 * kPi * x));
            for (unsigned c = 0; c < a.channels; ++c) {
                float& v = body.s[f * a.channels + c];
                v = v * gin + tail.s[f * a.channels + c] * gout;
            }
        }
        if (args.has("mono")) {
            body = toMono(body);
        }
        if (args.has("peak")) {
            normalise(body, args.num("peak", -1.0));
        }
        save(body, args.pos[1]);
        std::printf("loop %.3f + %.3f s, xfade %.3f s -> %s\n", start, length, xf,
                    args.pos[1].c_str());
        report("output", body);
        return 0;
    }

    fail("unknown command " + cmd);
}
