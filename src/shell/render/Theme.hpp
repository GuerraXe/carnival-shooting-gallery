#pragma once

// One palette and one type scale for every piece of game lettering: HUD
// boards, cues, and the title / pause / results boards (D-029). Painted-sign
// conventions: few colours, ink on board, red and gold only as accents, no
// gradients, no glow, no digital chrome. Sizes are logical units (1080p).
//
// Colours follow brief §7.3: tungsten amber light, faded carnival red,
// cream and mustard gold, warm brown-black shadows (never pure black).

#include "gallery/core/Geometry.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include <string_view>

namespace shell::render::theme {

// Paint.
inline const sf::Color kInk{34, 24, 18};              // sign-writer's lettering, warm brown-black
inline const sf::Color kCream{238, 226, 198};         // cream enamel
inline const sf::Color kRed{150, 38, 28};             // faded carnival red (accent / warning)
inline const sf::Color kGold{204, 156, 58};           // mustard gold (accent)
inline const sf::Color kShade{14, 9, 5};              // dimming wash behind a board (warm)
inline constexpr float kShadeAlpha = 0.58f;

// Indicator lamps. Without the bulb photograph these are flat grey discs
// (stand-in policy, D-026): dark = off, near-white = lit. No glow is drawn.
inline const sf::Color kLampOff{70, 66, 60};
inline const sf::Color kLampOn{240, 236, 226};
inline const sf::Color kLampPhotoOff{88, 72, 58};      // tint on DEC_BULB_LIT when off

// Type scale (logical px). One display face throughout (FNT_DISPLAY).
inline constexpr float kSizeNumeral = 72.0f;   // score and time numerals
inline constexpr float kSizeTitle = 58.0f;     // board headings
inline constexpr float kSizeCue = 64.0f;       // Ready / Go / Time
inline constexpr float kSizeRow = 30.0f;       // results rows, placard lettering
inline constexpr float kSizeBody = 25.0f;      // board copy
inline constexpr float kSizeCaption = 19.0f;   // letter-spaced captions over headings
inline constexpr float kSizeSmall = 16.0f;     // footnotes, tallies
inline constexpr float kCaptionSpacing = 2.2f; // letter spacing factor for captions

enum class Align { Left, Centre, Right };

// Draw a line of painted lettering. `at` is the top of the glyphs (cap
// height) at the given horizontal anchor. Returns the drawn bounds.
gallery::Rect letter(sf::RenderTarget& t, const sf::Font& font, std::string_view utf8,
                     float size, gallery::Vec2 at, Align align, sf::Color colour,
                     float pixelScale, float letterSpacing = 1.0f, bool bold = false);

// Width a line would take (for dot leaders and fitting).
float letterWidth(const sf::Font& font, std::string_view utf8, float size, float pixelScale,
                  float letterSpacing = 1.0f, bool bold = false);

// A thin painted rule (sign-writer's line), `width` long, centred on x.
void rule(sf::RenderTarget& t, float centreX, float y, float width, sf::Color colour,
          float thickness = 2.0f);

} // namespace shell::render::theme
