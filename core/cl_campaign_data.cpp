// CHESS LEGENDS: ARCANE WARS - campaign, story and challenge content.
//
// Every special rule of an encounter is part of its briefing text, so the player always knows the exact
// conditions before playing (design section 48). Positions are validated by tools/validation/validate.cpp.
#include "cl_campaign.h"

namespace cl
{

namespace
{
    using C = Character;
    using E = Expression;

    // -------------------------------------------------------------------------------------------- characters

    struct CharacterInfo
    {
        const char* name;
        const char* title;
    };

    constexpr CharacterInfo characters[int(C::COUNT)] = {
        { "Arden", "Royal Tactician" },
        { "Orrin", "Keeper of the Board" },
        { "Rowan Thorn", "The Emerald Warden" },
        { "Veyra Ember", "The Ash Queen" },
        { "Ysra Veil", "The Frost Empress" },
        { "Noctis Vane", "The Shadow Scholar" },
        { "Rael Tempest", "The Thunder Sovereign" },
        { "Aurelia Voss", "The Last Sovereign" },
        { "Brin", "Mossguard of the March" },
        { "Harrik", "Ember Knight" },
        { "Elsk", "Mirror Warden" },
        { "Quill", "Keeper of Forbidden Pages" },
        { "Tamsin", "Sky Engineer" },
        { "Seraph", "Herald of Dawn" },
        { "", "" },
    };

#define CL_DIALOGUE(name) constexpr Dialogue name##_d = { name, int(sizeof(name) / sizeof(DialogueLine)) }

    // -------------------------------------------------------------------------------------------- prologue

    constexpr DialogueLine prologue_lines[] = {
        { C::NARRATOR, E::NEUTRAL, "Long ago, the Crown of Six bound six kingdoms in a single peace." },
        { C::NARRATOR, E::NEUTRAL, "One night it shattered. Six fragments fell into the hands of six rulers." },
        { C::NARRATOR, E::NEUTRAL, "Since then the kingdoms wage war on the enchanted board: the Arcane Wars." },
        { C::ORRIN, E::NEUTRAL, "Arden. You have studied every opening I know. Now the board must teach you." },
        { C::ARDEN, E::NEUTRAL, "Then I will win back the fragments, one kingdom at a time." },
        { C::ORRIN, E::INTENSE, "Win them, yes. But learn why the crown broke. The truth matters more." },
    };
    CL_DIALOGUE(prologue_lines);

    constexpr DialogueLine epilogue_lines[] = {
        { C::NARRATOR, E::NEUTRAL, "The six fragments rise from the six rulers and meet above the board." },
        { C::AURELIA, E::DEFEATED, "I broke the crown myself, long ago. No single hand should ever hold it." },
        { C::AURELIA, E::NEUTRAL, "I meant to reforge it alone. You showed me another way to win." },
        { C::ARDEN, E::NEUTRAL, "Then let six hands hold it. One board, six kingdoms, one rule for all." },
        { C::ORRIN, E::NEUTRAL, "The kingdoms will need a guardian for that board. They have chosen you." },
        { C::NARRATOR, E::NEUTRAL, "The Crown of Six shines again, and the Arcane Wars become a game once more." },
    };
    CL_DIALOGUE(epilogue_lines);

    // -------------------------------------------------------------------------------------------- worlds

    constexpr DialogueLine w1_intro_lines[] = {
        { C::NARRATOR, E::NEUTRAL, "WORLD 1 - THE VERDANT MARCH. Ancient forests guard a sleeping temple." },
        { C::ORRIN, E::NEUTRAL, "Rowan Thorn keeps the Emerald fragment. She values patience above all." },
    };
    CL_DIALOGUE(w1_intro_lines);

    constexpr DialogueLine w2_intro_lines[] = {
        { C::NARRATOR, E::NEUTRAL, "WORLD 2 - THE INFERNO BASTION. Black iron walls rise over rivers of lava." },
        { C::ORRIN, E::INTENSE, "Veyra Ember attacks before her enemies can breathe. Do not fall behind." },
    };
    CL_DIALOGUE(w2_intro_lines);

    constexpr DialogueLine w3_intro_lines[] = {
        { C::NARRATOR, E::NEUTRAL, "WORLD 3 - THE FROZEN COURT. Crystal halls where mirrors remember everything." },
        { C::ORRIN, E::NEUTRAL, "Ysra Veil builds walls of ice and waits. Patience will be your weapon too." },
    };
    CL_DIALOGUE(w3_intro_lines);

    constexpr DialogueLine w4_intro_lines[] = {
        { C::NARRATOR, E::NEUTRAL, "WORLD 4 - THE UMBRAL ARCHIVE. Shadows drift between endless shelves." },
        { C::ORRIN, E::NEUTRAL, "Noctis Vane plays tricks with every piece. Expect the knight from nowhere." },
    };
    CL_DIALOGUE(w4_intro_lines);

