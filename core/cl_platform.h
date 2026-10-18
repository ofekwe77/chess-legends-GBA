// CHESS LEGENDS: ARCANE WARS - platform placement hints for the shared core.
//
// On the console the hottest functions run as ARM code from the 32 KB internal work RAM (IWRAM): they live in
// files named *.bn_iwram.cpp (compiled with -marm -mlong-calls by Butano) and carry CL_IWRAM_CODE. Small hot
// lookup tables are copied to IWRAM at boot through CL_IWRAM_DATA. Host builds ignore both.
#ifndef CL_PLATFORM_H
#define CL_PLATFORM_H

// Large zero-initialized buffers go to the 256 KB external work RAM through CL_EWRAM_BSS: IWRAM .bss shrinks the
// space left for the system stack (about 2 KB remain between IWRAM data and the stack top otherwise).
#if defined(CL_GBA)
    #define CL_IWRAM_CODE __attribute__((section(".iwram")))
    #define CL_IWRAM_DATA __attribute__((section(".iwram_data")))
    #define CL_EWRAM_BSS __attribute__((section(".sbss")))
#else
    #define CL_IWRAM_CODE
    #define CL_IWRAM_DATA
    #define CL_EWRAM_BSS
#endif

// Keeps IWRAM code compact: larger helpers are called instead of being duplicated at every call site.
#define CL_NOINLINE __attribute__((noinline))

#endif
