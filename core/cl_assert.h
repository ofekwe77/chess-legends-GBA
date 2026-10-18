// CHESS LEGENDS: ARCANE WARS - assertions shared by the ROM and the host programs.
//
// CL_ASSERT is always active: an internal state inconsistency stops the program instead of silently
// continuing (host: message + abort, GBA: Butano error screen). CL_DEBUG_ASSERT is only compiled into host
// builds, so hot search paths stay fast on the console.
#ifndef CL_ASSERT_H
#define CL_ASSERT_H

namespace cl
{
    [[noreturn]] void assert_failed(const char* condition, const char* file, int line);
}

#define CL_ASSERT(condition) \
    do { if(! (condition)) [[unlikely]] { cl::assert_failed(#condition, __FILE__, __LINE__); } } while(false)

#if defined(CL_HOST)
    #define CL_DEBUG_ASSERT(condition) CL_ASSERT(condition)
#else
    #define CL_DEBUG_ASSERT(condition) do { } while(false)
#endif

#endif
