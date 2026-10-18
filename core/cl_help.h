// CHESS LEGENDS: ARCANE WARS - player-facing help texts (rule cards, controls, rules summary).
//
// Kept in core/ so the host content validator checks every line against the real font metrics.
#ifndef CL_HELP_H
#define CL_HELP_H

#include "cl_config.h"

namespace cl
{

struct HelpLine
{
    const char* key;        // left column (button or term), may be empty
    const char* text;
};

struct HelpPage
{
    const char* title;
    const HelpLine* lines;
    int count;
};

// Full rule text of an ability (shown by SELECT and in the rules page).
[[nodiscard]] const char* ability_rule_text(Ability ability);

// One-line summary (tutorial lists, gallery).
[[nodiscard]] const char* ability_summary(Ability ability);

constexpr int help_pages_count = 3;   // controls, arcane rules, classic rules
[[nodiscard]] const HelpPage& help_page(int index);

}

#endif