    constexpr DialogueLine w5_intro_lines[] = {
        { C::NARRATOR, E::NEUTRAL, "WORLD 5 - THE STORM SPIRE. A citadel of floating stone in endless thunder." },
        { C::ORRIN, E::INTENSE, "Rael Tempest never stops moving. Keep your pieces active, or be swept away." },
    };
    CL_DIALOGUE(w5_intro_lines);

    constexpr DialogueLine w6_intro_lines[] = {
        { C::NARRATOR, E::NEUTRAL, "WORLD 6 - THE CROWN NEXUS. The ruined capital, between light and darkness." },
        { C::ORRIN, E::NEUTRAL, "Five fragments answer to you now. The last waits with Aurelia Voss." },
    };
    CL_DIALOGUE(w6_intro_lines);

    // -------------------------------------------------------------------------------------------- World 1

    constexpr DialogueLine e1_intro_lines[] = {
        { C::ORRIN, E::NEUTRAL, "Before the Warden, a practice game. My army is small; yours is not." },
        { C::ORRIN, E::NEUTRAL, "Corner my king and checkmate it. Mana flows +1 each turn, up to 6." },
    };
    CL_DIALOGUE(e1_intro_lines);

    constexpr DialogueLine e1_win_lines[] = {
        { C::ORRIN, E::NEUTRAL, "Clean and calm. The forest is listening to you already." },
    };
    CL_DIALOGUE(e1_win_lines);

    constexpr DialogueLine e1_lose_lines[] = {
        { C::ORRIN, E::NEUTRAL, "A lesson is never lost. Again, and slower this time." },
    };
    CL_DIALOGUE(e1_lose_lines);

    constexpr DialogueLine e2_intro_lines[] = {
        { C::BRIN, E::NEUTRAL, "The roots hold every pawn in place. Nothing passes the Mossguard." },
        { C::ARDEN, E::NEUTRAL, "Then I will step around the roots." },
    };
    CL_DIALOGUE(e2_intro_lines);

    constexpr DialogueLine e2_win_lines[] = {
        { C::BRIN, E::DEFEATED, "You walked between the shadows of the roots... The Warden will see you." },
    };
    CL_DIALOGUE(e2_win_lines);

    constexpr DialogueLine e2_lose_lines[] = {
        { C::BRIN, E::INTENSE, "Rooted! Step your b-pawn aside. Whatever I capture, one of your pawns runs free." },
    };
    CL_DIALOGUE(e2_lose_lines);

    constexpr DialogueLine e3_intro_lines[] = {
        { C::ROWAN, E::NEUTRAL, "Balance is earned, not taken. Show me your patience, apprentice." },
        { C::ROWAN, E::INTENSE, "Every piece you waste feeds my forest." },
    };
    CL_DIALOGUE(e3_intro_lines);

    constexpr DialogueLine e3_win_lines[] = {
        { C::ROWAN, E::DEFEATED, "The forest yields. Take the Emerald fragment." },
        { C::ROWAN, E::NEUTRAL, "Remember: the crown was never meant for one hand." },
    };
    CL_DIALOGUE(e3_win_lines);

    constexpr DialogueLine e3_lose_lines[] = {
        { C::ROWAN, E::NEUTRAL, "Your roots are shallow. Return when they run deeper." },
    };
    CL_DIALOGUE(e3_lose_lines);

    // -------------------------------------------------------------------------------------------- World 2

    constexpr DialogueLine e4_intro_lines[] = {
        { C::HARRIK, E::INTENSE, "Your army stands ready while mine still sleeps in the barracks. Strike!" },
        { C::HARRIK, E::INTENSE, "Mate me within 35 moves, or the Ash Queen will laugh at us both." },
    };
    CL_DIALOGUE(e4_intro_lines);

    constexpr DialogueLine e4_win_lines[] = {
        { C::HARRIK, E::DEFEATED, "Fast. Faster than the fire. She will want to burn you herself." },
    };
    CL_DIALOGUE(e4_win_lines);

    constexpr DialogueLine e4_lose_lines[] = {
        { C::HARRIK, E::NEUTRAL, "Development is a lead that melts. Use it before it is gone." },
    };
    CL_DIALOGUE(e4_lose_lines);

    constexpr DialogueLine e5_intro_lines[] = {
        { C::HARRIK, E::NEUTRAL, "The Iron Gate. Two moves to checkmate my king, or the gate stays shut." },
        { C::ARDEN, E::NEUTRAL, "Castle Swap: a rook trades places with a knight or bishop. The walls can move." },
    };
    CL_DIALOGUE(e5_intro_lines);

    constexpr DialogueLine e5_win_lines[] = {
        { C::HARRIK, E::DEFEATED, "The gate falls. Go. The Queen is waiting in the flames." },
    };
    CL_DIALOGUE(e5_win_lines);

    constexpr DialogueLine e5_lose_lines[] = {
        { C::HARRIK, E::INTENSE, "Your rook and bishop stand side by side. Swap them, and my king has nowhere to run." },
    };
    CL_DIALOGUE(e5_lose_lines);

    constexpr DialogueLine e6_intro_lines[] = {
        { C::VEYRA, E::INTENSE, "You crossed my bastion alive? Then you will burn on my board instead." },
        { C::VEYRA, E::INTENSE, "I never defend. I never wait. Keep up, little tactician." },
    };
    CL_DIALOGUE(e6_intro_lines);

