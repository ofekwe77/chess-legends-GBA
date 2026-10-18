// Host test runner: ./run_tests [group-or-name-filter ...]
#include "cl_test.h"

#include <chrono>
#include <cstdlib>
#include <cstring>

#include "cl_assert.h"

namespace
{
    constexpr int max_tests = 1024;
    cl_test::TestEntry tests[max_tests];
    int tests_count = 0;
    long long checks = 0;
    long long failures = 0;
    const char* current_test = "";
}

namespace cl_test
{
    void register_test(const char* group, const char* name, TestFunction function)
    {
        if(tests_count < max_tests)
        {
            tests[tests_count++] = { name, group, function };
        }
    }

    void report_failure(const char* expression, const char* file, int line, const char* detail)
    {
        ++failures;
        std::printf("  FAIL [%s] %s:%d: %s%s%s\n", current_test, file, line, expression, detail ? "  -- " : "",
                    detail ? detail : "");
    }

    void count_check()
    {
        ++checks;
    }
}

namespace cl
{
    [[noreturn]] void assert_failed(const char* condition, const char* file, int line)
    {
        std::printf("ASSERTION FAILED [%s] %s:%d: %s\n", current_test, file, line, condition);
        std::fflush(stdout);
        std::abort();
    }
}

int main(int argc, char** argv)
{
    int executed = 0;
    auto start = std::chrono::steady_clock::now();

    for(int i = 0; i < tests_count; ++i)
    {
        const cl_test::TestEntry& test = tests[i];
        bool selected = argc <= 1;

        for(int a = 1; a < argc && ! selected; ++a)
        {
            selected = std::strcmp(argv[a], test.group) == 0 || std::strcmp(argv[a], test.name) == 0;
        }

        if(! selected)
        {
            continue;
        }

        current_test = test.name;
        long long failures_before = failures;
        auto test_start = std::chrono::steady_clock::now();
        test.function();
        double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - test_start).count();
        std::printf("%-6s %-12s %-48s %7.2fs\n", failures == failures_before ? "ok" : "FAILED", test.group, test.name,
                    seconds);
        ++executed;
    }

    double total = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::printf("\n%d tests, %lld checks, %lld failures (%.1fs)\n", executed, checks, failures, total);
    return failures == 0 && executed > 0 ? 0 : 1;
}
