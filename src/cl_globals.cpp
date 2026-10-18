// CHESS LEGENDS: ARCANE WARS - console-side global state.
#include "cl_globals.h"

#include "bn_common.h"

namespace cl::gba
{

namespace
{
    BN_DATA_EWRAM_BSS Globals globals_storage;
    BN_DATA_EWRAM_BSS cl_test_interface test_interface_storage;
    uint32_t cosmetic_state = 0x2545F491u;
}

Globals& globals()
{
    return globals_storage;
}

cl_test_interface& test_interface()
{
    return test_interface_storage;
}

uint32_t cosmetic_random()
{
    cosmetic_state ^= cosmetic_state << 13;
    cosmetic_state ^= cosmetic_state >> 17;
    cosmetic_state ^= cosmetic_state << 5;
    return cosmetic_state;
}

}
