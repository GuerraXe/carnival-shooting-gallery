// Asset contract test (ARCHITECTURE §7.1, D-012, D-023).
//
//   asset_contract <project source dir> [--release-gate]
//
// Checks the compiled-in catalog against assets/ASSET_MANIFEST.md and the
// files on disk:
//   - every catalog entry has a manifest row with the same runtime path and priority
//   - status and disk agree: a file exists exactly when the status says it should
//   - PNGs that need transparency really carry an alpha channel
//   - acquired assets have a licence evidence file in assets/licenses/
//   - sound files are in the runtime format (16-bit mono WAV, Ogg streams)
//   - the post-process shader source is present
// With --release-gate (CTest passes it for the Release configuration), any MVP
// entry that is not READY, or any TEMP_ file under assets/, FAILS the test.
// Without it those are reported as warnings only (Debug builds).

#include "gallery/assets/AssetCatalog.hpp"

#include "support/test_framework.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <iterator>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace gallery;

namespace {

fs::path g_source;
bool g_releaseGate = false;

struct ManifestRow {
    std::string path;
    std::string priority;
    std::string status;
};

std::string trim(std::string s) {
    const auto b = s.find_first_not_of(" \t\r");
    const auto e = s.find_last_not_of(" \t\r");
    return b == std::string::npos ? std::string{} : s.substr(b, e - b + 1);
}

std::vector<std::string> cells(const std::string& line) {
    std::vector<std::string> out;
    std::string cur;
    for (std::size_t i = 1; i < line.size(); ++i) {   // skip the leading '|'
        if (line[i] == '|') {
            out.push_back(trim(cur));
            cur.clear();
        } else {
            cur += line[i];
        }
    }
    return out;
}

// Rows of the "Required assets" tables: | ID | Purpose | Runtime path | Prio | Status | ...
std::map<std::string, ManifestRow> readManifest() {
    std::ifstream in(g_source / "assets" / "ASSET_MANIFEST.md");
    if (!in) {
        throw test::Failure{"cannot open assets/ASSET_MANIFEST.md under " + g_source.string()};
    }
    std::map<std::string, ManifestRow> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("| ", 0) != 0) {
            continue;
        }
        const auto c = cells(line);
        if (c.size() < 5 || !assetIdFromKey(c[0])) {
            continue;
        }
        rows[c[0]] = ManifestRow{c[2], c[3], c[4]};
    }
    return rows;
}

std::string firstWord(const std::string& s) {
    const auto end = s.find_first_of(" (");
    return s.substr(0, end);
}

bool statusHasFile(const std::string& status) {
    const std::string w = firstWord(status);
    return w == "IN_PROGRESS" || w == "READY" || w == "PLACEHOLDER";
}

bool knownStatus(const std::string& status) {
    static const std::array<std::string, 8> known{"MISSING", "CANDIDATE", "VERIFIED", "IN_PROGRESS",
                                                  "READY", "PLACEHOLDER", "REFERENCE_ONLY", "REJECTED"};
    const std::string w = firstWord(status);
    for (const auto& k : known) {
        if (w == k) {
            return true;
        }
    }
    return false;
}

// PNG colour type lives at byte 25 (after the 8-byte signature and IHDR header).
// 4 = grey + alpha, 6 = RGBA. Returns -1 if not a PNG.
int pngColourType(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::array<unsigned char, 26> head{};
    if (!f.read(reinterpret_cast<char*>(head.data()), head.size())) {
        return -1;
    }
    const std::array<unsigned char, 8> sig{0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    for (std::size_t i = 0; i < sig.size(); ++i) {
        if (head[i] != sig[i]) {
            return -1;
        }
    }
    return head[25];
}

bool needsAlpha(AssetId id) {
    switch (id) {
    case AssetId::BgMid:
    case AssetId::FgBoothFrame:
    case AssetId::FgTrackRail:
    case AssetId::FgWaveStrip:
    case AssetId::FgCounter:
    case AssetId::FgRifle:
    case AssetId::TgtDuckA:
    case AssetId::TgtDuckB:
    case AssetId::TgtDuckC:
    case AssetId::DecSignHeader:
    case AssetId::DecBulbLit:
    case AssetId::DecBunting:
    case AssetId::UiScoreBoard:
    case AssetId::UiTimerBoard:
    case AssetId::UiPanel:
    case AssetId::UiReticle:
    case AssetId::OvMissMark:
        return true;
    default:
        return false;   // opaque JPEG plates; additive glows/puffs on black; grain; font
    }
}

void warn(const std::string& msg) {
    std::cout << "[WARN] " << msg << "\n";
}

} // namespace

