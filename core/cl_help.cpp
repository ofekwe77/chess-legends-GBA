// CHESS LEGENDS: ARCANE WARS - player-facing help texts.
#include "cl_help.h"

#include "cl_assert.h"

namespace cl
{

namespace
{
    constexpr const char* rule_texts[ABILITIES_COUNT] = {
        "Shadow Step (2 mana): a pawn steps one square left or right onto an empty square. It never captures "
        "and never promotes.",
        "Phase Shift (3 mana): a knight teleports to any empty square next to it, in any of the eight "
        "directions.",
        "Ghost Walk (3 mana): a bishop moves along a diagonal through exactly one of your pieces, onto an empty "
        "square beyond it.",
        "Castle Swap (4 mana): a rook trades places with your knight or bishop on its rank or file, up to three "
        "squares away, with nothing in between.",
        "Royal Command (5 mana): the queen moves an adjacent pawn, knight, bishop or rook one step to an empty "
        "square that is also next to her. A pawn can never be sent to the first or last rank.",
        "Royal Escape (5 mana, once per game): only while in check, the king leaps two squares in a straight "
        "line onto an empty, safe square.",
    };

    constexpr const char* summaries[ABILITIES_COUNT] = {
        "Pawn steps one square sideways.",
        "Knight teleports one square.",
        "Bishop walks through one ally.",
        "Rook swaps with a knight or bishop.",
        "Queen moves an adjacent ally.",
        "Checked king leaps two squares.",
    };

    constexpr HelpLine controls_lines[] = {
        { "D-Pad", "Move the cursor" },
        { "A", "Select a piece, confirm a move" },
        { "B", "Cancel, go back" },
        { "L", "Switch normal moves / spells" },
        { "R", "Panel: piece, threats, game" },
        { "SELECT", "Rule card of the piece" },
        { "START", "Pause menu" },
    };

    constexpr HelpLine arcane_lines[] = {
        { "", "One action per turn: a normal move or one spell." },
        { "", "Mana: +1 at the start of each of your turns, up to 6. White starts with 1." },
        { "", "Spells never capture. Mate and stalemate count spells as legal actions." },
        { "", "In check, a spell may block, move a piece away or make the king leap." },
    };

    constexpr HelpLine classic_lines[] = {
        { "", "Checkmate the enemy king to win. Castling, en passant and promotion are allowed." },
        { "", "Draws: stalemate, dead positions, fivefold repetition and the 75-move rule." },
        { "", "You may claim a draw after a threefold repetition or 50 moves (pause menu)." },
    };

    constexpr HelpPage pages[help_pages_count] = {
        { "CONTROLS", controls_lines, int(sizeof(controls_lines) / sizeof(HelpLine)) },
        { "ARCANE RULES", arcane_lines, int(sizeof(arcane_lines) / sizeof(HelpLine)) },
        { "CHESS RULES", classic_lines, int(sizeof(classic_lines) / sizeof(HelpLine)) },
    };
}

const char* ability_rule_text(Ability ability)
{
    CL_ASSERT(int(ability) < ABILITIES_COUNT);
    return rule_texts[int(ability)];
}

const char* ability_summary(Ability ability)
{
    CL_ASSERT(int(ability) < ABILITIES_COUNT);
    return summaries[int(ability)];
}

const HelpPage& help_page(int index)
{
    CL_ASSERT(index >= 0 && index < help_pages_count);
    return pages[index];
}

}
