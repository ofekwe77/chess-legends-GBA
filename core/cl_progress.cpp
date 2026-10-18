// CHESS LEGENDS: ARCANE WARS - achievements, statistics, gallery and cosmetic unlocks.
#include "cl_progress.h"

namespace cl
{

namespace
{
    using AC = AchievementCategory;

    constexpr AchievementInfo infos[achievements_count] = {
        { "first_victory", "First Victory", "Win a match against the computer.", AC::BEGINNER, 0 },
        { "apt_pupil", "Apt Pupil", "Complete all ten tutorial lessons.", AC::BEGINNER, 1 },
        { "first_check", "Check!", "Give check for the first time.", AC::BEGINNER, 2 },
        { "queensbane", "Queensbane", "Capture an enemy queen.", AC::BEGINNER, 3 },
        { "classicist", "Classicist", "Win a Classic Chess match against the computer.", AC::CLASSIC, 4 },
        { "checkmate_artist", "Checkmate Artist", "Win a match against the computer by checkmate.", AC::CLASSIC, 5 },
        { "coronation", "Coronation", "Promote a pawn.", AC::CLASSIC, 6 },
        { "safe_harbor", "Safe Harbor", "Castle your king.", AC::CLASSIC, 7 },
        { "in_passing", "In Passing", "Capture a pawn en passant.", AC::CLASSIC, 8 },
        { "first_spark", "First Spark", "Cast your first ability.", AC::ARCANE, 9 },
        { "shadow_stepper", "Shadow Stepper", "Win a match in which you cast Shadow Step.", AC::ARCANE, 10 },
        { "phase_walker", "Phase Walker", "Win a match in which you cast Phase Shift.", AC::ARCANE, 11 },
        { "veil_walker", "Veil Walker", "Win a match in which you cast Ghost Walk.", AC::ARCANE, 12 },
        { "moving_walls", "Moving Walls", "Win a match in which you cast Castle Swap.", AC::ARCANE, 13 },
        { "royal_decree", "Royal Decree", "Win a match in which you cast Royal Command.", AC::ARCANE, 14 },
        { "great_escape", "Great Escape", "Win a match in which you cast Royal Escape.", AC::ARCANE, 15 },
        { "giant_slayer", "Giant Slayer", "Defeat a Sorcerer or Grandmaster computer opponent.", AC::STRATEGY, 16 },
        { "grandmaster_slayer", "Grandmaster Slayer", "Defeat a Grandmaster computer opponent.", AC::STRATEGY, 17 },
        { "mana_miser", "Mana Miser", "Win an Arcane match while holding full mana.", AC::STRATEGY, 18 },
        { "spellweaver", "Spellweaver", "Win a match after casting four different abilities.", AC::STRATEGY, 19 },
        { "unshaken", "Unshaken", "Defeat a boss without casting Royal Escape.", AC::STRATEGY, 20 },
        { "double_trouble", "Double Trouble", "Give a double check.", AC::STRATEGY, 21 },
        { "flawless", "Flawless", "Win a match without losing a single piece or pawn.", AC::STRATEGY, 22 },
        { "marathon", "Marathon", "Finish a match of at least 80 moves.", AC::STRATEGY, 23 },
        { "wardens_fall", "Warden's Fall", "Defeat Rowan Thorn, ruler of the Verdant March.", AC::CAMPAIGN, 24 },
        { "three_crowns", "Three Crowns", "Defeat the rulers of three kingdoms.", AC::CAMPAIGN, 25 },
        { "crown_restored", "Crown Restored", "Defeat Aurelia Voss and restore the Crown of Six.", AC::CAMPAIGN, 26 },
        { "puzzle_solver", "Puzzle Solver", "Solve a challenge.", AC::MODES, 27 },
        { "grand_tactician", "Grand Tactician", "Earn a gold medal in every challenge.", AC::MODES, 28 },
        { "gauntlet_champion", "Gauntlet Champion", "Defeat all six rulers in one Boss Gauntlet run.", AC::MODES, 29 },
        { "friendly_rivals", "Friendly Rivals", "Finish a local two-player match.", AC::MODES, 30 },
        { "lorekeeper", "Lorekeeper", "Unlock every entry in the gallery.", AC::MODES, 31 },
    };

