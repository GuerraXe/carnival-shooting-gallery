#include "shell/render/Screens.hpp"

#include "shell/assets/AssetStore.hpp"
#include "shell/render/DrawUtil.hpp"
#include "shell/render/StandIn.hpp"
#include "shell/render/Theme.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <string>

namespace shell::render {

using gallery::AppState;
using gallery::ButtonId;
using gallery::Rect;
using gallery::Vec2;
namespace ui = gallery::ui;
using theme::Align;

namespace {

constexpr Rect kPanel = ui::kPanel;
constexpr float kCx = kPanel.x + kPanel.width / 2.0f;
constexpr float kCaptionY = kPanel.y + 46.0f;
constexpr float kTitleY = kPanel.y + 80.0f;
constexpr float kRuleY = kPanel.y + 160.0f;
constexpr float kBodyY = kPanel.y + 194.0f;          // first line under the heading rule
constexpr float kRowLeft = kPanel.x + 180.0f;        // results table
constexpr float kRowRight = kPanel.x + kPanel.width - 180.0f;

std::string clockText(int seconds) {
    seconds = std::max(0, seconds);
    return std::format("{}:{:02}", seconds / 60, seconds % 60);
}

// Keyboard equivalent painted small at a placard's right end.
const char* keyHint(AppState s, ButtonId id) {
    switch (id) {
    case ButtonId::StartRound: return "Enter";
    case ButtonId::Quit: return "Esc";
    case ButtonId::Resume: return "Space";
    case ButtonId::RestartRound: return "R";
    case ButtonId::MainMenu: return s == AppState::Paused ? "Esc" : "";
    case ButtonId::PlayAgain: return "Enter";
    }
    return "";
}

float smoothstep(float e0, float e1, float x) {
    const float t = std::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

} // namespace

void drawLamp(sf::RenderTarget& t, const AssetStore& assets, Vec2 centre, float diameter, bool lit) {
    if (const sf::Texture* tex = assets.texture(gallery::AssetId::DecBulbLit)) {
        const Rect r{centre.x - diameter / 2.0f, centre.y - diameter * 0.65f, diameter,
                     diameter * 1.3f};
        drawPlaced(t, *tex, gallery::placeImage(sizeOf(*tex), r, gallery::Fit::Contain),
                   lit ? sf::Color::White : theme::kLampPhotoOff);
        return;
    }
    // Stand-in: grey disc in a dark socket ring. Lit = light, off = dark.
    const float r = diameter / 2.0f;
    sf::CircleShape disc(r, 28);
    disc.setOrigin({r, r});
    disc.setPosition(toSf(centre));
    disc.setFillColor(lit ? theme::kLampOn : theme::kLampOff);
    disc.setOutlineThickness(2.0f);
    disc.setOutlineColor(theme::kInk);
    t.draw(disc);
}

// ---------------------------------------------------------------------------
// Overlays
// ---------------------------------------------------------------------------

void ScreenRenderer::drawOverlay(sf::RenderTarget& t, const gallery::Game& game,
                                 float pixelScale) const {
    const AppState s = game.state();
    if (s != AppState::Title && s != AppState::Paused && s != AppState::Results) {
        return;
    }
    // The booth stays visible, dimmed, behind the board (brief §6.2).
    fillRect(t, gallery::layout::kCanvas, withAlpha(theme::kShade, theme::kShadeAlpha));
    drawBoard(t);
    switch (s) {
    case AppState::Title: drawTitle(t, pixelScale); break;
    case AppState::Paused: drawPaused(t, game, pixelScale); break;
    case AppState::Results: drawResults(t, game, pixelScale); break;
    default: break;
    }
    drawPlacards(t, game, pixelScale);
}

void ScreenRenderer::drawBoard(sf::RenderTarget& t) const {
    if (const sf::Texture* tex = assets_.texture(gallery::AssetId::UiPanel)) {
        drawPlaced(t, *tex, gallery::placeImage(sizeOf(*tex), kPanel, gallery::Fit::Contain));
        return;
    }
    // D-026 stand-in: flat grey card, named, so it is never taken for art.
    fillRect(t, kPanel, standin::kBoard);
}

void ScreenRenderer::drawHeading(sf::RenderTarget& t, const char* caption, const char* title,
                                 float pixelScale) const {
    const sf::Font& f = assets_.font();
    theme::letter(t, f, caption, theme::kSizeCaption, {kCx, kCaptionY}, Align::Centre,
                  theme::kInk, pixelScale, theme::kCaptionSpacing);
    theme::letter(t, f, title, theme::kSizeTitle, {kCx, kTitleY}, Align::Centre, theme::kInk,
                  pixelScale);
    // Sign-writer's double rule: ink line with a short red stripe under it.
    theme::rule(t, kCx, kRuleY, 420.0f, theme::kInk, 2.0f);
    theme::rule(t, kCx, kRuleY + 7.0f, 120.0f, theme::kRed, 3.0f);
}

void ScreenRenderer::drawTitle(sf::RenderTarget& t, float pixelScale) const {
    const sf::Font& f = assets_.font();
    drawHeading(t, "STEP RIGHT UP", "Carnival Shooting Gallery", pixelScale);
    theme::letter(t, f, "Sixty seconds on the clock.", theme::kSizeBody, {kCx, kBodyY},
                  Align::Centre, theme::kInk, pixelScale);
    theme::letter(t, f, "Ten points for every duck you knock down.", theme::kSizeBody,
                  {kCx, kBodyY + 40.0f}, Align::Centre, theme::kInk, pixelScale);
    theme::letter(t, f, "Red decoys cost twenty.  Gold ducks give free ammo.", theme::kSizeBody,
                  {kCx, kBodyY + 80.0f}, Align::Centre, theme::kRed, pixelScale);
    theme::letter(t, f, "Six rounds a load  —  R or right-click to reload", theme::kSizeSmall,
                  {kCx, kBodyY + 140.0f}, Align::Centre, theme::kInk, pixelScale, 1.15f);
    theme::letter(t, f, "Mouse to aim  —  Click to shoot  —  Esc to pause", theme::kSizeSmall,
                  {kCx, kBodyY + 172.0f}, Align::Centre, theme::kInk, pixelScale, 1.15f);
}

void ScreenRenderer::drawPaused(sf::RenderTarget& t, const gallery::Game& game,
                                float pixelScale) const {
    const sf::Font& f = assets_.font();
    const gallery::Round& r = game.round();
    drawHeading(t, "INTERMISSION", "Paused", pixelScale);
    const std::string line =
        r.clock().phase() == gallery::RoundPhase::Countdown
            ? std::string("The round has not started yet")
            : std::format("Time left  {}      Score  {:03}", clockText(r.clock().displaySeconds()),
                          r.stats().score());
    theme::letter(t, f, line, theme::kSizeBody, {kCx, kBodyY}, Align::Centre, theme::kInk,
                  pixelScale);
    theme::letter(t, f, "Click anywhere on the booth, or press Space, to carry on",
                  theme::kSizeSmall, {kCx, kBodyY + 52.0f}, Align::Centre, theme::kInk,
                  pixelScale, 1.1f);
}

void ScreenRenderer::leaderRow(sf::RenderTarget& t, float y, const char* label,
                               const std::string& value, float pixelScale, bool emphasis) const {
    const sf::Font& f = assets_.font();
    const float size = emphasis ? theme::kSizeRow + 6.0f : theme::kSizeRow;
    const Rect l = theme::letter(t, f, label, size, {kRowLeft, y}, Align::Left, theme::kInk,
                                 pixelScale, 1.0f, emphasis);
    const Rect v = theme::letter(t, f, value, size, {kRowRight, y}, Align::Right, theme::kInk,
                                 pixelScale, 1.0f, emphasis);
    // Dot leaders between label and value, sitting on the row's baseline.
    // The pitch is measured over many dots (a single ". " has its trailing
    // space trimmed from the glyph box).
    constexpr int kProbe = 20;
    const std::string probe = [] {
        std::string s;
        for (int i = 0; i < kProbe; ++i) {
            s += ". ";
        }
        return s + ".";
    }();
    const float pitch =
        std::max(1.0f, theme::letterWidth(f, probe, theme::kSizeSmall, pixelScale) / kProbe);
    const float from = l.x + l.width + 12.0f;
    const float to = v.x - 12.0f;
    if (to > from + pitch) {
        const auto n = static_cast<std::size_t>((to - from) / pitch);
        std::string dots;
        dots.reserve(n * 2);
        for (std::size_t i = 0; i < n; ++i) {
            dots += i == 0 ? "." : " .";
        }
        // `y` is the cap top; the baseline is about 0.70 of the size below
        // it in this face. The dot glyph box starts just above the baseline.
        const float baseline = y + size * 0.70f;
        theme::letter(t, f, dots, theme::kSizeSmall, {to, baseline - theme::kSizeSmall * 0.16f},
                      Align::Right, withAlpha(theme::kInk, 0.6f), pixelScale);
    }
}

void ScreenRenderer::drawResults(sf::RenderTarget& t, const gallery::Game& game,
                                 float pixelScale) const {
    const sf::Font& f = assets_.font();
    const gallery::RoundStats& s = game.round().stats();
    drawHeading(t, "ROUND OVER", "Results", pixelScale);
    const auto acc = s.accuracyPercent();
    float y = kBodyY;
    constexpr float kStep = 42.0f;   // six rows fit above the placards
    leaderRow(t, y, "Score", std::format("{}", s.score()), pixelScale, true);
    y += kStep + 8.0f;
    leaderRow(t, y, "Shots fired", std::format("{}", s.shots()), pixelScale, false);
    y += kStep;
    leaderRow(t, y, "Hits", std::format("{}", s.hits()), pixelScale, false);
    y += kStep;
    leaderRow(t, y, "Misses", std::format("{}", s.misses()), pixelScale, false);
    y += kStep;
    leaderRow(t, y, "Decoys hit", std::format("{}", s.decoys()), pixelScale, false);
    y += kStep;
    // "—" when nothing was fired (G-23).
    leaderRow(t, y, "Accuracy", acc ? std::format("{}%", *acc) : std::string("—"), pixelScale,
              false);
    y += kStep + 14.0f;

    if (game.newBest()) {
        // A painted red ribbon with cream lettering.
        const char* text = "New session best!";
        const float w = theme::letterWidth(f, text, theme::kSizeBody, pixelScale, 1.05f) + 48.0f;
        fillRect(t, {kCx - w / 2.0f, y - 8.0f, w, 42.0f}, theme::kRed);
        theme::letter(t, f, text, theme::kSizeBody, {kCx, y + 2.0f}, Align::Centre, theme::kCream,
                      pixelScale, 1.05f);
    } else if (const auto best = game.best().best()) {
        theme::letter(t, f, std::format("Session best  {}", *best), theme::kSizeBody, {kCx, y},
                      Align::Centre, theme::kInk, pixelScale);
    }
}

void ScreenRenderer::drawPlacards(sf::RenderTarget& t, const gallery::Game& game,
                                  float pixelScale) const {
    const sf::Font& f = assets_.font();
    const std::optional<ButtonId> hover = game.hovered();
    for (const gallery::Button& b : ui::buttons(game.state())) {
        const Rect r = b.rect;
        const bool on = hover == b.id;
        // Painted placard: an ink-lined panel; the one under the pointer is
        // painted solid ink with cream lettering and its lamp lit.
        if (on) {
            fillRect(t, r, theme::kInk);
        } else {
            outline(t, r, theme::kInk, 2.0f);
        }
        const sf::Color ink = on ? theme::kCream : theme::kInk;
        drawLamp(t, assets_, {r.x + 28.0f, r.y + r.height / 2.0f}, 16.0f, on);
        theme::letter(t, f, b.label, theme::kSizeRow, {r.x + r.width / 2.0f + 6.0f,
                                                       r.y + (r.height - theme::kSizeRow * 0.74f) / 2.0f},
                      Align::Centre, ink, pixelScale);
        if (const char* k = keyHint(game.state(), b.id); *k != '\0') {
            theme::letter(t, f, k, theme::kSizeSmall - 2.0f,
                          {r.x + r.width - 14.0f, r.y + (r.height - theme::kSizeSmall * 0.7f) / 2.0f},
                          Align::Right, withAlpha(ink, 0.7f), pixelScale, 1.2f);
        }
    }
}

// ---------------------------------------------------------------------------
// Round cues
// ---------------------------------------------------------------------------

void ScreenRenderer::drawCue(sf::RenderTarget& t, const gallery::Round& round,
                             float pixelScale) const {
    const gallery::RoundClock& c = round.clock();
    const char* word = nullptr;
    float alpha = 1.0f;
    switch (c.phase()) {
    case gallery::RoundPhase::Countdown: word = "Ready…"; break;
    case gallery::RoundPhase::Live:
        if (c.phaseTime() < 0.8) {
            word = "Go!";
            alpha = 1.0f - smoothstep(0.45f, 0.8f, static_cast<float>(c.phaseTime()));
        }
        break;
    case gallery::RoundPhase::Ending:
    case gallery::RoundPhase::Finished: word = "Time!"; break;
    }
    if (!word || alpha <= 0.0f) {
        return;
    }
    // Showcard lettering over the backboard, above the lane: cream with an
    // ink edge so it reads on any plank colour.
    sf::Text text = crispText(assets_.font(), word, theme::kSizeCue, pixelScale);
    text.setLetterSpacing(1.1f);
    const sf::FloatRect b = text.getLocalBounds();
    text.setOrigin({b.position.x + b.size.x / 2.0f, b.position.y});
    text.setPosition({kCx, 330.0f});
    text.setFillColor(withAlpha(theme::kCream, alpha));
    text.setOutlineColor(withAlpha(theme::kInk, alpha));
    text.setOutlineThickness(3.0f * pixelScale);
    t.draw(text);
}

} // namespace shell::render
