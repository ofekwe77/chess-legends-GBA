// CHESS LEGENDS: ARCANE WARS - scenario objectives and encounter / challenge setup.
#include "cl_campaign.h"

namespace cl
{

int material_balance(const Position& pos, Color side)
{
    int balance = 0;

    for(int c = 0; c < 2; ++c)
    {
        int total = 0;

        for(int i = 1; i < pos.pcount[c]; ++i)
        {
            total += piece_value[type_of(pos.board[pos.plist[c][i]])];
        }

        balance += Color(c) == side ? total : -total;
    }

    return balance;
}

void ObjectiveTracker::start(const ObjectiveSpec& spec, Color player, const Position& start)
{
    _spec = spec;
    _player = player;
    _player_moves = 0;
    _start_balance = material_balance(start, player);
    _promoted = false;
}

void ObjectiveTracker::on_action(const ActionInfo& info, const Game& game)
{
    (void) game;

    if(info.mover != _player)
    {
        return;
    }

    ++_player_moves;

    if(info.move.kind() == MK_PROMOTION)
    {
        _promoted = true;
    }
}

ObjectiveStatus ObjectiveTracker::status(const Game& game) const
{
    const Position& pos = game.position();
    const Color enemy = opposite(_player);

    if(game.over())
    {
        const GameResult result = game.result();
        const bool player_won = (result == GameResult::WHITE_WINS && _player == WHITE) ||
                                (result == GameResult::BLACK_WINS && _player == BLACK);
        const bool player_lost = (result == GameResult::WHITE_WINS && _player == BLACK) ||
                                 (result == GameResult::BLACK_WINS && _player == WHITE);

        if(game.reason() == ResultReason::OBJECTIVE_COMPLETE)
        {
            return player_won ? ObjectiveStatus::SUCCESS : ObjectiveStatus::FAILED;
        }

        if(player_lost)
        {
            return ObjectiveStatus::FAILED;
        }

        // A win always completes the scenario.
        if(player_won)
        {
            return ObjectiveStatus::SUCCESS;
        }

        // Drawn games: the objective decides. (Winning material can end the game at once as a dead position, e.g.
        // a knight fork that leaves king and knight against a bare king.)
        switch(_spec.type)
        {
        case Objective::SURVIVE:
            return ObjectiveStatus::SUCCESS;

        case Objective::PROMOTE:
            return _promoted ? ObjectiveStatus::SUCCESS : ObjectiveStatus::FAILED;

        case Objective::CAPTURE_PAWNS:
            return pos.piece_count[make_piece(enemy, PAWN)] == 0 ? ObjectiveStatus::SUCCESS : ObjectiveStatus::FAILED;

        case Objective::WIN_MATERIAL:
            return _player_moves > 0 && material_balance(pos, _player) - _start_balance >= _spec.value * 100 ?
                       ObjectiveStatus::SUCCESS : ObjectiveStatus::FAILED;

        default:
            return ObjectiveStatus::FAILED;
        }
    }

    const bool limit_reached = _spec.limit && _player_moves >= _spec.limit;
    const bool player_to_move = pos.side == _player;

    switch(_spec.type)
    {
    case Objective::WIN:
        return ObjectiveStatus::ONGOING;

    case Objective::CHECKMATE_WITHIN:
        // The last allowed move has been played (and did not mate): failed once the opponent may move.
        return limit_reached && player_to_move ? ObjectiveStatus::FAILED : ObjectiveStatus::ONGOING;

    case Objective::SURVIVE:
        return limit_reached && player_to_move ? ObjectiveStatus::SUCCESS : ObjectiveStatus::ONGOING;

    case Objective::PROMOTE:
        if(_promoted)
        {
            return ObjectiveStatus::SUCCESS;
        }

        return limit_reached ? ObjectiveStatus::FAILED : ObjectiveStatus::ONGOING;

    case Objective::WIN_MATERIAL:
        // Measured when it is the player's turn again, so an immediate recapture is taken into account.
        if(player_to_move && _player_moves > 0 &&
           material_balance(pos, _player) - _start_balance >= _spec.value * 100)
        {
            return ObjectiveStatus::SUCCESS;
        }

        return limit_reached && player_to_move ? ObjectiveStatus::FAILED : ObjectiveStatus::ONGOING;

    case Objective::CAPTURE_PAWNS:
        if(pos.piece_count[make_piece(enemy, PAWN)] == 0)
        {
            return ObjectiveStatus::SUCCESS;
        }

        return limit_reached && player_to_move ? ObjectiveStatus::FAILED : ObjectiveStatus::ONGOING;

    default:
        return ObjectiveStatus::ONGOING;
    }
}

uint32_t ObjectiveTracker::pack() const
{
    return uint32_t(_player_moves & 0x3FF) | (uint32_t(_promoted) << 10) |
           (uint32_t(uint16_t(int16_t(_start_balance))) << 16);
}

void ObjectiveTracker::unpack(uint32_t state)
{
    _player_moves = int(state & 0x3FF);
    _promoted = (state >> 10) & 1;
    _start_balance = int16_t(uint16_t(state >> 16));
}

RuleSet encounter_rules(const Encounter& encounter)
{
    RuleSet rules = RuleSet::arcane_rules();

    if(encounter.max_mana)
    {
        rules.max_mana = encounter.max_mana;
    }

    rules.ability_mask[encounter.player] = encounter.player_abilities;
    rules.ability_mask[opposite(encounter.player)] = encounter.enemy_abilities;
    return rules;
}

bool setup_encounter(const Encounter& encounter, Game& game)
{
    const RuleSet rules = encounter_rules(encounter);

    if(! encounter.fen)
    {
        game.start(rules);
        return true;
    }

    // Scenario positions state their mana explicitly; when they do not, both sides start from zero and the side to
    // move receives its turn-start regeneration, exactly like the standard start.
    bool explicit_mana = false;

    for(const char* c = encounter.fen; *c; ++c)
    {
        if(c[0] == 'm' && c[1] == 'a' && c[2] == 'n' && c[3] == 'a' && c[4] == '=')
        {
            explicit_mana = true;
            break;
        }
    }

    return game.setup(encounter.fen, rules, ! explicit_mana) == FenError::NONE;
}

bool setup_challenge(const Challenge& challenge, Game& game)
{
    const RuleSet rules = challenge.arcane ? RuleSet::arcane_rules() : RuleSet::classic();
    return game.setup(challenge.fen, rules, false) == FenError::NONE;
}

int challenge_medal(const Challenge& challenge, int player_moves)
{
    const int best = challenge.par;

    if(player_moves <= best)
    {
        return 3;
    }

    return player_moves <= best + 1 ? 2 : 1;
}

}