    constexpr DialogueLine e6_win_lines[] = {
        { C::VEYRA, E::DEFEATED, "Ash and embers... Fine. The Ruby fragment is yours." },
        { C::VEYRA, E::NEUTRAL, "The night it broke, the Frost Empress was there. Ask her what she saw." },
    };
    CL_DIALOGUE(e6_win_lines);

    constexpr DialogueLine e6_lose_lines[] = {
        { C::VEYRA, E::INTENSE, "Too slow! Come back when you can survive the first wave." },
    };
    CL_DIALOGUE(e6_lose_lines);

    // -------------------------------------------------------------------------------------------- World 3

    constexpr DialogueLine e7_intro_lines[] = {
        { C::ELSK, E::NEUTRAL, "The winter is long. My court begins with full mana; you begin with two." },
        { C::ELSK, E::INTENSE, "Survive fifteen moves without being checkmated. If you can." },
    };
    CL_DIALOGUE(e7_intro_lines);

    constexpr DialogueLine e7_win_lines[] = {
        { C::ELSK, E::DEFEATED, "You spent your mana like a miser. That is how winters are survived." },
    };
    CL_DIALOGUE(e7_win_lines);

    constexpr DialogueLine e7_lose_lines[] = {
        { C::ELSK, E::NEUTRAL, "Keep five mana for Royal Escape. A king who can flee cannot be cornered." },
    };
    CL_DIALOGUE(e7_lose_lines);

    constexpr DialogueLine e8_intro_lines[] = {
        { C::ELSK, E::NEUTRAL, "Your bishop is frozen behind your own pawn. Checkmate me within three moves." },
        { C::ARDEN, E::NEUTRAL, "Ghost Walk lets a bishop pass through one friend. Ice is no wall." },
    };
    CL_DIALOGUE(e8_intro_lines);

    constexpr DialogueLine e8_win_lines[] = {
        { C::ELSK, E::DEFEATED, "Through the mirror itself... The Empress will receive you." },
    };
    CL_DIALOGUE(e8_win_lines);

    constexpr DialogueLine e8_lose_lines[] = {
        { C::ELSK, E::NEUTRAL, "Crown your pawn with check first. Then the bishop walks through the ice." },
    };
    CL_DIALOGUE(e8_lose_lines);

    constexpr DialogueLine e9_intro_lines[] = {
        { C::YSRA, E::NEUTRAL, "Every move you make, my mirrors have already seen." },
        { C::YSRA, E::NEUTRAL, "I do not attack. I wait until you break yourself against the ice." },
    };
    CL_DIALOGUE(e9_intro_lines);

    constexpr DialogueLine e9_win_lines[] = {
        { C::YSRA, E::DEFEATED, "The ice... cracks. Take the Sapphire fragment." },
        { C::YSRA, E::NEUTRAL, "I saw the night it broke. Aurelia stood at the center. Alone." },
    };
    CL_DIALOGUE(e9_win_lines);

    constexpr DialogueLine e9_lose_lines[] = {
        { C::YSRA, E::NEUTRAL, "You struck the wall instead of the door. Patience, apprentice." },
    };
    CL_DIALOGUE(e9_lose_lines);

    // -------------------------------------------------------------------------------------------- World 4

    constexpr DialogueLine e10_intro_lines[] = {
        { C::QUILL, E::NEUTRAL, "This labyrinth of pages hides three of my pawns. Find and capture them all." },
        { C::QUILL, E::INTENSE, "You have two knights and twenty-five moves. Knights never walk straight." },
    };
    CL_DIALOGUE(e10_intro_lines);

    constexpr DialogueLine e10_win_lines[] = {
        { C::QUILL, E::DEFEATED, "Every page found. The Scholar will be... intrigued." },
    };
    CL_DIALOGUE(e10_win_lines);

    constexpr DialogueLine e10_lose_lines[] = {
        { C::QUILL, E::NEUTRAL, "Lost in the stacks. Plan two jumps ahead, not one." },
    };
    CL_DIALOGUE(e10_lose_lines);

    constexpr DialogueLine e11_intro_lines[] = {
        { C::QUILL, E::INTENSE, "A knight that steps one square aside sees a different board." },
        { C::ARDEN, E::NEUTRAL, "Phase Shift. Three mana, and a fork appears from nowhere." },
    };
    CL_DIALOGUE(e11_intro_lines);

    constexpr DialogueLine e11_win_lines[] = {
        { C::QUILL, E::DEFEATED, "From nowhere indeed. Noctis will want to meet you." },
    };
    CL_DIALOGUE(e11_win_lines);

    constexpr DialogueLine e11_lose_lines[] = {
        { C::QUILL, E::NEUTRAL, "Look at the squares next to your knight. One of them checks the king and hits the queen." },
    };
    CL_DIALOGUE(e11_lose_lines);

