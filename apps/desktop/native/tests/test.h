#pragma once

#include <functional>
#include <string>
#include <vector>

namespace fileship::tests {

struct Test {
    char const* name;
    std::function<void()> run;
};

struct Failure {
    std::string message;
};

std::vector<Test>& registry();

struct Registration {
    Registration(char const* name, std::function<void()> run);
};

void check(bool condition, char const* expression, char const* file, int line);
void checkEqual(std::wstring const& actual, std::wstring const& expected, char const* expression, char const* file,
                int line);

template <typename Work>
bool throws(Work work) {
    try {
        work();
    } catch (...) {
        return true;
    }

    return false;
}

}

#define TEST(name)                                                         \
    static void name();                                                    \
    static fileship::tests::Registration name##Registration(#name, name); \
    static void name()

#define CHECK(expression) fileship::tests::check((expression), #expression, __FILE__, __LINE__)
#define CHECK_EQUAL(actual, expected) \
    fileship::tests::checkEqual(std::wstring(actual), std::wstring(expected), #actual, __FILE__, __LINE__)
#define CHECK_THROWS(expression) \
    fileship::tests::check(fileship::tests::throws([&] { expression; }), "throws: " #expression, __FILE__, __LINE__)
