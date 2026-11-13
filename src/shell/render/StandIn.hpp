#pragma once

// Layout stand-ins for MISSING photographic assets (Asset Guide §5.4).
//
// Policy, enforced here:
//   - flat, neutral grey only: no texture, no gradient, no colour, no
//     outline styling. A stand-in can never be mistaken for final art.
//   - the slot's real size and rough silhouette, so composition, scale and
//     occlusion can be judged.
//   - no lettering on the game screen (D-035, owner request): the F3
//     inspection overlay (Debug) and carnival.log list what is missing.
// Nothing here is ever a substitute for a photograph; the instant the real
// file is in assets/, the renderer draws it instead.

#include "gallery/assets/AssetCatalog.hpp"
#include "gallery/core/Geometry.hpp"
#include "gallery/view/SceneLayout.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Transform.hpp>

#include <functional>

namespace shell::render {

// Neutral greys by depth: far = dark, near = mid. Ducks are the lightest
// value so the "ducks have the highest contrast" rule can be judged even in
// the stand-in composition (Asset Guide §2.3).
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
inline const sf::Color kLabel{232, 232, 232};
} // namespace standin

class StandInPainter {
public:
    // Silhouettes.
    void boothFrame(sf::RenderTarget& t) const;
    void waveStrip(sf::RenderTarget& t, gallery::Rect r) const;
    void counter(sf::RenderTarget& t, gallery::Rect r) const;
    void duck(sf::RenderTarget& t, gallery::Rect r, sf::Color fill,
              const sf::RenderStates& states = sf::RenderStates::Default) const;
    // Rifle silhouette in the 900x500 spec frame, drawn through `toLogical`.
    // First-person rifle seen from behind (D-039), drawn in barrel coordinates:
    // toLogical(along, across) with along 0 = swivel, 1 = muzzle, across in
    // image units either side of the barrel line.
    using RifleMap = std::function<sf::Vector2f(float along, float across)>;
    void rifle(sf::RenderTarget& t, const RifleMap& toLogical) const;
};

} // namespace shell::render
