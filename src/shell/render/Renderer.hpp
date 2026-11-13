#pragma once

#include "gallery/core/Geometry.hpp"
#include "gallery/core/Time.hpp"
#include "gallery/view/SceneLayout.hpp"
#include "gallery/view/Viewport.hpp"
#include "gallery/game/DuckLane.hpp"
#include "gallery/game/Shooting.hpp"
#include "gallery/flow/Game.hpp"
#include "shell/render/DuckVisuals.hpp"
#include "shell/render/PostProcess.hpp"
#include "shell/render/Screens.hpp"
#include "shell/render/StandIn.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace shell {
class AssetStore;
class Logger;
} // namespace shell

namespace shell::render {

// Visual-only transient effects (ARCHITECTURE §2.3 EffectsLayer). Spawned from
// the logic's shot events (gallery::GameEvent); they never affect gameplay
// and hold no rules of their own (no cooldown, no hit testing).
class EffectsState {
public:
    static constexpr std::size_t kMaxMissMarks = 20;      // brief G-10
    static constexpr gallery::Seconds kMissMarkLife = 2.0;
    static constexpr gallery::Seconds kMarkPopTime = 0.10; // impact: mark settles in
    static constexpr gallery::Seconds kPuffLife = 0.45;
    static constexpr gallery::Seconds kRecoilTime = 0.16;
    static constexpr gallery::Seconds kKickTime = 0.12;    // reticle kick (G-9)
    static constexpr gallery::Seconds kHitFlashTime = 0.20;
    static constexpr std::size_t kMaxTags = 8;
    static constexpr gallery::Seconds kTagLife = 0.85;

    void onShot(gallery::Vec2 muzzle);
    void onMiss(gallery::Vec2 at);
    void onHit(gallery::Vec2 at, int points, gallery::TargetKind kind);
    void update(gallery::Seconds dt);
    void clear();

    struct Mark {
        gallery::Vec2 pos;
        gallery::Seconds age = kMissMarkLife;   // >= life means inactive
    };
    struct Tag {
        gallery::Vec2 pos;
        int points = 0;
        gallery::TargetKind kind = gallery::TargetKind::Duck;
        gallery::Seconds age = kTagLife;
    };
    [[nodiscard]] const std::array<Mark, kMaxMissMarks>& marks() const noexcept { return marks_; }
    [[nodiscard]] const std::array<Tag, kMaxTags>& tags() const noexcept { return tags_; }
    [[nodiscard]] float recoil() const noexcept;               // 1 at the shot -> 0
    [[nodiscard]] float kick() const noexcept;                 // reticle kick 1 -> 0
    [[nodiscard]] float hitFlash() const noexcept;             // reticle hit flash 1 -> 0
    [[nodiscard]] gallery::Vec2 puffPos() const noexcept { return puffPos_; }
    [[nodiscard]] gallery::Seconds puffAge() const noexcept { return puffAge_; }
    [[nodiscard]] gallery::Seconds sinceShot() const noexcept { return sinceShot_; }

private:
    std::array<Mark, kMaxMissMarks> marks_{};
    std::size_t nextMark_ = 0;
    std::array<Tag, kMaxTags> tags_{};
    std::size_t nextTag_ = 0;
    gallery::Vec2 puffPos_{};
    gallery::Seconds puffAge_ = kPuffLife;
    gallery::Seconds sinceShot_ = 10.0;
    gallery::Seconds sinceHit_ = 10.0;
};

// Per-frame diagnostics for the inspection overlay.
struct FrameInfo {
    gallery::Seconds dt = 0.0;
    bool focused = true;
    bool showHint = false;               // debug-key hint on the title (Debug builds)
    std::string_view lanePreset;         // --targets
    std::string_view reaction;           // --reaction
};

// Draws the booth composition: the pass list in gallery::kDrawPasses, world
// passes through PostProcess, interface passes after it at full clarity
// (D-014). Read-only with respect to everything outside itself.
class Renderer {
public:
    Renderer(const AssetStore& assets, const DuckVisuals& ducks, Logger& log);

    void setAim(gallery::Vec2 logicalAim) noexcept { aim_ = logicalAim; }
    // Spawn presentation for this frame's logic events (call before draw).
    void consume(const std::vector<gallery::GameEvent>& events);
    // `effectsRun` is false while the game is paused or on a menu: shot
    // effects freeze with the round; lamps and grain keep their own time.
    void update(gallery::Seconds dt, bool effectsRun);