    constexpr DialogueLine e12_intro_lines[] = {
        { C::NOCTIS, E::INTENSE, "Ah, the fragment collector. Shall we see who reads the board better?" },
        { C::NOCTIS, E::NEUTRAL, "Every piece of mine has a second purpose. Find it, or fall to it." },
    };
    CL_DIALOGUE(e12_intro_lines);

    constexpr DialogueLine e12_win_lines[] = {
        { C::NOCTIS, E::DEFEATED, "Read and understood. Take the Amethyst fragment." },
        { C::NOCTIS, E::NEUTRAL, "My archive holds one page about that night: the crown was not stolen. It was broken." },
    };
    CL_DIALOGUE(e12_win_lines);

    constexpr DialogueLine e12_lose_lines[] = {
        { C::NOCTIS, E::INTENSE, "A trick you did not see is still a trick. Read deeper." },
    };
    CL_DIALOGUE(e12_lose_lines);

    // -------------------------------------------------------------------------------------------- World 5

    constexpr DialogueLine e13_intro_lines[] = {
        { C::TAMSIN, E::NEUTRAL, "The open board is my workshop. Every piece should be moving!" },
        { C::TAMSIN, E::INTENSE, "Checkmate me within forty moves. Keep the initiative." },
    };
    CL_DIALOGUE(e13_intro_lines);

    constexpr DialogueLine e13_win_lines[] = {
        { C::TAMSIN, E::DEFEATED, "Full throttle! Rael will love this. Or hate it. Probably both." },
    };
    CL_DIALOGUE(e13_win_lines);

    constexpr DialogueLine e13_lose_lines[] = {
        { C::TAMSIN, E::NEUTRAL, "You let the storm pass you by. Open lines, active pieces!" },
    };
    CL_DIALOGUE(e13_lose_lines);

    constexpr DialogueLine e14_intro_lines[] = {
        { C::TAMSIN, E::NEUTRAL, "Chain Lightning: two spells, one after the other, then checkmate." },
        { C::TAMSIN, E::INTENSE, "Mate within three moves. Count your mana: the second spell must still be affordable." },
    };
    CL_DIALOGUE(e14_intro_lines);

    constexpr DialogueLine e14_win_lines[] = {
        { C::TAMSIN, E::DEFEATED, "Two sparks, one bolt. The Sovereign is ready for you." },
    };
    CL_DIALOGUE(e14_win_lines);

    constexpr DialogueLine e14_lose_lines[] = {
        { C::TAMSIN, E::NEUTRAL, "Your rook and knight should trade places first. Then let the knight strike." },
    };
    CL_DIALOGUE(e14_lose_lines);

    constexpr DialogueLine e15_intro_lines[] = {
        { C::RAEL, E::INTENSE, "Thunder does not ask permission. Neither do I." },
        { C::RAEL, E::NEUTRAL, "Show me a game where every piece fights, and the Topaz fragment is yours." },
    };
    CL_DIALOGUE(e15_intro_lines);

    constexpr DialogueLine e15_win_lines[] = {
        { C::RAEL, E::DEFEATED, "Ha! Struck down by my own storm. Take it, and go to the Nexus." },
        { C::RAEL, E::NEUTRAL, "Aurelia gathers fragments too. She means to wear the crown herself." },
    };
    CL_DIALOGUE(e15_win_lines);

    constexpr DialogueLine e15_lose_lines[] = {
        { C::RAEL, E::INTENSE, "Lightning hits the tallest tower first. Keep your king lower!" },
    };
    CL_DIALOGUE(e15_lose_lines);

    // -------------------------------------------------------------------------------------------- World 6

    constexpr DialogueLine e16_intro_lines[] = {
        { C::SERAPH, E::NEUTRAL, "In the Nexus the old magic is thin. Mana stops at four for both of us." },
        { C::SERAPH, E::NEUTRAL, "Spend wisely: Royal Command and Royal Escape are out of reach here." },
    };
    CL_DIALOGUE(e16_intro_lines);

    constexpr DialogueLine e16_win_lines[] = {
        { C::SERAPH, E::DEFEATED, "Little magic, great skill. You may pass the Dawn Gate." },
    };
    CL_DIALOGUE(e16_win_lines);

    constexpr DialogueLine e16_lose_lines[] = {
        { C::SERAPH, E::NEUTRAL, "With four mana you choose one spell at a time. Choose better." },
    };
    CL_DIALOGUE(e16_lose_lines);

    constexpr DialogueLine e17_intro_lines[] = {
        { C::SERAPH, E::INTENSE, "A riddle of the Dawn Gate: checkmate me within three moves." },
        { C::SERAPH, E::NEUTRAL, "My knight will hunt your king. A king who can leap need not fear it." },
    };
    CL_DIALOGUE(e17_intro_lines);

    constexpr DialogueLine e17_win_lines[] = {
        { C::SERAPH, E::DEFEATED, "The riddle is answered. She is waiting beneath the floating crown." },
    };
    CL_DIALOGUE(e17_win_lines);

    constexpr DialogueLine e17_lose_lines[] = {
        { C::SERAPH, E::NEUTRAL, "Threaten mate first. When my knight checks you, Royal Escape keeps the threat alive." },
    };
    CL_DIALOGUE(e17_lose_lines);