    constexpr Achievement spell_achievements[ABILITIES_COUNT] = {
        Achievement::SHADOW_STEPPER, Achievement::PHASE_WALKER, Achievement::VEIL_WALKER,
        Achievement::MOVING_WALLS, Achievement::ROYAL_DECREE, Achievement::GREAT_ESCAPE
    };

    constexpr int boss_encounter(int world)
    {
        return world * 3 + 2;
    }

    int popcount(uint32_t value)
    {
        int count = 0;

        while(value)
        {
            value &= value - 1;
            ++count;
        }

        return count;
    }

    uint8_t saturate8(int value)
    {
        return uint8_t(value > 255 ? 255 : value);
    }

    uint16_t saturate16(int value)
    {
        return uint16_t(value > 65535 ? 65535 : value);
    }

    // ---- lore ----

    constexpr const char* character_lores[gallery_characters] = {
        "A young tactician of the old royal line, raised on the stories of the Crown of Six. Arden plays patiently "
        "and learns from every defeat.",
        "Keeper of the Board and Arden's teacher. Orrin has watched every Arcane War since the crown broke, and "
        "remembers more than he tells.",
        "Ruler of the Verdant March. Rowan believes balance is earned, never taken, and defends her forest one "
        "patient move at a time.",
        "Ruler of the Inferno Bastion. Veyra attacks before her enemies can breathe and trusts only what can "
        "survive her fire.",
        "Empress of the Frozen Court. Ysra builds walls of ice and waits; her mirrors are said to remember every "
        "move ever played.",
        "Scholar of the Umbral Archive. Noctis hides a second purpose in every piece and reads opponents like "
        "forbidden pages.",
        "Sovereign of the Storm Spire. Rael never stops moving, and believes lightning always strikes the tallest "
        "tower first.",
        "The Last Sovereign of the Crown Nexus. Aurelia understands every spell of the board, and every reason the "
        "crown had to break.",
        "Mossguard of the Verdant March. Brin's roots hold the forest's pawns in place, and nothing passes them "
        "without a little magic.",
        "Ember Knight of the Bastion. Harrik guards the Iron Gate and admires speed above all other virtues.",
        "Mirror Warden of the Frozen Court. Elsk spends mana like winter spends daylight: slowly and without mercy.",
        "Keeper of Forbidden Pages. Quill hides pawns in labyrinths of shelves and loves a fork from nowhere.",
        "Sky Engineer of the Storm Spire. Tamsin keeps every piece moving, and chains spells like lightning.",
        "Herald of Dawn. Seraph guards the gate of the Nexus, and speaks only in riddles of the old magic.",
    };

    constexpr const char* kingdom_lores[gallery_kingdoms] = {
        "Ancient forests and a sleeping temple. Its pieces are rooted and patient, and its board grows living moss.",
        "Black iron walls over rivers of lava. Its armies march at dawn and never retreat before dusk.",
        "Crystal halls where every mirror remembers. Its rulers win by waiting longer than their enemies.",
        "Endless shelves lost in shadow. Its scholars hide a trap behind every quiet move.",
        "A citadel of floating stone in endless thunder. Its pieces are never still.",
        "The ruined first capital, between light and darkness, beneath the floating fragments of the crown.",
    };

    constexpr const char* music_titles[gallery_music] = {
        "Crown of Six", "The Six Roads", "Emerald Canopy", "Iron and Ember", "Mirror of Winter",
        "Forbidden Pages", "Thunder Over Stone", "The Floating Crown", "Arcane Gambit", "The Ruler's Gambit",
        "Checkmate Fanfare", "The Fallen King", "Six Hands, One Crown",
    };

    constexpr const char* character_hints[gallery_characters] = {
        "", "",
        "Defeat Rowan Thorn in the story.", "Defeat Veyra Ember in the story.", "Defeat Ysra Veil in the story.",
        "Defeat Noctis Vane in the story.", "Defeat Rael Tempest in the story.", "Defeat Aurelia Voss in the story.",
        "Clear Brin's trial in the story.", "Clear one of Harrik's trials.", "Clear one of Elsk's trials.",
        "Clear one of Quill's trials.", "Clear one of Tamsin's trials.", "Clear one of Seraph's trials.",
    };

