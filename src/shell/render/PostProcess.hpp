#pragma once

#include "gallery/core/Geometry.hpp"

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <optional>
#include <random>
#include <string>

namespace shell {
class Logger;
}

namespace shell::render {

// World layers render into an offscreen target sized to the viewport's
// PIXELS (D-016) with a logical 1920x1080 view, so photographs are resampled
// once, straight to the final resolution. present() blits it to the window
// through the grade/vignette/grain shader, or as a plain copy if shaders are
// unavailable or switched off (D-013: cosmetic features degrade).
class PostProcess {
public:
    PostProcess(const std::string& shaderSource, const sf::Texture* grain, Logger& log);

    // Recreate the target if `pixelSize` changed. Returns false if no target
    // exists (zero size or allocation failure); the caller skips the frame.
    [[nodiscard]] bool prepare(sf::Vector2u pixelSize);

    [[nodiscard]] sf::RenderTarget& target() noexcept { return target_; }
    [[nodiscard]] sf::Vector2u size() const noexcept { return size_; }

    // Finish the offscreen frame and draw it at `pixelRect` in the window.
    // `light` is a per-channel exposure for the whole world (bulb flicker,
    // shot flash; D-031); {1,1,1} leaves the frame untouched. It needs the
    // shader: the plain-blit fallback shows steady light.
    void present(sf::RenderWindow& window, gallery::Rect pixelRect, double time, bool enabled,
                 sf::Vector3f light = {1.0f, 1.0f, 1.0f});

    [[nodiscard]] bool shaderAvailable() const noexcept { return shader_.has_value(); }
    [[nodiscard]] bool grainAvailable() const noexcept { return grain_ != nullptr; }

private:
    Logger& log_;
    sf::RenderTexture target_;
    sf::Vector2u size_{0, 0};
    std::optional<sf::Shader> shader_;
    const sf::Texture* grain_;
    std::minstd_rand rng_{20261111u};
    sf::Vector2f grainOffset_{0.0f, 0.0f};
    double lastGrainStep_ = -1.0;
};

} // namespace shell::render
