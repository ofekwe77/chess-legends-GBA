// CHESS LEGENDS: ARCANE WARS - the interactive tutorial.
#include "cl_tutorial.h"

namespace cl
{

namespace
{
    constexpr int8_t sq(int file, int rank)
    {
        return int8_t(rank * 16 + file);
    }

    constexpr int8_t none = int8_t(NO_SQUARE);

    constexpr Lesson lessons[lessons_count] = {
        { "The board",
          { "Welcome, tactician. The gold frame is your cursor: the D-pad moves it across the board.",
            "Press A to pick up one of your pieces, B to put it back down.", nullptr },
          "Move the cursor to the knight on g1 and press A.",
          "Well done. That is all the control you need to command an army.",
          nullptr, false, LessonGoal::SELECT_SQUARE, sq(6, 0), none, 0, {}, Difficulty::APPRENTICE },
        { "Moving pieces",
          { "A selected piece shows every legal destination as a dot. Only legal moves are ever shown.",
            "Knights jump in an L shape and are the only pieces that leap over others.", nullptr },
          "Select the knight on g1 and move it to f3.",
          "The knight is out, ready to join the battle.",
          nullptr, false, LessonGoal::MOVE, sq(6, 0), sq(5, 2), 0, {}, Difficulty::APPRENTICE },
        { "Pawns",
          { "Pawns move straight ahead and capture diagonally. On their first move they may advance two squares.",
            "A pawn that reaches the far side is promoted to a queen, rook, bishop or knight.", nullptr },
          "Advance the e-pawn two squares: from e2 to e4.",
          "The centre is yours. Pawns are slow, but they shape the whole game.",
          nullptr, false, LessonGoal::MOVE, sq(4, 1), sq(4, 3), 0, {}, Difficulty::APPRENTICE },
        { "Captures",
          { "Move onto an enemy piece to capture it. Captures are marked in red when you select a piece.",
            "Bishops slide along diagonals as far as the path is clear.", nullptr },
          "Capture the black rook with your bishop.",
          "A rook is worth five pawns: an excellent trade for nothing.",
          "4k3/8/8/5r2/8/3B4/8/4K3 w - - 0 1", false, LessonGoal::CAPTURE, none, none, ROOK, {},
          Difficulty::APPRENTICE },
        { "Check and checkmate",
          { "A king under attack is in check: it must escape at once, by moving, blocking or capturing.",
            "If no escape exists, it is checkmate and the game is won.", nullptr },
          "Checkmate the black king in one move.",
          "Checkmate! The king's own pawns trapped it on the back rank.",
          "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1", false, LessonGoal::OBJECTIVE, none, none, 0,
          { Objective::CHECKMATE_WITHIN, 1, 0 }, Difficulty::GRANDMASTER },
        { "Mana",
          { "Arcane Chess adds magic. Each side gains 1 mana at the start of its turn, up to 6.",
            "White begins with 1 mana, Black with none. The gems in the side panel count it.", nullptr },
          "Make any move. When your turn comes back you will hold 2 mana.",
          "Mana grows every turn. Spending it wisely is the heart of Arcane Chess.",
          nullptr, true, LessonGoal::REACH_MANA, none, none, 2, {}, Difficulty::APPRENTICE },
        { "Spells",
          { "Every piece type knows one spell. Select a piece and press L to see where its spell can go.",
            "Pawns know Shadow Step: one square sideways onto an empty square.", nullptr },
          "Select the d2 pawn, press L and step it sideways.",
          "Shadow Step! Spells never capture, but they open new paths.",
          "4k3/8/8/8/8/8/3P4/4K3 w - - 0 1 mana=2/0", true, LessonGoal::CAST, none, none, ABILITY_SHADOW_STEP, {},
          Difficulty::APPRENTICE },
        { "Spell costs",
          { "Spells cost mana: Shadow Step 2, Phase Shift and Ghost Walk 3, Castle Swap 4, the royal spells 5.",
            "The side panel shows each cost, in red when you cannot afford it yet.", nullptr },
          "You have 3 mana. Let the bishop Ghost Walk through its own pawn.",
          "Ghost Walk! The bishop passed through a friend. Watch your mana before you cast.",
          "4k3/8/8/8/8/2P5/1B6/4K3 w - - 0 1 mana=3/0", true, LessonGoal::CAST, none, none, ABILITY_GHOST_WALK,
          {}, Difficulty::APPRENTICE },
        { "King safety",
          { "Your king is in check and no square next to it is safe.",
            "Royal Escape (5 mana, once per game, only in check) lets the king leap two squares.", nullptr },
          "Escape the check and survive the reply.",
          "Saved! Keeping 5 mana in reserve can save a king. It works only once per game.",
          "6k1/5ppp/8/8/8/8/5PPP/4r1K1 w - - 0 1 mana=5/0", true, LessonGoal::OBJECTIVE, none, none, 0,
          { Objective::SURVIVE, 1, 0 }, Difficulty::GRANDMASTER },
        { "Combinations",
          { "Spells combine. Castle Swap trades a rook with a knight or bishop up to three squares away.",
            "Phase Shift teleports a knight to any empty square next to it.", nullptr },
          "Mate in two: Castle Swap, then Phase Shift.",
          "Two spells, one checkmate. You are ready for the Arcane Wars.",
          "1k6/RN6/2K5/3r4/8/8/8/8 w - - 0 1 mana=6/0", true, LessonGoal::OBJECTIVE, none, none, 0,
          { Objective::CHECKMATE_WITHIN, 2, 0 }, Difficulty::GRANDMASTER },
    };
}

const Lesson& lesson(int index)
{
    CL_ASSERT(index >= 0 && index < lessons_count);
    return lessons[index];
}

bool setup_lesson(const Lesson& l, Game& game)
{
    const RuleSet rules = l.arcane ? RuleSet::arcane_rules() : RuleSet::classic();

    if(! l.fen)
    {
        game.start(rules);
        return true;
    }

    return game.setup(l.fen, rules, false) == FenError::NONE;
}

bool lesson_action_completes(const Lesson& l, const ActionInfo& info)
{
    switch(l.goal)
    {
    case LessonGoal::MOVE:
        return info.move.from() == l.from && info.move.to() == l.to;

    case LessonGoal::CAPTURE:
        return info.captured != EMPTY && type_of(info.captured) == PieceType(l.value);

    case LessonGoal::CAST:
        return info.move.is_magic() && int(info.move.ability()) == l.value;

    default:
        return false;
    }
}

}
