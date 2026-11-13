#include "shell/render/PostProcess.hpp"

#include "gallery/view/Viewport.hpp"
#include "shell/platform/Logger.hpp"

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/View.hpp>

#include <cmath>
#include <format>

namespace shell::render {

namespace {
constexpr float kVignetteStrength = 0.24f;   // ~20 % at the corners (brief §7.5)
constexpr float kGradeStrength = 1.0f;
constexpr float kGrainStrength = 0.055f;     // subtle; scanned plate only
constexpr double kGrainRate = 24.0;          // new grain position 24x per second, like film
const sf::Color kClear{18, 13, 10};          // warm near-black behind every layer
} // namespace

PostProcess::PostProcess(const std::string& shaderSource, const sf::Texture* grain, Logger& log)
    : log_(log), grain_(grain) {
    if (shaderSource.empty()) {
        log_.warn("Post-process: no shader source; using plain blit");
        return;
    }
    if (!sf::Shader::isAvailable()) {
        log_.warn("Post-process: shaders not supported by this GPU/driver; using plain blit");
        return;
    }
    sf::Shader shader;
    if (!shader.loadFromMemory(shaderSource, sf::Shader::Type::Fragment)) {
        log_.warn("Post-process: shader failed to compile (see SFML messages above); plain blit");
        return;
    }
    shader_.emplace(std::move(shader));
    log_.info(std::format("Post-process: grade + vignette active; film grain {}",
                          grain_ ? "from scanned plate (OV_GRAIN)"
                                 : "OFF (OV_GRAIN missing; no procedural substitute, D-022)"));
}

bool PostProcess::prepare(sf::Vector2u pixelSize) {
    if (pixelSize.x == 0 || pixelSize.y == 0) {
        return false;
    }
    if (pixelSize != size_) {
        if (!target_.resize(pixelSize, sf::ContextSettings{.antiAliasingLevel = 4})) {
            log_.error(std::format("Post-process: could not allocate a {}x{} target; keeping {}x{}",
                                   pixelSize.x, pixelSize.y, size_.x, size_.y));
            return size_.x > 0;   // ARCHITECTURE §6: keep the previous target, retry next resize
        }
        target_.setSmooth(true);
        size_ = pixelSize;
        log_.info(std::format("Post-process target {}x{} px", size_.x, size_.y));
    }
    target_.setView(sf::View(sf::FloatRect(
        {0.0f, 0.0f}, {gallery::kLogicalSize.width, gallery::kLogicalSize.height})));
    target_.clear(kClear);
    return true;
}

void PostProcess::present(sf::RenderWindow& window, gallery::Rect pixelRect, double time,
                          bool enabled, sf::Vector3f light) {
    target_.display();

    const sf::Vector2u win = window.getSize();
    window.setView(sf::View(sf::FloatRect(
        {0.0f, 0.0f}, {static_cast<float>(win.x), static_cast<float>(win.y)})));

    sf::Sprite sprite(target_.getTexture());
    // Whole-pixel position: the target is exactly the viewport's pixel size,
    // so this is a 1:1 copy with no resampling.
    sprite.setPosition({std::round(pixelRect.x), std::round(pixelRect.y)});

    if (!enabled || !shader_) {
        window.draw(sprite);
        return;
    }

    if (time - lastGrainStep_ >= 1.0 / kGrainRate || time < lastGrainStep_) {
        std::uniform_real_distribution<float> u(0.0f, 1.0f);
        grainOffset_ = {u(rng_), u(rng_)};
        lastGrainStep_ = time;
    }

    sf::Shader& s = *shader_;
    s.setUniform("texture", sf::Shader::CurrentTexture);
    s.setUniform("vignetteStrength", kVignetteStrength);
    s.setUniform("gradeStrength", kGradeStrength);
    s.setUniform("light", sf::Glsl::Vec3{light});
    if (grain_ != nullptr) {
        s.setUniform("grain", *grain_);
        s.setUniform("grainStrength", kGrainStrength);
        s.setUniform("grainScale",
                     sf::Glsl::Vec2{static_cast<float>(size_.x) / static_cast<float>(grain_->getSize().x),
                                    static_cast<float>(size_.y) / static_cast<float>(grain_->getSize().y)});
        s.setUniform("grainOffset", sf::Glsl::Vec2{grainOffset_});
    } else {
        s.setUniform("grainStrength", 0.0f);
    }
    window.draw(sprite, &s);
}

} // namespace shell::render