TEST_CASE("contract: every catalog entry has a matching manifest row") {
    const auto rows = readManifest();
    for (const AssetEntry& e : kAssetCatalog) {
        const auto it = rows.find(std::string(e.key));
        if (it == rows.end()) {
            throw test::Failure{"no manifest row for " + std::string(e.key)};
        }
        if (it->second.path != e.path) {
            throw test::Failure{std::string(e.key) + ": manifest path '" + it->second.path +
                                "' != catalog path '" + std::string(e.path) + "'"};
        }
        const bool mvp = it->second.priority.rfind("MVP", 0) == 0;
        if (mvp != (e.priority == AssetPriority::Mvp)) {
            throw test::Failure{std::string(e.key) + ": priority mismatch (manifest '" +
                                it->second.priority + "')"};
        }
        if (!knownStatus(it->second.status)) {
            throw test::Failure{std::string(e.key) + ": unknown status '" + it->second.status + "'"};
        }
    }
}

TEST_CASE("contract: manifest status and files on disk agree") {
    const auto rows = readManifest();
    for (const AssetEntry& e : kAssetCatalog) {
        const ManifestRow& row = rows.at(std::string(e.key));
        const bool exists = fs::is_regular_file(g_source / "assets" / fs::path(e.path));
        const bool expected = statusHasFile(row.status);
        if (exists && !expected) {
            throw test::Failure{std::string(e.key) + ": file exists but manifest status is '" +
                                row.status + "'. Record its source and licence first."};
        }
        if (!exists && expected) {
            throw test::Failure{std::string(e.key) + ": status '" + row.status +
                                "' but file is missing: assets/" + std::string(e.path)};
        }
    }
}

TEST_CASE("contract: transparent cutouts carry an alpha channel") {
    for (const AssetEntry& e : kAssetCatalog) {
        const fs::path p = g_source / "assets" / fs::path(e.path);
        if (!needsAlpha(e.id) || !fs::is_regular_file(p)) {
            continue;
        }
        const int type = pngColourType(p);
        if (type != 4 && type != 6) {
            throw test::Failure{std::string(e.key) + ": " + std::string(e.path) +
                                " must be a PNG with alpha (colour type 4 or 6), got " +
                                std::to_string(type)};
        }
    }
}

TEST_CASE("contract: every acquired asset has a licence evidence file (Asset Guide §3.2)") {
    const auto rows = readManifest();
    for (const AssetEntry& e : kAssetCatalog) {
        const std::string w = firstWord(rows.at(std::string(e.key)).status);
        if (w != "VERIFIED" && w != "IN_PROGRESS" && w != "READY") {
            continue;
        }
        const fs::path evidence = g_source / "assets" / "licenses" / (std::string(e.key) + ".md");
        if (!fs::is_regular_file(evidence)) {
            throw test::Failure{std::string(e.key) + ": status " + w +
                                " but no evidence file assets/licenses/" + std::string(e.key) + ".md"};
        }
        // The evidence names the licence and where the original came from.
        std::ifstream in(evidence);
        const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (text.find("License:") == std::string::npos || text.find("Source URL:") == std::string::npos ||
            text.find("Original SHA-256:") == std::string::npos) {
            throw test::Failure{std::string(e.key) +
                                ": evidence file lacks License:, Source URL: or Original SHA-256:"};
        }
    }
}

