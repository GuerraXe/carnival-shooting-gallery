#pragma once

// The thin, Windows-specific edge of the shell. Everything that includes
// <windows.h> lives behind these functions (D-001: only platform/ is non-portable).

#include <SFML/Window/WindowHandle.hpp>

#include <filesystem>
#include <string>
#include <string_view>

namespace shell::platform {

// Directory containing the running executable. Assets and the log file are
// located relative to this, never the current working directory, so the game
// works however it is launched (Explorer, terminal from any folder, IDE).
[[nodiscard]] std::filesystem::path exeDirectory();

// Fallback folder for the log when the exe directory is read-only (e.g. the
// game was copied under Program Files): %LOCALAPPDATA%\CarnivalShootingGallery,
// created on demand. Empty if LOCALAPPDATA is unset or cannot be created.
[[nodiscard]] std::filesystem::path userDataDirectory();

// Path to a font shipped with Windows, e.g. "segoeui.ttf". Used only for the
// temporary M0 title text until the project's own OFL font is sourced.
[[nodiscard]] std::filesystem::path windowsFontPath(std::string_view fileName);

// Modal error dialog for fatal startup failures. `message` is UTF-8.
void showFatalError(std::string_view title, std::string_view message) noexcept;

// Effective DPI of the window's monitor (96 = 100 % scaling). 0 if unknown.
[[nodiscard]] unsigned windowDpi(sf::WindowHandle handle) noexcept;

// System DPI before any window exists (96 = 100 % scaling).
[[nodiscard]] unsigned systemDpi() noexcept;

// True while the window is minimised (Windows does not send a 0x0 resize).
[[nodiscard]] bool isMinimized(sf::WindowHandle handle) noexcept;

// UTF-8 text of a path, safe for logging regardless of the ANSI code page.
[[nodiscard]] std::string toUtf8(const std::filesystem::path& path);

} // namespace shell::platform
