#pragma once

#include <filesystem>
#include <string_view>

namespace shell {

// Resolves asset paths against ONE root directory, which by default is the
// `assets/` folder next to the executable (CMake mirrors it there after every
// build). The working directory is never consulted, so launching from
// Explorer, an IDE, or a terminal in any folder behaves identically.
//
// M0 only validates and resolves paths. M1's AssetStore will load from it.
class AssetPaths {
public:
    // Throws StartupError (naming the expected path) if `root` is not a directory.
    explicit AssetPaths(std::filesystem::path root);

    // <exe directory>/assets
    [[nodiscard]] static std::filesystem::path defaultRoot();

    [[nodiscard]] const std::filesystem::path& root() const noexcept { return root_; }

    // `relative` uses '/' separators, e.g. "images/targets/duck_red.png".
    // Absolute paths and ".." components are rejected (std::invalid_argument):
    // the compiled-in manifest must never escape the asset root.
    [[nodiscard]] std::filesystem::path resolve(std::string_view relative) const;

private:
    std::filesystem::path root_;
};

} // namespace shell
