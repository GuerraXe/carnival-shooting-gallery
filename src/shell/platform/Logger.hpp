#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <streambuf>
#include <string_view>

namespace shell {

// Timestamped log lines to stderr and to a log file (ARCHITECTURE §6).
//
// - If the file cannot be opened the game continues with stderr only.
// - While a Logger is alive, sf::err() is redirected into it so SFML's own
//   warnings (failed decode, GL context issues) land in the same log. The
//   original stream buffer is restored in the destructor.
// - Thread-safe for whole lines (SFML's audio backend may report from its
//   own thread).
class Logger {
public:
    enum class Level { Info, Warn, Error };

    explicit Logger(const std::filesystem::path& logFile);
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    void info(std::string_view message) { write(Level::Info, message); }
    void warn(std::string_view message) { write(Level::Warn, message); }
    void error(std::string_view message) { write(Level::Error, message); }
    void write(Level level, std::string_view message);

    [[nodiscard]] bool fileOpen() const noexcept { return file_.is_open(); }
    [[nodiscard]] const std::filesystem::path& filePath() const noexcept { return path_; }

private:
    class SfmlErrBuffer;   // line-buffering streambuf that forwards to write()

    std::mutex mutex_;
    std::filesystem::path path_;
    std::ofstream file_;
    std::unique_ptr<SfmlErrBuffer> sfmlBuffer_;
    std::streambuf* previousSfmlBuffer_ = nullptr;
};

} // namespace shell
