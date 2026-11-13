#pragma once

// The game's art (D-044): flat grey shapes, drawn in code.
//
// These began as layout stand-ins for photographs (D-026). The owner chose to
// keep them as the final look, so they are now the source of every sprite:
// tools/sprite_bake.cpp draws each one through this painter and saves it as a
// PNG under assets/images/, and the game loads those files like any sprite.
// The renderer also calls the painter directly if a file is missing (Debug
// or --stand-ins), so the two always match.
//
// Style rules:
//   - flat greys only; darker = farther, ducks lightest (Asset Guide §2.3).
//     The one soft edge is the bulb glow, which is light, not paint.
//   - each shape fills its slot's real size and rough silhouette.
//   - lettering only on the header sign, in the display font.
// Every function draws into a destination rectangle (or centre) in the
// target's coordinates, so the same code paints the screen and a sprite file.

#include "gallery/core/Geometry.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Transform.hpp>

#include <functional>

namespace shell::render {

// Greys by depth: far = dark, near = mid. Ducks are the lightest value so they
// have the highest contrast in the composition (Asset Guide §2.3).
namespace standin {
inline const sf::Color kSky{38, 38, 38};
inline const sf::Color kMidground{58, 58, 58};
inline const sf::Color kBackboard{92, 92, 92};
inline const sf::Color kRail{70, 70, 70};
inline const sf::Color kDuck{214, 214, 214};
inline const sf::Color kWave{120, 120, 120};
inline const sf::Color kFrame{78, 78, 78};
inline const sf::Color kAwning{104, 104, 104};
inline const sf::Color kBulb{170, 170, 170};
inline const sf::Color kCounterTop{112, 112, 112};
inline const sf::Color kCounterFront{84, 84, 84};
inline const sf::Color kRifle{50, 50, 50};
inline const sf::Color kRifleWood{64, 64, 64};   // stock and forend, a shade off the metal
inline const sf::Color kBoard{128, 128, 128};
inline const sf::Color kMark{48, 48, 48};
inline const sf::Color kPuff{150, 150, 150};
inline const sf::Color kGlow{40, 40, 40};         // added on top of the scene (additive)
inline const sf::Color kPennantA{128, 128, 128};
inline const sf::Color kPennantB{108, 108, 108};
inline const sf::Color kLabel{232, 232, 232};
} // namespace standin

class StandInPainter {
public:
    // Scenery.
    void sky(sf::RenderTarget& t, gallery::Rect r) const;          // BG_FAR, slot kFarPlate
    void midground(sf::RenderTarget& t, gallery::Rect r) const;    // BG_MID, slot kMidPlate
    void backboard(sf::RenderTarget& t, gallery::Rect r) const;    // FG_BACKBOARD
    void trackRail(sf::RenderTarget& t, gallery::Rect r) const;    // FG_TRACK_RAIL (tiles)
    void boothFrame(sf::RenderTarget& t) const;                    // FG_BOOTH_FRAME, whole canvas
    void waveStrip(sf::RenderTarget& t, gallery::Rect r) const;    // FG_WAVE_STRIP
    void counter(sf::RenderTarget& t, gallery::Rect r) const;      // FG_COUNTER

    // Decorations.
    void bulb(sf::RenderTarget& t, gallery::Rect r) const;         // DEC_BULB_LIT, kBulbSize
    void bulbGlow(sf::RenderTarget& t, gallery::Rect r) const;     // DEC_BULB_GLOW (additive)
    void sign(sf::RenderTarget& t, gallery::Rect r, const sf::Font& font,
              float pixelScale) const;                             // DEC_SIGN_HEADER
    void bunting(sf::RenderTarget& t, gallery::Rect r) const;      // DEC_BUNTING

    // Targets.
    void duck(sf::RenderTarget& t, gallery::Rect r, sf::Color fill,
              const sf::RenderStates& states = sf::RenderStates::Default) const;

    // First-person rifle seen from behind (D-039), drawn in barrel coordinates:
    // toLogical(along, across) with along 0 = swivel, 1 = muzzle, across in
    // image units either side of the barrel line.
    using RifleMap = std::function<sf::Vector2f(float along, float across)>;
    void rifle(sf::RenderTarget& t, const RifleMap& toLogical) const;

    // Interface boards (UI_SCORE_BOARD, UI_TIMER_BOARD, UI_PANEL).
    void board(sf::RenderTarget& t, gallery::Rect r) const;
    // Aiming reticle (UI_RETICLE): `size` across, `kick` 0..1 opens the ticks.
    void reticle(sf::RenderTarget& t, gallery::Vec2 centre, float size, float kick,
                 sf::Color light) const;

    // Effects.
    void missMark(sf::RenderTarget& t, gallery::Vec2 centre, float radius, float alpha) const;
    void muzzlePuff(sf::RenderTarget& t, gallery::Rect r, float alpha) const;
};

} // namespace shell::render
