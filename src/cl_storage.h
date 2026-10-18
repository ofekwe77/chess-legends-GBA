// CHESS LEGENDS: ARCANE WARS - cartridge SRAM persistence.
//
// Moves the byte records of core/cl_save between SRAM and memory. Each record kind has two slots (A / B): a save
// always writes the slot not holding the newest valid copy, then reads it back and verifies it, so an interrupted
// or failed write never destroys the previous save.
#ifndef CL_STORAGE_H
#define CL_STORAGE_H

#include "cl_save.h"

namespace cl::gba
{

enum class LoadStatus : uint8_t
{
    LOADED,         // newest valid copy loaded
    RECOVERED,      // one copy was damaged; the other valid copy was loaded
    FRESH,          // no save yet (blank memory): defaults
    RESET,          // every copy damaged or from a newer format: defaults (the damaged data is left untouched)
};

[[nodiscard]] LoadStatus load_profile(Profile& profile);

// Returns false if the written record could not be verified.
bool save_profile(const Profile& profile);

[[nodiscard]] bool has_suspended_match();
[[nodiscard]] bool load_suspended_match(SuspendedMatch& match);
bool save_suspended_match(const SuspendedMatch& match);
void clear_suspended_match();

// Erases every record (options menu, after a double confirmation).
void erase_all_saves();

}

#endif
