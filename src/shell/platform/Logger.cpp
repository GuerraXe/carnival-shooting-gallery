#include "shell/platform/Logger.hpp"

#include <SFML/System/Err.hpp>

#include <chrono>
#include <ctime>
#include <format>
#include <iostream>
#include <string>

namespace shell {

namespace {

std::string timestamp() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const std::time_t t = system_clock::to_time_t(now);
    std::tm local{};
    localtime_s(&local, &t);
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;
    char buf[32]{};
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &local);
    return std::format("{}.{:03}", buf, ms);
}

constexpr std::string_view levelTag(Logger::Level level) {
    switch (level) {
    case Logger::Level::Info: return "INFO ";
    case Logger::Level::Warn: return "WARN ";
    case Logger::Level::Error: return "ERROR";
    }
    return "?????";
}

} // namespace

// Collects characters written to sf::err() and forwards each complete line.
class Logger::SfmlErrBuffer : public std::streambuf {
public:
    explicit SfmlErrBuffer(Logger& owner) : owner_(owner) {}

    ~SfmlErrBuffer() override { flushLine(); }

protected:
    int_type overflow(int_type ch) override {
        if (traits_type::eq_int_type(ch, traits_type::eof())) {
            return traits_type::not_eof(ch);
        }
        const char c = traits_type::to_char_type(ch);
        if (c == '\n') {
            flushLine();
        } else {
            line_.push_back(c);
        }
        return ch;
    }

    int sync() override {
        flushLine();
        return 0;
    }

private:
    void flushLine() {
        if (!line_.empty()) {
            owner_.write(Level::Warn, "[SFML] " + line_);
            line_.clear();
        }
    }

    Logger& owner_;
    std::string line_;
};

Logger::Logger(const std::filesystem::path& logFile) : path_(logFile) {
    file_.open(logFile, std::ios::out | std::ios::trunc);
    if (!file_.is_open()) {
        std::cerr << "[logger] could not open log file; continuing with stderr only\n";
    }
    sfmlBuffer_ = std::make_unique<SfmlErrBuffer>(*this);
    previousSfmlBuffer_ = sf::err().rdbuf(sfmlBuffer_.get());
}

Logger::~Logger() {
    sf::err().rdbuf(previousSfmlBuffer_);   // never leave sf::err pointing at a dead buffer
    sfmlBuffer_.reset();                    // flushes any partial SFML line
}

void Logger::write(Level level, std::string_view message) {
    const std::string line = std::format("{} [{}] {}\n", timestamp(), levelTag(level), message);
    const std::scoped_lock lock(mutex_);
    std::cerr << line;
    if (file_.is_open()) {
        file_ << line;
        file_.flush();   // a crash must not lose the lines leading up to it
    }
}

} // namespace shell
