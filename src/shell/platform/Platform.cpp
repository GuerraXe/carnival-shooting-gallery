#include "shell/platform/Platform.hpp"

#include <windows.h>

#include <string>
#include <system_error>

namespace shell::platform {

std::filesystem::path exeDirectory() {
    std::wstring buffer(MAX_PATH, L'\0');
    for (;;) {
        const DWORD written =
            GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (written == 0) {
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                                    "GetModuleFileNameW failed");
        }
        if (written < buffer.size()) {   // not truncated
            buffer.resize(written);
            return std::filesystem::path(buffer).parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
}

std::filesystem::path userDataDirectory() {
    wchar_t buffer[MAX_PATH]{};
    const DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        return {};
    }
    std::filesystem::path dir = std::filesystem::path(buffer) / L"CarnivalShootingGallery";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return (!ec && std::filesystem::is_directory(dir, ec)) ? dir : std::filesystem::path{};
}

std::filesystem::path windowsFontPath(std::string_view fileName) {
    wchar_t dir[MAX_PATH]{};
    const UINT n = GetWindowsDirectoryW(dir, MAX_PATH);
    std::filesystem::path base = (n > 0 && n < MAX_PATH) ? std::filesystem::path(dir)
                                                         : std::filesystem::path(L"C:\\Windows");
    return base / L"Fonts" / std::filesystem::path(std::string(fileName));
}

namespace {

std::wstring widen(std::string_view utf8) {
    if (utf8.empty()) {
        return {};
    }
    const int len = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
                                        nullptr, 0);
    std::wstring out(static_cast<std::size_t>(len > 0 ? len : 0), L'\0');
    if (len > 0) {
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), out.data(),
                            len);
    }
    return out;
}

} // namespace

void showFatalError(std::string_view title, std::string_view message) noexcept {
    try {
        const std::wstring wTitle = widen(title);
        const std::wstring wMessage = widen(message);
        MessageBoxW(nullptr, wMessage.c_str(), wTitle.c_str(),
                    MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND);
    } catch (...) {
        // Out of memory while reporting a fatal error: nothing more we can do.
    }
}

unsigned windowDpi(sf::WindowHandle handle) noexcept {
    return handle != nullptr ? GetDpiForWindow(handle) : 0u;
}

unsigned systemDpi() noexcept {
    const UINT dpi = GetDpiForSystem();
    return dpi > 0 ? dpi : 96u;
}

bool isMinimized(sf::WindowHandle handle) noexcept {
    return handle != nullptr && IsIconic(handle) != FALSE;
}

std::string toUtf8(const std::filesystem::path& path) {
    const std::u8string u8 = path.u8string();
    return std::string(u8.begin(), u8.end());
}

} // namespace shell::platform
