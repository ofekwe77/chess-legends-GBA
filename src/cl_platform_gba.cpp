// CHESS LEGENDS: ARCANE WARS - GBA implementations of the core's platform hooks.
#include "bn_assert.h"

#include "cl_assert.h"

namespace cl
{
    [[noreturn]] void assert_failed(const char* condition, const char* file, int line)
    {
        BN_ERROR("Internal error: ", condition, "\n", file, ":", line);
        while(true)
        {
        }
    }
}
