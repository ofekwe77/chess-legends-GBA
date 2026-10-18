// Minimal host-side test framework for CHESS LEGENDS: ARCANE WARS.
#ifndef CL_TEST_H
#define CL_TEST_H

#include <cstdint>
#include <cstdio>

namespace cl_test
{
    using TestFunction = void (*)();

    struct TestEntry
    {
        const char* name;
        const char* group;
        TestFunction function;
    };

    void register_test(const char* group, const char* name, TestFunction function);
    void report_failure(const char* expression, const char* file, int line, const char* detail);
    void count_check();

    struct Registrar
    {
        Registrar(const char* group, const char* name, TestFunction function)
        {
            register_test(group, name, function);
        }
    };

    template<typename A, typename B>
    void check_equal(const A& a, const B& b, const char* expression, const char* file, int line)
    {
        count_check();

        if(! (a == b))
        {
            char detail[160];
            std::snprintf(detail, sizeof(detail), "left=%lld right=%lld", (long long) a, (long long) b);
            report_failure(expression, file, line, detail);
        }
    }
}

#define CL_TEST_CONCAT2(a, b) a##b
#define CL_TEST_CONCAT(a, b) CL_TEST_CONCAT2(a, b)

#define TEST_CASE(group, name) \
    static void CL_TEST_CONCAT(test_, name)(); \
    static cl_test::Registrar CL_TEST_CONCAT(registrar_, name)(group, #name, CL_TEST_CONCAT(test_, name)); \
    static void CL_TEST_CONCAT(test_, name)()

#define CHECK(condition) \
    do { cl_test::count_check(); if(! (condition)) { cl_test::report_failure(#condition, __FILE__, __LINE__, nullptr); } } while(false)

#define CHECK_MSG(condition, message) \
    do { cl_test::count_check(); if(! (condition)) { cl_test::report_failure(#condition, __FILE__, __LINE__, message); } } while(false)

#define CHECK_EQ(a, b) cl_test::check_equal((a), (b), #a " == " #b, __FILE__, __LINE__)

#endif
