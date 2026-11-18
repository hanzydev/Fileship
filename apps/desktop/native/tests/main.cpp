#include "test.h"

#include <shared/utf8.h>

#include <cstdio>
#include <cstring>
#include <exception>

namespace fileship::tests {

std::vector<Test>& registry() {
    static std::vector<Test> tests;
    return tests;
}

Registration::Registration(char const* name, std::function<void()> run) {
    registry().push_back({name, std::move(run)});
}

void check(bool condition, char const* expression, char const* file, int line) {
    if (condition) return;
    throw Failure{std::string(file) + ":" + std::to_string(line) + ": " + expression};
}

void checkEqual(std::wstring const& actual, std::wstring const& expected, char const* expression, char const* file,
                int line) {
    if (actual == expected) return;

    throw Failure{std::string(file) + ":" + std::to_string(line) + ": " + expression + "\n      actual:   " +
                  toUtf8(actual) + "\n      expected: " + toUtf8(expected)};
}

}

int main(int count, char** arguments) {
    using namespace fileship::tests;

    char const* filter = count > 1 ? arguments[1] : "";
    int failed = 0;
    int ran = 0;

    for (auto const& test : registry()) {
        if (!strstr(test.name, filter)) continue;
        ran++;

        try {
            test.run();
        } catch (Failure const& failure) {
            failed++;
            printf("FAIL %s\n    %s\n", test.name, failure.message.c_str());
        } catch (std::exception const& error) {
            failed++;
            printf("FAIL %s\n    threw %s\n", test.name, error.what());
        } catch (...) {
            failed++;
            printf("FAIL %s\n    threw\n", test.name);
        }
    }

    printf("%d tests, %d failed\n", ran, failed);
    return failed ? 1 : 0;
}