    void toggleGroup(gallery::LayerGroup group);
    void togglePostProcess();
    // Lighting flicker (bulb dips, generator sags; D-031). --no-flicker turns
    // it off for players who find any flicker uncomfortable: steady light.
    void setFlicker(bool on) noexcept { flicker_ = on; }
    // A short line of painted lettering (volume, mute) that fades after ~1.6 s.
    void showNote(std::string text);
    // F3 cycles Off -> Full (layout + targets) -> Lane (targets only) -> Off.
    enum class InspectMode { Off, Full, Lane };
    void toggleInspect() noexcept {
        inspect_ = inspect_ == InspectMode::Off    ? InspectMode::Full
                   : inspect_ == InspectMode::Full ? InspectMode::Lane
                                                   : InspectMode::Off;
    }
    void setInspect(InspectMode mode) noexcept { inspect_ = mode; }

    // `game` is read only (the renderer never mutates gameplay).
    void draw(sf::RenderWindow& window, const gallery::Viewport& viewport,
              const gallery::Game& game, const FrameInfo& info);

    [[nodiscard]] gallery::Vec2 aim() const noexcept { return aim_; }

    // The static scenery drawn in front of the targets (wave cutout, booth
    // frame, header sign, counter), rendered once through the very passes
    // draw() uses and reduced to an alpha mask over the logical canvas. A
    // shot on an opaque part of it hits the scenery (G-16). Call once at
    // startup; throws StartupError if the offscreen target cannot be made.
    [[nodiscard]] gallery::Occluder buildForegroundOccluder();

private:
    void drawPass(sf::RenderTarget& t, gallery::DrawPass pass);
    void drawTrackRail(sf::RenderTarget& t);
    void drawDucks(sf::RenderTarget& t);
    void drawTargetOverlay(sf::RenderTarget& t);
    void drawBulbs(sf::RenderTarget& t);
    void drawRifle(sf::RenderTarget& t);
    void drawMissMarks(sf::RenderTarget& t);
    void drawMuzzlePuff(sf::RenderTarget& t);
    void drawBoard(sf::RenderTarget& t, gallery::AssetId id, gallery::Rect slot,
                   const char* caption, const std::string& value, sf::Color valueInk);
    void drawTimerBoard(sf::RenderTarget& t);
    void drawAmmo(sf::RenderTarget& t);   // magazine on the counter (D-035)
    void drawCrosshair(sf::RenderTarget& t);
    void drawInspectOverlay(sf::RenderTarget& t, const gallery::Viewport& viewport,
                            const FrameInfo& info);
    void drawHint(sf::RenderTarget& t);
    void drawHitTags(sf::RenderTarget& t);
    void drawShotTally(sf::RenderTarget& t);
    void drawNote(sf::RenderTarget& t);
    void drawBunting(sf::RenderTarget& t);
    [[nodiscard]] float lightLevel() const noexcept;   // ambientLight, or 1 with flicker off
    [[nodiscard]] std::string shotSummary() const;

    [[nodiscard]] bool groupOn(gallery::LayerGroup g) const noexcept {
        return groupsOn_[static_cast<std::size_t>(g)];
    }
    [[nodiscard]] gallery::RiflePose rifleNow() const noexcept;
    [[nodiscard]] gallery::Vec2 muzzleNow() const noexcept;

    const AssetStore& assets_;
    const DuckVisuals& ducks_;
    Logger& log_;
    PostProcess post_;
    StandInPainter standIn_;
    ScreenRenderer screens_;

    std::array<bool, gallery::kLayerGroupCount> groupsOn_{};
    bool postEnabled_ = true;
    bool flicker_ = true;
    std::string note_;
    gallery::Seconds noteAge_ = 10.0;
    InspectMode inspect_ = InspectMode::Off;
    float pixelScale_ = 1.0f;

    gallery::Vec2 aim_{gallery::kLogicalSize.width / 2.0f, gallery::kLogicalSize.height / 2.0f};
    gallery::Seconds time_ = 0.0;         // wall-clock-ish render time (effects, bulbs, grain)
    const gallery::DuckLane* lane_ = nullptr;   // set by draw(); Application keeps the lane alive longer
    const gallery::Round* round_ = nullptr;     // set by draw()
    const gallery::Game* game_ = nullptr;       // set by draw()
    EffectsState effects_;
    gallery::RifleMount mount_;     // stiff swivel (D-039)
    bool mountStarted_ = false;
};

} // namespace shell::render
