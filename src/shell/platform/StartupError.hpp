#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace shell {

// Thrown only during boot (D-013). Carries EVERY problem found, one per line,
// so the player sees the full list in one dialog instead of fix-one-rerun.
class StartupError : public std::runtime_error {
public:
    explicit StartupError(std::string_view problem) : std::runtime_error(std::string(problem)) {}
    explicit StartupError(const std::vector<std::string>& problems)
        : std::runtime_error(join(problems)) {}

private:
    static std::string join(const std::vector<std::string>& problems) {
        std::string out;
        for (const auto& p : problems) {
            if (!out.empty()) {
                out += '\n';
            }
            out += p;
        }
        return out;
    }
};

} // namespace shell
