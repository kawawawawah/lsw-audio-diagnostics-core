// SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
// SPDX-License-Identifier: MIT

#pragma once

#include <cmath>
#include <iostream>
#include <vector>

namespace lsw::audio_diag::test
{
    using TestFunction = void (*)();

    struct TestCase
    {
        const char* name = "";
        TestFunction function = nullptr;
    };

    struct TestState
    {
        int passed = 0;
        int failed = 0;
        bool currentFailed = false;
    };

    inline std::vector<TestCase>& registry()
    {
        static std::vector<TestCase> tests;
        return tests;
    }

    inline TestState& state()
    {
        static TestState currentState;
        return currentState;
    }

    class Registrar
    {
    public:
        Registrar(const char* name, const TestFunction function)
        {
            registry().push_back({ name, function });
        }
    };

    inline void reportFailure(const char* expression, const char* file, const int line)
    {
        state().currentFailed = true;
        std::clog << "  failure: " << expression << " at " << file << ':' << line << '\n';
    }

    inline int runAll()
    {
        for (const TestCase& test : registry())
        {
            state().currentFailed = false;
            std::clog << "[ RUN      ] " << test.name << '\n';
            test.function();
            if (state().currentFailed)
            {
                ++state().failed;
                std::clog << "[  FAILED  ] " << test.name << '\n';
            }
            else
            {
                ++state().passed;
                std::clog << "[       OK ] " << test.name << '\n';
            }
        }
        std::clog << "Passed: " << state().passed << ", Failed: " << state().failed << '\n';
        return state().failed == 0 ? 0 : 1;
    }

    [[nodiscard]] inline bool near(const double left, const double right, const double tolerance)
    {
        return std::abs(left - right) <= tolerance;
    }
}

#define LSW_TEST_CASE(name) \
    static void name(); \
    static ::lsw::audio_diag::test::Registrar name##_registrar(#name, &name); \
    static void name()

#define LSW_CHECK(expression) \
    do \
    { \
        if (!(expression)) \
        { \
            ::lsw::audio_diag::test::reportFailure(#expression, __FILE__, __LINE__); \
            return; \
        } \
    } while (false)

#define LSW_CHECK_EQ(left, right) LSW_CHECK((left) == (right))
#define LSW_CHECK_NEAR(left, right, tolerance) \
    LSW_CHECK(::lsw::audio_diag::test::near(static_cast<double>(left), \
                                             static_cast<double>(right), \
                                             static_cast<double>(tolerance)))