    constexpr DialogueLine e18_intro_lines[] = {
        { C::AURELIA, E::NEUTRAL, "Five fragments. You carried them across every kingdom for me." },
        { C::AURELIA, E::INTENSE, "Now place them on my board. I understand every spell you know." },
    };
    CL_DIALOGUE(e18_intro_lines);

    constexpr DialogueLine e18_win_lines[] = {
        { C::AURELIA, E::DEFEATED, "Checkmate... by someone who never wanted the crown." },
    };
    CL_DIALOGUE(e18_win_lines);

    constexpr DialogueLine e18_lose_lines[] = {
        { C::AURELIA, E::INTENSE, "Every mechanic, mastered. Return when you master yourself." },
    };
    CL_DIALOGUE(e18_lose_lines);

    // -------------------------------------------------------------------------------------------- tables

    constexpr World worlds[worlds_count] = {
        { "THE VERDANT MARCH", "Forest of the Emerald Warden", C::ROWAN, C::BRIN, 1, 1, w1_intro_lines_d },
        { "THE INFERNO BASTION", "Fortress of the Ash Queen", C::VEYRA, C::HARRIK, 2, 2, w2_intro_lines_d },
        { "THE FROZEN COURT", "Palace of the Frost Empress", C::YSRA, C::ELSK, 3, 3, w3_intro_lines_d },
        { "THE UMBRAL ARCHIVE", "Library of the Shadow Scholar", C::NOCTIS, C::QUILL, 4, 4, w4_intro_lines_d },
        { "THE STORM SPIRE", "Citadel of the Thunder Sovereign", C::RAEL, C::TAMSIN, 5, 5, w5_intro_lines_d },
        { "THE CROWN NEXUS", "Ruins of the First Capital", C::AURELIA, C::SERAPH, 6, 6, w6_intro_lines_d },
    };

    constexpr uint8_t ALL = ABILITY_MASK_ALL;
    constexpr uint8_t NO_ROYAL = ABILITY_MASK_ALL & ~((1 << ABILITY_ROYAL_COMMAND) | (1 << ABILITY_ROYAL_ESCAPE));

