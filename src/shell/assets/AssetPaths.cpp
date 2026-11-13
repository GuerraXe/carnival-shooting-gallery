#include "shell/assets/AssetPaths.hpp"

#include "shell/platform/Platform.hpp"
#include "shell/platform/StartupError.hpp"

#include <stdexcept>
#include <string>
#include <system_error>

namespace shell {

namespace fs = std::filesystem;

AssetPaths::AssetPaths(fs::path root) {
    std::error_code ec;
    fs::path absolute = fs::weakly_canonical(root, ec);
    root_ = ec ? std::move(root) : std::move(absolute);

    if (!fs::is_directory(root_, ec)) {
        throw StartupError(
            "The game's asset folder was not found.\n"
            "Expected: " + platform::toUtf8(root_) + "\n"
            "Rebuild the game (the build copies assets/ next to the executable), "
            "or reinstall it.");
    }
}

fs::path AssetPaths::defaultRoot() {
    return platform::exeDirectory() / "assets";
}

fs::path AssetPaths::resolve(std::string_view relative) const {
    const fs::path rel = fs::path(std::u8string(relative.begin(), relative.end()));
    if (rel.empty() || rel.is_absolute() || rel.has_root_name()) {
        throw std::invalid_argument("asset path must be relative: " + std::string(relative));
    }
    for (const auto& part : rel) {
        if (part == "..") {
            throw std::invalid_argument("asset path must not contain '..': " +
                                        std::string(relative));
        }
    }
    return (root_ / rel).make_preferred();
}

} // namespace shell
