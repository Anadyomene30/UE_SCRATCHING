// A deliberately tiny test harness. Pulling in a framework would add the first
// external dependency to a library whose whole point is to have none.
#pragma once

#include <cmath>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace svjtest {

struct TestCase {
    std::string name;
    std::function<void()> run;
};

std::vector<TestCase>& registry();

struct Registrar {
    Registrar(std::string name, std::function<void()> run) {
        registry().push_back(TestCase{std::move(name), std::move(run)});
    }
};

// Thrown by a failing check; caught by the runner so one failure does not stop
// the rest of the suite.
struct Failure {
    std::string message;
};

[[noreturn]] void fail(const char* file, int line, const std::string& detail);

int run_all();

}  // namespace svjtest

#define SVJ_CONCAT_INNER(a, b) a##b
#define SVJ_CONCAT(a, b) SVJ_CONCAT_INNER(a, b)

#define SVJ_TEST(name)                                                            \
    static void SVJ_CONCAT(svj_test_fn_, __LINE__)();                             \
    static const ::svjtest::Registrar SVJ_CONCAT(svj_test_reg_, __LINE__)(        \
        name, SVJ_CONCAT(svj_test_fn_, __LINE__));                                \
    static void SVJ_CONCAT(svj_test_fn_, __LINE__)()

#define CHECK(cond)                                                               \
    do {                                                                          \
        if (!(cond)) ::svjtest::fail(__FILE__, __LINE__, "CHECK(" #cond ")");     \
    } while (false)

#define CHECK_EQ(a, b)                                                            \
    do {                                                                          \
        const auto svj_a = (a);                                                   \
        const auto svj_b = (b);                                                   \
        if (!(svj_a == svj_b)) {                                                  \
            std::ostringstream svj_os;                                            \
            svj_os << "CHECK_EQ(" #a ", " #b ") -- left=" << svj_a                \
                   << " right=" << svj_b;                                         \
            ::svjtest::fail(__FILE__, __LINE__, svj_os.str());                    \
        }                                                                         \
    } while (false)

#define CHECK_NEAR(a, b, eps)                                                     \
    do {                                                                          \
        const double svj_a = static_cast<double>(a);                              \
        const double svj_b = static_cast<double>(b);                              \
        if (std::fabs(svj_a - svj_b) > (eps)) {                                   \
            std::ostringstream svj_os;                                            \
            svj_os << "CHECK_NEAR(" #a ", " #b ") -- left=" << svj_a              \
                   << " right=" << svj_b << " tolerance=" << (eps);               \
            ::svjtest::fail(__FILE__, __LINE__, svj_os.str());                    \
        }                                                                         \
    } while (false)