    constexpr Encounter encounters[campaign_encounters] = {
        // ---- World 1: The Verdant March ----
        { 0, EncounterType::LESSON, "The First Lesson", C::ORRIN,
          "4k3/3ppp2/8/8/8/8/8/R2QK3 w Q - 0 1", WHITE, { Objective::WIN, 0, 0 },
          Difficulty::APPRENTICE, Personality::BALANCED, 60, 0, ALL, ALL,
          "Objective: checkmate Orrin. Standard Arcane rules.",
          e1_intro_lines_d, e1_win_lines_d, e1_lose_lines_d },
        { 0, EncounterType::TRIAL, "Roots and Pawns", C::BRIN,
          "7k/8/1p6/1P1p4/3P4/8/8/4K3 w - - 0 1 mana=2/0", WHITE, { Objective::PROMOTE, 9, 0 },
          Difficulty::KNIGHT, Personality::DEFENSIVE, 100, 0, ALL, ALL,
          "Objective: promote a pawn within 9 moves. Your pawns are blocked; you start with 2 mana, enough for "
          "one Shadow Step (a pawn steps one square sideways).",
          e2_intro_lines_d, e2_win_lines_d, e2_lose_lines_d },
        { 0, EncounterType::BOSS, "The Emerald Warden", C::ROWAN, nullptr, WHITE, { Objective::WIN, 0, 0 },
          Difficulty::KNIGHT, Personality::POSITIONAL, 100, 0, ALL, ALL,
          "Boss: Rowan Thorn. Objective: checkmate. Standard Arcane rules, standard start.",
          e3_intro_lines_d, e3_win_lines_d, e3_lose_lines_d },

        // ---- World 2: The Inferno Bastion ----
        { 1, EncounterType::TRIAL, "Trial by Fire", C::HARRIK,
          "rnbqkbnr/ppp2ppp/8/3pp3/2B1P3/2N2N2/PPPP1PPP/R1BQ1RK1 w kq - 0 6", WHITE,
          { Objective::CHECKMATE_WITHIN, 35, 0 }, Difficulty::KNIGHT, Personality::AGGRESSIVE, 100, 0, ALL, ALL,
          "Objective: checkmate within 35 moves. You are developed and castled; Harrik is not.",
          e4_intro_lines_d, e4_win_lines_d, e4_lose_lines_d },
        { 1, EncounterType::TRIAL, "The Iron Gate", C::HARRIK,
          "2k5/8/4N3/5p2/RB6/8/8/5K1R w - - 0 1 mana=6/0", WHITE,
          { Objective::CHECKMATE_WITHIN, 2, 0 }, Difficulty::GRANDMASTER, Personality::TACTICAL, 100, 0, ALL, ALL,
          "Objective: checkmate within 2 moves. You have 6 mana, Harrik has none.",
          e5_intro_lines_d, e5_win_lines_d, e5_lose_lines_d },
        { 1, EncounterType::BOSS, "The Ash Queen", C::VEYRA, nullptr, WHITE, { Objective::WIN, 0, 0 },
          Difficulty::KNIGHT, Personality::AGGRESSIVE, 150, 0, ALL, ALL,
          "Boss: Veyra Ember (aggressive). Objective: checkmate. Standard Arcane rules.",
          e6_intro_lines_d, e6_win_lines_d, e6_lose_lines_d },

        // ---- World 3: The Frozen Court ----
        { 2, EncounterType::TRIAL, "The Long Winter", C::ELSK,
          "r1b2rk1/ppp2ppp/2n5/3q4/3P4/2P2N2/P4PPP/R2QKB1R w KQ - 0 10 mana=2/6", WHITE,
          { Objective::SURVIVE, 15, 0 }, Difficulty::KNIGHT, Personality::AGGRESSIVE, 100, 0, ALL, ALL,
          "Objective: avoid defeat for 15 moves. Elsk starts with 6 mana, you start with 2.",
          e7_intro_lines_d, e7_win_lines_d, e7_lose_lines_d },
        { 2, EncounterType::TRIAL, "Through the Ice", C::ELSK,
          "7k/5P2/8/4K3/p3P3/7r/r7/1B6 w - - 0 1 mana=6/0", WHITE,
          { Objective::CHECKMATE_WITHIN, 3, 0 }, Difficulty::GRANDMASTER, Personality::DEFENSIVE, 100, 0, ALL, ALL,
          "Objective: checkmate within 3 moves. You have 6 mana, Elsk has none.",
          e8_intro_lines_d, e8_win_lines_d, e8_lose_lines_d },
        { 2, EncounterType::BOSS, "The Frost Empress", C::YSRA, nullptr, WHITE, { Objective::WIN, 0, 0 },
          Difficulty::SORCERER, Personality::DEFENSIVE, 80, 0, ALL, ALL,
          "Boss: Ysra Veil (defensive). Objective: checkmate. Standard Arcane rules.",
          e9_intro_lines_d, e9_win_lines_d, e9_lose_lines_d },

        // ---- World 4: The Umbral Archive ----
        { 3, EncounterType::TRIAL, "Knight's Labyrinth", C::QUILL,
          "8/3k4/8/2p1p3/6p1/2N1N3/8/4K3 w - - 0 1", WHITE, { Objective::CAPTURE_PAWNS, 25, 0 },
          Difficulty::KNIGHT, Personality::TRICKSTER, 100, 0, ALL, ALL,
          "Objective: capture all three enemy pawns within 25 moves.",
          e10_intro_lines_d, e10_win_lines_d, e10_lose_lines_d },
        { 3, EncounterType::TRIAL, "Shadow Leap", C::QUILL,
          "r2q1r2/pbp3kp/1p1p2p1/3N1p2/8/3Q4/PPP2PPP/R2R2K1 w - - 0 16 mana=3/1", WHITE,
          { Objective::WIN_MATERIAL, 3, 3 }, Difficulty::SORCERER, Personality::TRICKSTER, 100, 0, ALL, ALL,
          "Objective: lead by 3 more pawns of material within 3 moves. You have 3 mana, Quill has 1.",
          e11_intro_lines_d, e11_win_lines_d, e11_lose_lines_d },
        { 3, EncounterType::BOSS, "The Shadow Scholar", C::NOCTIS, nullptr, WHITE, { Objective::WIN, 0, 0 },
          Difficulty::SORCERER, Personality::TRICKSTER, 100, 0, ALL, ALL,
          "Boss: Noctis Vane (trickster). Objective: checkmate. Standard Arcane rules.",
          e12_intro_lines_d, e12_win_lines_d, e12_lose_lines_d },

        // ---- World 5: The Storm Spire ----
        { 4, EncounterType::TRIAL, "Eye of the Storm", C::TAMSIN,
          "r2qk2r/ppp2ppp/2n5/3np3/2B5/2N2N2/PPPP1PPP/R1BQ1RK1 w kq - 0 8", WHITE,
          { Objective::CHECKMATE_WITHIN, 40, 0 }, Difficulty::KNIGHT, Personality::AGGRESSIVE, 100, 0, ALL, ALL,
          "Objective: checkmate within 40 moves. Open lines favour the active side.",
          e13_intro_lines_d, e13_win_lines_d, e13_lose_lines_d },
        { 4, EncounterType::TRIAL, "Chain Lightning", C::TAMSIN,
          "1k6/RN6/2K5/3r4/8/8/8/8 w - - 0 1 mana=6/0", WHITE, { Objective::CHECKMATE_WITHIN, 3, 0 },
          Difficulty::GRANDMASTER, Personality::BALANCED, 100, 0, ALL, ALL,
          "Objective: checkmate within 3 moves. You have 6 mana, Tamsin has none.",
          e14_intro_lines_d, e14_win_lines_d, e14_lose_lines_d },
        { 4, EncounterType::BOSS, "The Thunder Sovereign", C::RAEL, nullptr, WHITE, { Objective::WIN, 0, 0 },
          Difficulty::SORCERER, Personality::TACTICAL, 130, 0, ALL, ALL,
          "Boss: Rael Tempest (tactical). Objective: checkmate. Standard Arcane rules.",
          e15_intro_lines_d, e15_win_lines_d, e15_lose_lines_d },

        // ---- World 6: The Crown Nexus ----
        { 5, EncounterType::TRIAL, "Crown of Ash", C::SERAPH, nullptr, WHITE, { Objective::WIN, 0, 0 },
          Difficulty::SORCERER, Personality::POSITIONAL, 80, 4, NO_ROYAL, NO_ROYAL,
          "Objective: checkmate. Special rules for both sides: mana is capped at 4; Royal Command and Royal "
          "Escape are sealed.",
          e16_intro_lines_d, e16_win_lines_d, e16_lose_lines_d },
        { 5, EncounterType::TRIAL, "The Herald's Riddle", C::SERAPH,
          "2k5/8/8/1R6/4K3/8/8/3n1NQ1 w - - 0 1 mana=6/0", WHITE, { Objective::CHECKMATE_WITHIN, 3, 0 },
          Difficulty::GRANDMASTER, Personality::BALANCED, 100, 0, ALL, ALL,
          "Objective: checkmate within 3 moves against the strongest defense. You have 6 mana.",
          e17_intro_lines_d, e17_win_lines_d, e17_lose_lines_d },
        { 5, EncounterType::BOSS, "The Last Sovereign", C::AURELIA, nullptr, WHITE, { Objective::WIN, 0, 0 },
          Difficulty::GRANDMASTER, Personality::MASTER, 100, 0, ALL, ALL,
          "Final boss: Aurelia Voss (master). Objective: checkmate. Standard Arcane rules.",
          e18_intro_lines_d, e18_win_lines_d, e18_lose_lines_d },
    };

