// Carnival Shooting Gallery — process entry point.
//
// Owns the Logger (so a throwing Application constructor can still be logged),
// builds the Application, runs it, and turns every failure into a logged,
// user-visible error with exit code 1 (ARCHITECTURE §6, D-013).

#include "shell/app/Application.hpp"
#include "shell/app/LaunchOptions.hpp"
#include "shell/platform/Logger.hpp"
#include "shell/platform/Platform.hpp"
#include "shell/platform/StartupError.hpp"

#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

namespace {

constexpr std::string_view kDialogTitle = "Carnival Shooting Gallery";

// Debug builds: the CRT reports any heap block still allocated at process
// exit to stderr. A clean run prints nothing.
void enableLeakReport() {
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
#endif
}

int reportFatal(shell::Logger* log, bool dialog, std::string_view headline,
                std::string_view detail) {
    const std::string text = std::string(headline) + "\n\n" + std::string(detail);
    if (log != nullptr) {
        log->error(std::string(headline) + ": " + std::string(detail));
        log->error("Exiting with code 1");
    } else {
        std::cerr << text << '\n';
    }
    if (dialog) {
        shell::platform::showFatalError(kDialogTitle, text);
    }
    return 1;
}

// --no-dialog must hold even when the rest of the command line is invalid:
// an automated run with a typo has to fail with exit code 1, not block on a
// modal message box that nobody will click.
bool noDialogRequested(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr && std::string_view(argv[i]) == "--no-dialog") {
            return true;
        }
    }
    return false;
}

} // namespace

int main(int argc, char* argv[]) {
    enableLeakReport();

    shell::LaunchOptions options;
    try {
        options = shell::parseLaunchOptions(argc, argv);
    } catch (const std::exception& e) {
        return reportFatal(nullptr, !noDialogRequested(argc, argv), "Invalid command line",
                           e.what());
    }

    std::optional<shell::Logger> logger;
    try {
        logger.emplace(shell::platform::exeDirectory() / "carnival.log");
        if (!logger->fileOpen()) {
            // Read-only install folder: keep a log the user can still find.
            const auto fallback = shell::platform::userDataDirectory();
            if (!fallback.empty()) {
                logger.reset();
                logger.emplace(fallback / "carnival.log");
            }
        }

        int exitCode = 0;
        {
            shell::Application app(*logger, options);
            exitCode = app.run();
        }   // Application (window, GL context, fonts) destroyed here, before the log closes
        logger->info("Shutdown complete; exit code " + std::to_string(exitCode));
        return exitCode;
    } catch (const shell::StartupError& e) {
        return reportFatal(logger ? &*logger : nullptr, options.fatalDialog,
                           "The game could not start", e.what());
    } catch (const std::exception& e) {
        return reportFatal(logger ? &*logger : nullptr, options.fatalDialog,
                           "The game stopped because of an unexpected error", e.what());
    } catch (...) {
        return reportFatal(logger ? &*logger : nullptr, options.fatalDialog,
                           "The game stopped because of an unexpected error",
                           "Unknown exception.");
    }
}