namespace {
struct WavInfo {
    bool ok = false;
    unsigned channels = 0;
    unsigned rate = 0;
    unsigned bits = 0;
    double seconds = 0.0;
};

std::uint32_t le32(const unsigned char* p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

// Walks the RIFF chunks for 'fmt ' and 'data'.
WavInfo readWav(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    WavInfo w;
    if (bytes.size() < 12 || bytes.compare(0, 4, "RIFF") != 0 || bytes.compare(8, 4, "WAVE") != 0) {
        return w;
    }
    const auto* b = reinterpret_cast<const unsigned char*>(bytes.data());
    std::size_t pos = 12;
    std::uint32_t dataSize = 0;
    unsigned blockAlign = 0;
    bool fmt = false;
    while (pos + 8 <= bytes.size()) {
        const std::string id = bytes.substr(pos, 4);
        const std::uint32_t size = le32(b + pos + 4);
        if (id == "fmt " && pos + 24 <= bytes.size()) {
            w.channels = b[pos + 10] | (b[pos + 11] << 8);
            w.rate = le32(b + pos + 12);
            blockAlign = b[pos + 20] | (b[pos + 21] << 8);
            w.bits = b[pos + 22] | (b[pos + 23] << 8);
            fmt = true;
        } else if (id == "data") {
            dataSize = size;
        }
        pos += 8 + size + (size & 1u);
    }
    w.ok = fmt && blockAlign > 0 && w.rate > 0;
    w.seconds = w.ok ? static_cast<double>(dataSize / blockAlign) / w.rate : 0.0;
    return w;
}
} // namespace

TEST_CASE("contract: audio files match the runtime spec (Asset Guide §5.3)") {
    for (const AssetEntry& e : kAssetCatalog) {
        const fs::path p = g_source / "assets" / fs::path(e.path);
        if (!fs::is_regular_file(p)) {
            continue;
        }
        if (e.kind == AssetKind::Sound) {
            // SFX: WAV, 16-bit, mono, 44.1 or 48 kHz.
            const WavInfo w = readWav(p);
            if (!w.ok || w.bits != 16 || w.channels != 1 || (w.rate != 44100 && w.rate != 48000)) {
                throw test::Failure{std::string(e.key) + ": " + std::string(e.path) +
                                    " must be 16-bit mono WAV at 44.1/48 kHz (bits " +
                                    std::to_string(w.bits) + ", channels " +
                                    std::to_string(w.channels) + ", rate " + std::to_string(w.rate) + ")"};
            }
            if (e.id == AssetId::SfxShot && w.seconds > 0.6) {
                throw test::Failure{"SFX_SHOT is " + std::to_string(w.seconds) + " s; spec is <= 0.6 s"};
            }
            if (e.id == AssetId::SfxChainLoop && (w.seconds < 4.0 || w.seconds > 8.01)) {
                throw test::Failure{"SFX_CHAIN_LOOP is " + std::to_string(w.seconds) +
                                    " s; spec is a 4-8 s loop"};
            }
        } else if (e.kind == AssetKind::Stream) {
            std::ifstream f(p, std::ios::binary);
            char magic[4]{};
            f.read(magic, 4);
            if (std::string(magic, 4) != "OggS") {
                throw test::Failure{std::string(e.key) + ": " + std::string(e.path) + " is not Ogg"};
            }
        }
    }
}

TEST_CASE("contract: post-process shader source is present") {
    CHECK(fs::is_regular_file(g_source / "assets" / fs::path(kPostProcessShaderPath)));
}

TEST_CASE("contract: release gate (D-023) - MVP assets READY, no TEMP_ files") {
    const auto rows = readManifest();
    std::vector<std::string> problems;
    for (const AssetEntry& e : kAssetCatalog) {
        const std::string status = firstWord(rows.at(std::string(e.key)).status);
        if (e.priority == AssetPriority::Mvp && status != "READY") {
            problems.push_back(std::string(e.key) + " is " + status);
        }
    }
    for (const auto& f : fs::recursive_directory_iterator(g_source / "assets")) {
        if (f.is_regular_file() && f.path().filename().string().rfind("TEMP_", 0) == 0) {
            problems.push_back("placeholder file " + f.path().string());
        }
    }
    if (problems.empty()) {
        return;
    }
    std::ostringstream all;
    all << problems.size() << " MVP asset problem(s): ";
    for (std::size_t i = 0; i < problems.size(); ++i) {
        all << (i ? ", " : "") << problems[i];
    }
    if (g_releaseGate) {
        throw test::Failure{all.str()};
    }
    warn(all.str() + " (Debug: warning only; Release fails)");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: asset_contract <project source dir> [--release-gate]\n";
        return 2;
    }
    g_source = fs::path(argv[1]);
    g_releaseGate = argc > 2 && std::string(argv[2]) == "--release-gate";
    std::cout << "Asset contract against " << g_source.string()
              << (g_releaseGate ? " (release gate ON)" : " (release gate off)") << "\n";
    return test::runAll();
}