    // -------------------------------------------------------------------------------------------- challenges

    constexpr uint8_t NONE = 0;
    constexpr uint8_t SS = 1 << ABILITY_SHADOW_STEP;
    constexpr uint8_t PS = 1 << ABILITY_PHASE_SHIFT;
    constexpr uint8_t GW = 1 << ABILITY_GHOST_WALK;
    constexpr uint8_t CS = 1 << ABILITY_CASTLE_SWAP;
    constexpr uint8_t RC = 1 << ABILITY_ROYAL_COMMAND;
    constexpr uint8_t RE = 1 << ABILITY_ROYAL_ESCAPE;

    // Ordered by difficulty: classic tactics, then one spell each, then combinations. Every entry is proven by
    // tools/validation/validate.cpp (forced solution in `par`, none faster, each required ability necessary).
    constexpr Challenge challenges[challenges_count] = {
        { "First Blood", "Mate in one", "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1", false,
          { Objective::CHECKMATE_WITHIN, 3, 0 }, 1, NONE, Difficulty::GRANDMASTER,
          "The king's own pawns are its prison.", "d1d8" },
        { "Doubled Towers", "Mate in two", "r5k1/5ppp/8/8/8/8/4RPPP/4R1K1 w - - 0 1", false,
          { Objective::CHECKMATE_WITHIN, 4, 0 }, 2, NONE, Difficulty::GRANDMASTER,
          "Offer the first rook. The second one finishes.", "e2e8" },
        { "The Royal Fork", "Knight fork", "q3k3/8/8/1N6/8/8/8/4K3 w - - 0 1", false,
          { Objective::WIN_MATERIAL, 3, 8 }, 2, NONE, Difficulty::GRANDMASTER,
          "Check the king and attack the queen in one jump.", "b5c7" },
        { "The Pin", "Pinned piece", "4k3/8/5p2/4n3/8/3P4/8/4R1K1 w - - 0 1", false,
          { Objective::WIN_MATERIAL, 3, 2 }, 2, NONE, Difficulty::GRANDMASTER,
          "A pinned knight cannot run from a pawn.", "d3d4" },
        { "Unmasked", "Discovered attack", "4k3/8/8/4B3/8/q7/8/4R1K1 w - - 0 1", false,
          { Objective::WIN_MATERIAL, 3, 8 }, 2, NONE, Difficulty::GRANDMASTER,
          "Move the bishop with check. Aim it at the queen, and cut her path to the king.", "e5d6" },
        { "Shadow Ambush", "Shadow Step", "7k/7p/8/8/8/1N6/1P6/B5RK w - - 0 1 mana=2/0", true,
          { Objective::CHECKMATE_WITHIN, 3, 0 }, 1, SS, Difficulty::GRANDMASTER,
          "Your pawn blocks your own bishop. It cannot move forward... but it can step aside.", "ss:b2a2" },
        { "Phantom Fork", "Phase Shift", "q3k3/3N4/8/8/8/8/8/4K3 w - - 0 1 mana=3/0", true,
          { Objective::WIN_MATERIAL, 3, 8 }, 2, PS, Difficulty::GRANDMASTER,
          "From c7 a knight sees both king and queen.", "ps:d7c7" },
        { "Through the Veil", "Ghost Walk", "7k/7p/8/8/2N5/1PPP4/1B6/6RK w - - 0 1 mana=3/0", true,
          { Objective::CHECKMATE_WITHIN, 3, 0 }, 1, GW, Difficulty::GRANDMASTER,
          "The long diagonal is blocked only by a friend.", "gw:b2d4" },
        { "Fortress Shift", "Castle Swap", "N5k1/5ppp/8/R7/8/8/8/6K1 w - - 0 1 mana=4/0", true,
          { Objective::CHECKMATE_WITHIN, 3, 0 }, 1, CS, Difficulty::GRANDMASTER,
          "The knight sits where the rook wants to be.", "cs:a5a8" },
        { "The Queen's Word", "Royal Command", "5bk1/7p/4PQ2/8/8/8/8/6K1 w - - 0 1 mana=5/0", true,
          { Objective::CHECKMATE_WITHIN, 3, 0 }, 1, RC, Difficulty::GRANDMASTER,
          "A pawn commanded to f7 would attack g8.", "rc:e6f7@f6" },
        { "Crownless Flight", "Royal Escape", "6k1/5ppp/8/8/8/8/5PPP/4r1K1 w - - 0 1 mana=5/0", true,
          { Objective::SURVIVE, 3, 0 }, 3, RE, Difficulty::GRANDMASTER,
          "Survive 3 moves. No square is safe within one step: leap two.", "re:g1g3" },
        { "Twin Spells", "Two spells", "1kB5/RN6/5K2/5p2/8/4B3/8/8 w - - 0 1 mana=6/0", true,
          { Objective::CHECKMATE_WITHIN, 3, 0 }, 2, PS | CS, Difficulty::GRANDMASTER,
          "Castle Swap and Phase Shift: one prepares, one strikes.", "cs:a7b7" },
        { "Veil and Phase", "Two spells", "3k4/1p1N2Q1/8/8/8/4P3/3B4/K7 w - - 0 1 mana=6/0", true,
          { Objective::CHECKMATE_WITHIN, 3, 0 }, 2, PS | GW, Difficulty::GRANDMASTER,
          "A knight through the air, a bishop through the pawn.", "ps:d7d6" },
        { "The Last Gambit", "Mate in three", "6k1/8/r4P2/3P2P1/8/7K/6Q1/8 w - - 0 1 mana=6/0", true,
          { Objective::CHECKMATE_WITHIN, 4, 0 }, 3, SS, Difficulty::GRANDMASTER,
          "Three moves. One of them is a single sideways step.", "ss:g5f5" },
    };

