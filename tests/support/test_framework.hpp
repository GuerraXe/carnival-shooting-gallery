#pragma once

// Minimal, dependency-free test framework (D-017).
//
//   TEST_CASE("thing does X") {
//       CHECK(1 + 1 == 2);
//       CHECK_EQ(compute(), 42);
//       CHECK_NEAR(ratio(), 0.5, 1e-6);
//   }
//
// TEST_CASE blocks self-register; tests/test_main.cpp runs them all and
// returns non-zero if any failed (CTest treats that as failure).

#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace gallery::test {

struct Failure {
    std::string message;
};

inline std::vector<std::pair<std::string, std::function<void()>>>& registry() {
    static std::vector<std::pair<std::string, std::function<void()>>> r;
    return r;
}

struct Registrar {
    Registrar(std::string name, std::function<void()> fn) {
        registry().emplace_back(std::move(name), std::move(fn));
    }
};

inline int runAll() {
    int failed = 0;
    for (auto& [name, fn] : registry()) {
        try {
            fn();
            std::cout << "[ OK ] " << name << "\n";
        } catch (const Failure& f) {
            std::cout << "[FAIL] " << name << ": " << f.message << "\n";
            ++failed;
        } catch (const std::exception& e) {
            std::cout << "[FAIL] " << name << ": unexpected exception: " << e.what() << "\n";
            ++failed;
        }
    }
    const auto total = registry().size();
    std::cout << (failed == 0 ? "All tests passed" : "SOME TESTS FAILED") << " ("
              << (total - static_cast<std::size_t>(failed)) << "/" << total << ")\n";
    return failed == 0 ? 0 : 1;
}

} // namespace gallery::test

#define GALLERY_TEST_CONCAT_INNER(a, b) a##b
#define GALLERY_TEST_CONCAT(a, b) GALLERY_TEST_CONCAT_INNER(a, b)

#define TEST_CASE(name)                                                                     \
    static void GALLERY_TEST_CONCAT(gallery_test_fn_, __LINE__)();                          \
    static ::gallery::test::Registrar GALLERY_TEST_CONCAT(gallery_test_reg_, __LINE__){     \
        (name), GALLERY_TEST_CONCAT(gallery_test_fn_, __LINE__)};                           \
    static void GALLERY_TEST_CONCAT(gallery_test_fn_, __LINE__)()

#define CHECK(cond)                                                                         \
    do {                                                                                    \
        if (!(cond)) {                                                                      \
            std::ostringstream gallery_oss;                                                 \
            gallery_oss << "CHECK failed: " #cond << " (" << __FILE__ << ":" << __LINE__ << ")"; \
            throw ::gallery::test::Failure{gallery_oss.str()};                              \
        }                                                                                   \
    } while (0)

#define CHECK_EQ(a, b)                                                                      \
    do {                                                                                    \
        auto&& gallery_a = (a);                                                             \
        auto&& gallery_b = (b);                                                             \
        if (!(gallery_a == gallery_b)) {                                                    \
            std::ostringstream gallery_oss;                                                 \
            gallery_oss << "CHECK_EQ failed: " #a " == " #b " (" << gallery_a << " != "     \
                        << gallery_b << ") at " << __FILE__ << ":" << __LINE__;             \
            throw ::gallery::test::Failure{gallery_oss.str()};                              \
        }                                                                                   \
    } while (0)

#define CHECK_NEAR(a, b, tol)                                                               \
    do {                                                                                    \
        const double gallery_a = static_cast<double>(a);                                    \
        const double gallery_b = static_cast<double>(b);                                    \
        if (!(std::fabs(gallery_a - gallery_b) <= static_cast<double>(tol))) {              \
            std::ostringstream gallery_oss;                                                 \
            gallery_oss << "CHECK_NEAR failed: " #a " ~= " #b " (" << gallery_a << " vs "   \
                        << gallery_b << ", tol " << (tol) << ") at " << __FILE__ << ":"     \
                        << __LINE__;                                                        \
            throw ::gallery::test::Failure{gallery_oss.str()};                              \
        }                                                                                   \
    } while (0)
