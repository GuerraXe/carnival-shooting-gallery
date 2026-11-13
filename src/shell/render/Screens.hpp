#pragma once

// The title (main menu), pause and results boards, and the round cues
// ("Ready", "Go!", "Time!") — ARCHITECTURE §2.3 ScreenRenderer, D-029.
//
// Every board is a physical object: the UI_PANEL photograph (a painted board
// hung in front of the booth) with runtime sign-writer lettering on it. While
// the photograph is missing, the board is the D-026 labelled grey card. The
// placards' rects come from gallery::ui, so what is drawn is what is
// clickable. Read-only with respect to the game.

#include "gallery/core/Geometry.hpp"
#include "gallery/core/Time.hpp"
#include "gallery/flow/Game.hpp"

#include <string>

#include <SFML/Graphics/RenderTarget.hpp>

namespace shell {
class AssetStore;
}

namespace shell::render {

// An indicator lamp: the DEC_BULB_LIT photograph tinted on/off, or, while it
// is missing, a flat grey disc (dark = off, light = lit). Never a glow.
void drawLamp(sf::RenderTarget& t, gallery::Vec2 centre, float diameter,
              bool lit);

class ScreenRenderer {
public:
    explicit ScreenRenderer(const AssetStore& assets) : assets_(assets) {}

    // Dim wash + board for Title, Paused and Results; nothing otherwise.
    void drawOverlay(sf::RenderTarget& t, const gallery::Game& game, float pixelScale) const;

    // Countdown / Go / Time cue over the lane while playing.
    void drawCue(sf::RenderTarget& t, const gallery::Round& round, float pixelScale) const;

private:
    void drawBoard(sf::RenderTarget& t) const;
    void drawPlacards(sf::RenderTarget& t, const gallery::Game& game, float pixelScale) const;
    void drawHeading(sf::RenderTarget& t, const char* caption, const char* title,
                     float pixelScale) const;
    void drawTitle(sf::RenderTarget& t, float pixelScale) const;
    void drawPaused(sf::RenderTarget& t, const gallery::Game& game, float pixelScale) const;
    void drawResults(sf::RenderTarget& t, const gallery::Game& game, float pixelScale) const;
    void leaderRow(sf::RenderTarget& t, float y, const char* label, const std::string& value,
                   float pixelScale, bool emphasis) const;

    const AssetStore& assets_;
};

} // namespace shell::render