    constexpr GauntletStage gauntlet[gauntlet_stages] = {
        { C::ROWAN, "French Defence, Advance",
          "r1bqkbnr/pp3ppp/2n1p3/2ppP3/3P4/5N2/PPP2PPP/RNBQKB1R w KQkq - 2 5",
          Difficulty::KNIGHT, Personality::POSITIONAL, 100, "The forest has grown since we last met." },
        { C::VEYRA, "King's Gambit Accepted",
          "rnbqkbnr/pppp1p1p/8/6p1/4Pp2/5N2/PPPP2PP/RNBQKB1R w KQkq - 0 4",
          Difficulty::KNIGHT, Personality::AGGRESSIVE, 150, "Fire answers fire. Gambit accepted." },
        { C::YSRA, "Semi-Slav Defence",
          "rnbqkb1r/pp3ppp/2p1pn2/3p4/2PP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - 0 5",
          Difficulty::SORCERER, Personality::DEFENSIVE, 80, "My walls are higher this winter." },
        { C::NOCTIS, "Sicilian Defence, Dragon",
          "rnbqkb1r/pp2pp1p/3p1np1/8/3NP3/2N5/PPP2PPP/R1BQKB1R w KQkq - 0 6",
          Difficulty::SORCERER, Personality::TRICKSTER, 100, "A dragon sleeps on page six. Mind its breath." },
        { C::RAEL, "Two Knights Defence",
          "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4",
          Difficulty::SORCERER, Personality::TACTICAL, 130, "The storm remembers you. It wants a rematch." },
        { C::AURELIA, "The original board", nullptr,
          Difficulty::GRANDMASTER, Personality::MASTER, 100, "Once more, from the very first move." },
    };
}

const GauntletStage& gauntlet_stage(int index)
{
    CL_ASSERT(index >= 0 && index < gauntlet_stages);
    return gauntlet[index];
}

const World& world(int index)
{
    CL_ASSERT(index >= 0 && index < worlds_count);
    return worlds[index];
}

const Encounter& encounter(int index)
{
    CL_ASSERT(index >= 0 && index < campaign_encounters);
    return encounters[index];
}

const char* character_name(Character character)
{
    return characters[int(character)].name;
}

const char* character_title(Character character)
{
    return characters[int(character)].title;
}

const Dialogue& prologue()
{
    return prologue_lines_d;
}

const Dialogue& epilogue()
{
    return epilogue_lines_d;
}

const Challenge& challenge(int index)
{
    CL_ASSERT(index >= 0 && index < challenges_count);
    return challenges[index];
}

}
