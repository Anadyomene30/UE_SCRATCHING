#include "harness.h"

#include <iostream>

namespace svjtest {

std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

void fail(const char* file, int line, const std::string& detail) {
    std::ostringstream os;
    os << file << ":" << line << ": " << detail;
    throw Failure{os.str()};
}

int run_all() {
    int failures = 0;
    for (const TestCase& test : registry()) {
        try {
            test.run();
            std::cout << "  ok   " << test.name << "\n";
        } catch (const Failure& f) {
            ++failures;
            std::cout << "  FAIL " << test.name << "\n       " << f.message << "\n";
        } catch (const std::exception& e) {
            ++failures;
            std::cout << "  FAIL " << test.name << "\n       unexpected exception: " << e.what()
                      << "\n";
        }
    }
    std::cout << "\n"
              << registry().size() - static_cast<std::size_t>(failures) << " passed, " << failures
              << " failed, " << registry().size() << " total\n";
    return failures == 0 ? 0 : 1;
}

}  // namespace svjtest

int main() {
    std::cout << "scratchvj core tests\n\n";
    return svjtest::run_all();
}
