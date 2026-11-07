#pragma once
#include <exception>
#include <initializer_list>
#include <iostream>
#include <string>
#include <utility>

using TestCase = std::pair<const char*, void (*)()>;

inline int runTests(int argc, char** argv, std::initializer_list<TestCase> cases) {
    if (argc > 2) { std::cerr << "Usage: test_executable [case_name]\n"; return 2; }
    int executed = 0;
    int failures = 0;
    for (const auto& test : cases) {
        if (argc == 2 && std::string(argv[1]) != test.first) continue;
        ++executed;
        try {
            test.second();
            std::cout << "PASS " << test.first << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "FAIL " << test.first << ": " << error.what() << '\n';
        }
    }
    if (executed == 0) { std::cerr << "Unknown test case\n"; return 2; }
    std::cout << executed - failures << '/' << executed << " cases passed\n";
    return failures == 0 ? 0 : 1;
}
