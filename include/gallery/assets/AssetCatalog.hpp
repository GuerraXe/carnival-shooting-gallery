#pragma once

// Compiled-in asset table (D-012). Pure data: no SFML, so the contract test
// (tests/asset_contract.cpp) can check it against assets/ASSET_MANIFEST.md
// without a window.
//
// Scope: every image, the display font, and every sound (D-030). Wildcard
// manifest rows (prizes_*, dust_scratch_*, light_leak_*) are E1 sets and are
// added when their concrete files exist.

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace gallery {

enum class AssetId : std::uint8_t {
    BgFar,
    BgMid,
    FgBoothFrame,
    FgBackboard,
    FgTrackRail,
    FgWaveStrip,
    FgCounter,
    FgRifle,
    TgtDuckA,
    TgtDuckB,
    TgtDuckC,
    DecSignHeader,
    DecBulbLit,
    DecBulbGlow,
    DecBunting,
    UiScoreBoard,
    UiTimerBoard,
    UiPanel,
    UiReticle,
    OvGrain,
    OvMissMark,
    OvMuzzlePuff,
    FntDisplay,
    OvMuzzleFlash,
    SfxShot,
    SfxDuckHit,
    SfxDuckFlip,
    SfxDuckReset,
    SfxMiss,
    SfxChainLoop,
    SfxBellStart,
    SfxTick,
    SfxEnd,
    SfxUiClick,
    SfxScoreTick,
    SfxResults,
    AmbMidway,
    MusCalliope,
    Count
};

inline constexpr std::size_t kAssetCount = static_cast<std::size_t>(AssetId::Count);

// Sound: decoded into memory at startup (short effects, D-019).
// Stream: opened and decoded while playing (long loops: ambience, music).
enum class AssetKind : std::uint8_t { Image, Font, Sound, Stream };

// Asset Guide §4 priority key. MVP assets are required for a release build;
// E1 assets are optional enhancements (absent = feature not drawn).
enum class AssetPriority : std::uint8_t { Mvp, E1 };

struct AssetEntry {
    AssetId id;
    std::string_view key;    // manifest ID column, e.g. "FG_COUNTER"
    std::string_view path;   // relative to the asset root, '/' separators
    AssetKind kind;
    AssetPriority priority;
    std::string_view spec;   // short human spec, shown on stand-in cards
};

// Order matches AssetId; checked by a unit test.
inline constexpr std::array<AssetEntry, kAssetCount> kAssetCatalog{{
    {AssetId::BgFar, "BG_FAR", "images/backgrounds/bg_far_dusk.jpg", AssetKind::Image, AssetPriority::Mvp, "1960x1100 dusk sky, ride silhouettes, soft focus"},
    {AssetId::BgMid, "BG_MID", "images/backgrounds/bg_mid_midway.png", AssetKind::Image, AssetPriority::Mvp, "1960x1100 tents, string lights; alpha above skyline"},
    {AssetId::FgBoothFrame, "FG_BOOTH_FRAME", "images/foreground/booth_frame.png", AssetKind::Image, AssetPriority::Mvp, "1920x1080 posts, header beam, awning; transparent opening"},
    {AssetId::FgBackboard, "FG_BACKBOARD", "images/foreground/backboard.jpg", AssetKind::Image, AssetPriority::Mvp, "1920x~500 weathered painted planks"},
    {AssetId::FgTrackRail, "FG_TRACK_RAIL", "images/foreground/track_rail.png", AssetKind::Image, AssetPriority::Mvp, "tileable ~512x64 metal rail + chain"},
    {AssetId::FgWaveStrip, "FG_WAVE_STRIP", "images/foreground/wave_strip.png", AssetKind::Image, AssetPriority::Mvp, "1920x~140 blue-painted wooden wave cutout"},
    {AssetId::FgCounter, "FG_COUNTER", "images/foreground/counter.png", AssetKind::Image, AssetPriority::Mvp, "1920x~220 worn wooden counter, shooter's view"},
    {AssetId::FgRifle, "FG_RIFLE", "images/foreground/rifle.png", AssetKind::Image, AssetPriority::Mvp, "~900x500 unbranded mounted rifle; pivot recorded"},
    {AssetId::TgtDuckA, "TGT_DUCK_A", "images/targets/duck_a.png", AssetKind::Image, AssetPriority::Mvp, "~220 px tall straight-on duck; alpha = hit mask"},
    {AssetId::TgtDuckB, "TGT_DUCK_B", "images/targets/duck_b.png", AssetKind::Image, AssetPriority::E1, "paint variant B"},
    {AssetId::TgtDuckC, "TGT_DUCK_C", "images/targets/duck_c.png", AssetKind::Image, AssetPriority::E1, "paint variant C"},
    {AssetId::DecSignHeader, "DEC_SIGN_HEADER", "images/decorations/sign_header.png", AssetKind::Image, AssetPriority::Mvp, "~1400x260 hand-lettered header sign"},
    {AssetId::DecBulbLit, "DEC_BULB_LIT", "images/decorations/bulb_lit.png", AssetKind::Image, AssetPriority::Mvp, "~96x128 lit incandescent bulb in socket"},
    {AssetId::DecBulbGlow, "DEC_BULB_GLOW", "images/decorations/bulb_glow.png", AssetKind::Image, AssetPriority::Mvp, "~256x256 real bulb bloom on black (additive)"},
    {AssetId::DecBunting, "DEC_BUNTING", "images/decorations/bunting.png", AssetKind::Image, AssetPriority::E1, "1920x~160 fabric bunting"},
    {AssetId::UiScoreBoard, "UI_SCORE_BOARD", "images/ui/score_board.png", AssetKind::Image, AssetPriority::Mvp, "~420x180 painted score board, pre-graded"},
    {AssetId::UiTimerBoard, "UI_TIMER_BOARD", "images/ui/timer_board.png", AssetKind::Image, AssetPriority::Mvp, "~300x180 tin plate / small sign, pre-graded"},
    {AssetId::UiPanel, "UI_PANEL", "images/ui/panel_board.png", AssetKind::Image, AssetPriority::Mvp, "~1000x700 board for title/pause/results"},
    {AssetId::UiReticle, "UI_RETICLE", "images/ui/reticle.png", AssetKind::Image, AssetPriority::Mvp, "~64x64 reticle (functional exception, Q-2)"},
    {AssetId::OvGrain, "OV_GRAIN", "images/overlays/film_grain.png", AssetKind::Image, AssetPriority::Mvp, "tileable ~512x512 scanned film grain"},
    {AssetId::OvMissMark, "OV_MISS_MARK", "images/overlays/miss_mark.png", AssetKind::Image, AssetPriority::Mvp, "~48x48 pellet dent in painted wood"},
    {AssetId::OvMuzzlePuff, "OV_MUZZLE_PUFF", "images/overlays/muzzle_puff.png", AssetKind::Image, AssetPriority::Mvp, "~256x256 real smoke puff on black"},
    {AssetId::FntDisplay, "FNT_DISPLAY", "fonts/display.ttf", AssetKind::Font, AssetPriority::Mvp, "OFL showcard / slab-serif display font"},
    {AssetId::OvMuzzleFlash, "OV_MUZZLE_FLASH", "images/overlays/muzzle_flash.png", AssetKind::Image, AssetPriority::E1, "~256x256 real muzzle flash photographed on black (additive)"},
    {AssetId::SfxShot, "SFX_SHOT", "audio/effects/shot.wav", AssetKind::Sound, AssetPriority::Mvp, "air-rifle shot, <= 0.6 s, no leading silence"},
    {AssetId::SfxDuckHit, "SFX_DUCK_HIT", "audio/effects/duck_hit.wav", AssetKind::Sound, AssetPriority::Mvp, "pellet on sheet metal: ping"},
    {AssetId::SfxDuckFlip, "SFX_DUCK_FLIP", "audio/effects/duck_flip.wav", AssetKind::Sound, AssetPriority::Mvp, "hinged metal target slamming flat"},
    {AssetId::SfxDuckReset, "SFX_DUCK_RESET", "audio/effects/duck_reset.wav", AssetKind::Sound, AssetPriority::E1, "hinge rattle: target swinging back up"},
    {AssetId::SfxMiss, "SFX_MISS", "audio/effects/miss_thud.wav", AssetKind::Sound, AssetPriority::Mvp, "pellet into wood"},
    {AssetId::SfxChainLoop, "SFX_CHAIN_LOOP", "audio/effects/chain_loop.wav", AssetKind::Sound, AssetPriority::Mvp, "seamless mechanical track loop, 4-8 s"},
    {AssetId::SfxBellStart, "SFX_BELL_START", "audio/effects/bell_start.wav", AssetKind::Sound, AssetPriority::Mvp, "hand or boxing bell strike"},
    {AssetId::SfxTick, "SFX_TICK", "audio/effects/tick.wav", AssetKind::Sound, AssetPriority::Mvp, "mechanical clock tick"},
    {AssetId::SfxEnd, "SFX_END", "audio/effects/end_bell.wav", AssetKind::Sound, AssetPriority::Mvp, "end bell or buzzer"},
    {AssetId::SfxUiClick, "SFX_UI_CLICK", "audio/effects/ui_click.wav", AssetKind::Sound, AssetPriority::Mvp, "wooden click"},
    {AssetId::SfxScoreTick, "SFX_SCORE_TICK", "audio/effects/score_tick.wav", AssetKind::Sound, AssetPriority::E1, "mechanical counter clack (score board)"},
    {AssetId::SfxResults, "SFX_RESULTS", "audio/effects/results_ding.wav", AssetKind::Sound, AssetPriority::E1, "register bell: score rung up"},
    {AssetId::AmbMidway, "AMB_MIDWAY", "audio/ambience/midway_loop.ogg", AssetKind::Stream, AssetPriority::Mvp, "fairground crowd loop >= 60 s, no intelligible speech"},
    {AssetId::MusCalliope, "MUS_CALLIOPE", "audio/music/calliope_loop.ogg", AssetKind::Stream, AssetPriority::E1, "band organ / calliope; composition AND recording cleared"},
}};

[[nodiscard]] constexpr const AssetEntry& assetEntry(AssetId id) noexcept {
    return kAssetCatalog[static_cast<std::size_t>(id)];
}

[[nodiscard]] constexpr std::optional<AssetId> assetIdFromKey(std::string_view key) noexcept {
    for (const AssetEntry& e : kAssetCatalog) {
        if (e.key == key) {
            return e.id;
        }
    }
    return std::nullopt;
}

// The post-process shader is project source, not an acquired asset (Asset
// Guide §4.4), so it is not a manifest row; it still lives under assets/.
inline constexpr std::string_view kPostProcessShaderPath = "shaders/postprocess.frag";

} // namespace gallery