    constexpr const char* kingdom_hints[gallery_kingdoms] = {
        "", "Reach World 2 in the story.", "Reach World 3 in the story.", "Reach World 4 in the story.",
        "Reach World 5 in the story.", "Reach World 6 in the story.",
    };

    constexpr const char* spell_hints[gallery_spells] = {
        "Cast Shadow Step in any match.", "Cast Phase Shift in any match.", "Cast Ghost Walk in any match.",
        "Cast Castle Swap in any match.", "Cast Royal Command in any match.", "Cast Royal Escape in any match.",
    };
}

static_assert(MatchRecorder::packed_size == sizeof(SuspendedMatch::recorder), "suspend record layout");

const AchievementInfo& achievement_info(Achievement achievement)
{
    CL_ASSERT(int(achievement) < achievements_count);
    return infos[int(achievement)];
}

// ------------------------------------------------------------------------------------------------ MatchRecorder

void MatchRecorder::start(const MatchContext& context)
{
    *this = MatchRecorder();
    _context = context;
}

void MatchRecorder::on_action(const ActionInfo& info, const Game& game)
{
    ++_actions;

    if(info.captured != EMPTY && person(color_of(info.captured)))
    {
        _person_lost_material = true;
    }

    if(! person(info.mover))
    {
        return;
    }

    const MoveKind kind = info.move.kind();

    if(info.captured != EMPTY)
    {
        ++_captures;

        if(type_of(info.captured) == QUEEN)
        {
            ++_queens;
            _earned |= achievement_bit(Achievement::QUEENSBANE);
        }
    }

    if(info.check)
    {
        ++_checks;
        _earned |= achievement_bit(Achievement::FIRST_CHECK);
        const Position& pos = game.position();
        const Color enemy = opposite(info.mover);

        if(pos.attackers_count(pos.king_square(enemy), info.mover, 2) >= 2)
        {
            _earned |= achievement_bit(Achievement::DOUBLE_TROUBLE);
        }
    }

    switch(kind)
    {
    case MK_PROMOTION:
        ++_promotions;
        _earned |= achievement_bit(Achievement::CORONATION);
        break;

    case MK_CASTLE_KING:
    case MK_CASTLE_QUEEN:
        ++_castles;
        _earned |= achievement_bit(Achievement::SAFE_HARBOR);
        break;

    case MK_EN_PASSANT:
        ++_en_passants;
        _earned |= achievement_bit(Achievement::IN_PASSING);
        break;

    default:
        break;
    }

    if(info.move.is_magic())
    {
        const int ability = int(info.move.ability());
        _spells_cast |= uint8_t(1 << ability);
        _spell_counts[ability] = saturate8(_spell_counts[ability] + 1);
        _earned |= achievement_bit(Achievement::FIRST_SPARK);
    }
}

void MatchRecorder::on_finish(const Game& game, bool objective_success)
{
    if(_finished)
    {
        return;
    }

    // Contract: the result is final (scenarios call Game::finish_objective first).
    CL_ASSERT(game.over());
    _finished = true;

    if(_context.human_mask == 0)
    {
        return;     // computer against computer: nobody to credit
    }

    const bool two_players = _context.human_mask == 3;
    const bool counts_as_match = _context.kind != MatchKind::CHALLENGE && _context.kind != MatchKind::TUTORIAL;
    const GameResult result = game.result();

    if(two_players || ! counts_as_match)
    {
        if(two_players && counts_as_match)
        {
            _earned |= achievement_bit(Achievement::FRIENDLY_RIVALS);
        }

        if(counts_as_match && _actions >= 160)
        {
            _earned |= achievement_bit(Achievement::MARATHON);
        }

        return;
    }

    const Color person_color = person(WHITE) ? WHITE : BLACK;
    const bool scenario = _context.kind == MatchKind::STORY || _context.kind == MatchKind::GAUNTLET;
    const bool result_win = (result == GameResult::WHITE_WINS && person_color == WHITE) ||
                            (result == GameResult::BLACK_WINS && person_color == BLACK);
    _person_won = scenario ? objective_success : result_win;
    _person_lost = scenario ? ! objective_success :
                              ((result == GameResult::WHITE_WINS && person_color == BLACK) ||
                               (result == GameResult::BLACK_WINS && person_color == WHITE));
    _checkmate = result_win && game.reason() == ResultReason::CHECKMATE;

    if(_actions >= 160)
    {
        _earned |= achievement_bit(Achievement::MARATHON);
    }

    if(! _person_won)
    {
        return;
    }

    _earned |= achievement_bit(Achievement::FIRST_VICTORY);

    if(_context.kind == MatchKind::CLASSIC)
    {
        _earned |= achievement_bit(Achievement::CLASSICIST);
    }

    if(_checkmate)
    {
        _earned |= achievement_bit(Achievement::CHECKMATE_ARTIST);
    }

    for(int a = 0; a < ABILITIES_COUNT; ++a)
    {
        if(_spells_cast & (1 << a))
        {
            _earned |= achievement_bit(spell_achievements[a]);
        }
    }

    if(_context.difficulty >= Difficulty::SORCERER)
    {
        _earned |= achievement_bit(Achievement::GIANT_SLAYER);
    }

    if(_context.difficulty == Difficulty::GRANDMASTER)
    {
        _earned |= achievement_bit(Achievement::GRANDMASTER_SLAYER);
    }

    const Position& pos = game.position();

    if(pos.rules.arcane && pos.mana[person_color] >= pos.rules.max_mana)
    {
        _earned |= achievement_bit(Achievement::MANA_MISER);
    }

    if(popcount(_spells_cast) >= 4)
    {
        _earned |= achievement_bit(Achievement::SPELLWEAVER);
    }

    if(_context.boss && ! (_spells_cast & (1 << ABILITY_ROYAL_ESCAPE)))
    {
        _earned |= achievement_bit(Achievement::UNSHAKEN);
    }

    if(! _person_lost_material)
    {
        _earned |= achievement_bit(Achievement::FLAWLESS);
    }
}

uint64_t MatchRecorder::commit(Profile& profile) const
{
    SaveStats& stats = profile.stats;
    stats.captures += _captures;
    stats.checks += _checks;
    stats.promotions += _promotions;
    stats.castles += _castles;
    stats.en_passants += _en_passants;
    stats.queens_captured += _queens;

    for(int a = 0; a < ABILITIES_COUNT; ++a)
    {
        stats.spells[a] += _spell_counts[a];
    }

    const bool counts_as_match = _context.kind != MatchKind::CHALLENGE && _context.kind != MatchKind::TUTORIAL;

    if(_finished && counts_as_match)
    {
        ++stats.games_played;

        if(_context.human_mask != 3)
        {
            if(_person_won)
            {
                ++stats.games_won;
                stats.win_streak = saturate16(stats.win_streak + 1);
                stats.best_streak = stats.win_streak > stats.best_streak ? stats.win_streak : stats.best_streak;
                const uint8_t level = uint8_t(int(_context.difficulty) + 1);
                stats.best_difficulty_beaten = level > stats.best_difficulty_beaten ? level :
                                                                                      stats.best_difficulty_beaten;
            }
            else
            {
                stats.win_streak = 0;
                (_person_lost ? stats.games_lost : stats.games_drawn) += 1;
            }

            if(_checkmate)
            {
                ++stats.checkmates;
            }
        }

        // A local two-player match only counts as played: won / lost / drawn describe games against the computer.
    }

    uint64_t unlocked = unlock_achievements(profile.progress, _earned);
    unlocked |= unlock_achievements(profile.progress, progress_achievements(profile));
    return unlocked;
}

void MatchRecorder::pack(uint8_t* out) const
{
    for(int i = 0; i < packed_size; ++i)
    {
        out[i] = 0;
    }

    out[0] = uint8_t(_actions);
    out[1] = uint8_t(_actions >> 8);
    out[2] = _spells_cast;
    out[3] = uint8_t(_person_lost_material);
    out[4] = uint8_t(_captures);
    out[5] = uint8_t(_captures >> 8);
    out[6] = uint8_t(_checks);
    out[7] = uint8_t(_checks >> 8);
    out[8] = _promotions;
    out[9] = _castles;
    out[10] = _en_passants;
    out[11] = _queens;

    for(int a = 0; a < ABILITIES_COUNT; ++a)
    {
        out[12 + a] = _spell_counts[a];
    }
}

bool MatchRecorder::unpack(const uint8_t* in)
{
    if(in[3] > 1 || in[2] >> ABILITIES_COUNT)
    {
        return false;
    }

    _actions = uint16_t(in[0] | (in[1] << 8));
    _spells_cast = in[2];
    _person_lost_material = in[3];
    _captures = uint16_t(in[4] | (in[5] << 8));
    _checks = uint16_t(in[6] | (in[7] << 8));
    _promotions = in[8];
    _castles = in[9];
    _en_passants = in[10];
    _queens = in[11];

    for(int a = 0; a < ABILITIES_COUNT; ++a)
    {
        _spell_counts[a] = in[12 + a];
    }

    _finished = false;
    _earned = 0;
    return true;
}

// ------------------------------------------------------------------------------------------------ progress

uint64_t progress_achievements(const Profile& profile)
{
    const SaveProgress& progress = profile.progress;
    uint64_t earned = 0;

    if(tutorial_complete(progress))
    {
        earned |= achievement_bit(Achievement::APT_PUPIL);
    }

    if(encounter_cleared(progress, boss_encounter(0)))
    {
        earned |= achievement_bit(Achievement::WARDENS_FALL);
    }

    if(bosses_defeated(progress) >= 3)
    {
        earned |= achievement_bit(Achievement::THREE_CROWNS);
    }

    if(encounter_cleared(progress, boss_encounter(worlds_count - 1)))
    {
        earned |= achievement_bit(Achievement::CROWN_RESTORED);
    }

    bool any_medal = false;
    bool all_gold = true;

    for(int c = 0; c < challenges_count; ++c)
    {
        const int medal = challenge_medal_of(progress, c);
        any_medal = any_medal || medal > 0;
        all_gold = all_gold && medal == 3;
    }

    if(any_medal)
    {
        earned |= achievement_bit(Achievement::PUZZLE_SOLVER);
    }

    if(all_gold)
    {
        earned |= achievement_bit(Achievement::GRAND_TACTICIAN);
    }

    if(progress.gauntlet_cleared)
    {
        earned |= achievement_bit(Achievement::GAUNTLET_CHAMPION);
    }

    if(gallery_unlocked_count(profile) == gallery_entries)
    {
        earned |= achievement_bit(Achievement::LOREKEEPER);
    }

    return earned;
}

uint64_t unlock_achievements(SaveProgress& progress, uint64_t earned)
{
    const uint64_t valid = achievements_count == 64 ? ~uint64_t(0) : (uint64_t(1) << achievements_count) - 1;
    const uint64_t fresh = earned & valid & ~progress.achievements;
    progress.achievements |= fresh;
    return fresh;
}

bool encounter_cleared(const SaveProgress& progress, int encounter)
{
    return encounter >= 0 && encounter < campaign_encounters && ((progress.encounters_cleared >> encounter) & 1);
}

int bosses_defeated(const SaveProgress& progress)
{
    int count = 0;

    for(int w = 0; w < worlds_count; ++w)
    {
        count += encounter_cleared(progress, boss_encounter(w));
    }

    return count;
}

bool tutorial_complete(const SaveProgress& progress)
{
    return progress.tutorial_lessons == (1u << tutorial_lessons_count) - 1;
}

bool encounter_unlocked(const SaveProgress& progress, int encounter)
{
    if(encounter < 0 || encounter >= campaign_encounters)
    {
        return false;
    }

    const uint32_t earlier = (1u << encounter) - 1;
    return (progress.encounters_cleared & earlier) == earlier;
}

void record_encounter_cleared(SaveProgress& progress, int encounter, int player_moves)
{
    CL_ASSERT(encounter >= 0 && encounter < campaign_encounters);
    const uint8_t moves = saturate8(player_moves < 1 ? 1 : player_moves);
    uint8_t& best = progress.encounter_best[encounter];
    best = best == 0 || moves < best ? moves : best;
    progress.encounters_cleared |= 1u << encounter;

    // The unlocked world is the world of the first encounter not cleared yet.
    int first_open = 0;

    while(first_open < campaign_encounters && encounter_cleared(progress, first_open))
    {
        ++first_open;
    }

    const int world_index = first_open >= campaign_encounters ? worlds_count - 1 : first_open / 3;
    progress.story_world = uint8_t(world_index > progress.story_world ? world_index : progress.story_world);
}

int challenge_medal_of(const SaveProgress& progress, int challenge)
{
    return int((progress.challenge_medals >> (challenge * 2)) & 3);
}

void record_challenge_medal(SaveProgress& progress, int challenge, int medal)
{
    CL_ASSERT(challenge >= 0 && challenge < challenges_count && challenge < challenges_max);
    medal = medal < 0 ? 0 : (medal > 3 ? 3 : medal);

    if(medal > challenge_medal_of(progress, challenge))
    {
        progress.challenge_medals &= ~(3u << (challenge * 2));
        progress.challenge_medals |= uint32_t(medal) << (challenge * 2);
    }
}

bool board_theme_unlocked(const SaveProgress& progress, int theme)
{
    if(theme == 0 || theme == board_themes_count - 1)
    {
        return true;
    }

    return theme > 0 && theme < board_themes_count - 1 && encounter_cleared(progress, boss_encounter(theme - 1));
}

// ------------------------------------------------------------------------------------------------ gallery

int gallery_section_size(GallerySection section)
{
    switch(section)
    {
    case GallerySection::CHARACTERS: return gallery_characters;
    case GallerySection::KINGDOMS: return gallery_kingdoms;
    case GallerySection::MUSIC: return gallery_music;
    case GallerySection::SPELLS: return gallery_spells;
    default: return 0;
    }
}

bool gallery_unlocked(const Profile& profile, GallerySection section, int index)
{
    const SaveProgress& progress = profile.progress;

    switch(section)
    {
    case GallerySection::CHARACTERS:
    {
        const Character character = Character(index);

        if(character == Character::ARDEN || character == Character::ORRIN)
        {
            return true;
        }

        for(int e = 0; e < campaign_encounters; ++e)
        {
            if(encounter(e).opponent == character && encounter_cleared(progress, e))
            {
                return true;
            }
        }

        return false;
    }

    case GallerySection::KINGDOMS:
        return index <= progress.story_world;

    case GallerySection::MUSIC:
        return (progress.music_unlocked >> index) & 1;

    case GallerySection::SPELLS:
        return profile.stats.spells[index] > 0;

    default:
        return false;
    }
}

int gallery_unlocked_count(const Profile& profile)
{
    int count = 0;

    for(int s = 0; s < 4; ++s)
    {
        const GallerySection section = GallerySection(s);

        for(int i = 0; i < gallery_section_size(section); ++i)
        {
            count += gallery_unlocked(profile, section, i);
        }
    }

    return count;
}

const char* character_lore(Character character)
{
    CL_ASSERT(int(character) < gallery_characters);
    return character_lores[int(character)];
}

const char* kingdom_lore(int world)
{
    CL_ASSERT(world >= 0 && world < gallery_kingdoms);
    return kingdom_lores[world];
}

const char* gallery_unlock_hint(GallerySection section, int index)
{
    switch(section)
    {
    case GallerySection::CHARACTERS: return character_hints[index];
    case GallerySection::KINGDOMS: return kingdom_hints[index];
    case GallerySection::MUSIC: return "Hear this track while playing.";
    case GallerySection::SPELLS: return spell_hints[index];
    default: return "";
    }
}

const char* music_title(int track)
{
    CL_ASSERT(track >= 0 && track < gallery_music);
    return music_titles[track];
}

}
