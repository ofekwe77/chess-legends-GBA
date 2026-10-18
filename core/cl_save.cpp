// CHESS LEGENDS: ARCANE WARS - persistent save data.
#include "cl_save.h"

namespace cl
{

namespace
{
    class ByteWriter
    {
    public:
        ByteWriter(uint8_t* buffer, int capacity) :
            _buffer(buffer),
            _capacity(capacity)
        {
        }

        void u8(uint32_t v)
        {
            if(_size < _capacity)
            {
                _buffer[_size] = uint8_t(v);
            }

            ++_size;
        }

        void u16(uint32_t v)
        {
            u8(v);
            u8(v >> 8);
        }

        void u32(uint32_t v)
        {
            u16(v);
            u16(v >> 16);
        }

        void u64(uint64_t v)
        {
            u32(uint32_t(v));
            u32(uint32_t(v >> 32));
        }

        [[nodiscard]] int size() const
        {
            return _size;
        }

        [[nodiscard]] bool overflow() const
        {
            return _size > _capacity;
        }

        void patch_u16(int offset, uint32_t v)
        {
            _buffer[offset] = uint8_t(v);
            _buffer[offset + 1] = uint8_t(v >> 8);
        }

        void patch_u32(int offset, uint32_t v)
        {
            patch_u16(offset, v);
            patch_u16(offset + 2, v >> 16);
        }

        [[nodiscard]] uint8_t* data()
        {
            return _buffer;
        }

    private:
        uint8_t* _buffer;
        int _capacity;
        int _size = 0;
    };

    class ByteReader
    {
    public:
        ByteReader(const uint8_t* buffer, int size) :
            _buffer(buffer),
            _size(size)
        {
        }

        uint32_t u8()
        {
            if(_position >= _size)
            {
                _failed = true;
                return 0;
            }

            return _buffer[_position++];
        }

        uint32_t u16()
        {
            uint32_t lo = u8();
            return lo | (u8() << 8);
        }

        uint32_t u32()
        {
            uint32_t lo = u16();
            return lo | (u16() << 16);
        }

        uint64_t u64()
        {
            uint64_t lo = u32();
            return lo | (uint64_t(u32()) << 32);
        }

        [[nodiscard]] bool failed() const
        {
            return _failed;
        }

        [[nodiscard]] int position() const
        {
            return _position;
        }

    private:
        const uint8_t* _buffer;
        int _size;
        int _position = 0;
        bool _failed = false;
    };

    struct CrcTable
    {
        uint32_t entries[256] = {};
    };

    constexpr CrcTable build_crc_table()
    {
        CrcTable table;

        for(uint32_t n = 0; n < 256; ++n)
        {
            uint32_t c = n;

            for(int k = 0; k < 8; ++k)
            {
                c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            }

            table.entries[n] = c;
        }

        return table;
    }

    constexpr CrcTable crc_table = build_crc_table();

    // Header: signature u32, version u16, kind u8, reserved u8, sequence u32, payload length u16, reserved u16,
    // crc u32. The CRC covers the first 16 header bytes AND the payload: a torn write that updated the sequence
    // number but not the payload must never look like a newer valid record.
    uint32_t record_crc(const uint8_t* record, int payload)
    {
        uint32_t crc = crc32(record, 16);
        return crc32(record + header_size, payload, crc);
    }

    void begin_record(ByteWriter& w, RecordKind kind, uint32_t sequence)
    {
        w.u32(save_signature);
        w.u16(save_version);
        w.u8(uint32_t(kind));
        w.u8(0);
        w.u32(sequence);
        w.u16(0);   // payload length, patched later
        w.u16(0);
        w.u32(0);   // crc, patched later
    }

    int finish_record(ByteWriter& w)
    {
        if(w.overflow())
        {
            return 0;
        }

        const int payload = w.size() - header_size;
        w.patch_u16(12, uint32_t(payload));
        w.patch_u32(16, record_crc(w.data(), payload));
        return w.size();
    }

    RecordStatus check_header(ByteReader& r, const uint8_t* buffer, int size, RecordKind kind, uint32_t& sequence,
                              int& payload)
    {
        const uint32_t signature = r.u32();
        const uint32_t version = r.u16();
        const uint32_t record_kind = r.u8();
        r.u8();
        sequence = r.u32();
        payload = int(r.u16());
        r.u16();
        const uint32_t crc = r.u32();

        if(signature != save_signature)
        {
            return RecordStatus::BLANK;
        }

        if(version > save_version)
        {
            return RecordStatus::UNSUPPORTED;
        }

        if(version == 0 || record_kind != uint32_t(kind) || payload <= 0 || header_size + payload > size)
        {
            return RecordStatus::CORRUPTED;
        }

        if(record_crc(buffer, payload) != crc)
        {
            return RecordStatus::CORRUPTED;
        }

        return RecordStatus::VALID;
    }

    void write_settings(ByteWriter& w, const SaveSettings& s)
    {
        w.u8(s.music_volume);
        w.u8(s.sfx_volume);
        w.u8(uint32_t(s.music_on) | (uint32_t(s.sfx_on) << 1) | (uint32_t(s.high_contrast) << 2) |
             (uint32_t(s.reduced_effects) << 3) | (uint32_t(s.show_coordinates) << 4));
        w.u8(s.animations);
        w.u8(s.text_speed);
        w.u8(s.board_theme);
        w.u8(s.preferred_difficulty);
    }

    bool read_settings(ByteReader& r, SaveSettings& s)
    {
        s.music_volume = uint8_t(r.u8());
        s.sfx_volume = uint8_t(r.u8());
        uint32_t flags = r.u8();
        s.music_on = flags & 1;
        s.sfx_on = (flags >> 1) & 1;
        s.high_contrast = (flags >> 2) & 1;
        s.reduced_effects = (flags >> 3) & 1;
        s.show_coordinates = (flags >> 4) & 1;
        s.animations = uint8_t(r.u8());
        s.text_speed = uint8_t(r.u8());
        s.board_theme = uint8_t(r.u8());
        s.preferred_difficulty = uint8_t(r.u8());

        // Range validation: corrupted values never reach the game.
        return s.music_volume <= 10 && s.sfx_volume <= 10 && s.animations <= 3 && s.text_speed <= 3 &&
               s.board_theme < 8 && s.preferred_difficulty < 4;
    }

    void write_progress(ByteWriter& w, const SaveProgress& p)
    {
        w.u32(p.encounters_cleared);

        for(uint8_t best : p.encounter_best)
        {
            w.u8(best);
        }

        w.u64(p.achievements);
        w.u32(p.challenge_medals);
        w.u16(p.music_unlocked);
        w.u16(p.tutorial_lessons);
        w.u8(p.gauntlet_best);
        w.u8(uint32_t(p.gauntlet_cleared) | (uint32_t(p.intro_seen) << 1) | (uint32_t(p.ending_seen) << 2));
        w.u8(p.story_world);
        w.u8(p.worlds_seen);
    }

    bool read_progress(ByteReader& r, SaveProgress& p)
    {
        p.encounters_cleared = r.u32();

        for(uint8_t& best : p.encounter_best)
        {
            best = uint8_t(r.u8());
        }

        p.achievements = r.u64();
        p.challenge_medals = r.u32();
        p.music_unlocked = uint16_t(r.u16());
        p.tutorial_lessons = uint16_t(r.u16());
        p.gauntlet_best = uint8_t(r.u8());
        uint32_t flags = r.u8();
        p.gauntlet_cleared = flags & 1;
        p.intro_seen = (flags >> 1) & 1;
        p.ending_seen = (flags >> 2) & 1;
        p.story_world = uint8_t(r.u8());
        p.worlds_seen = uint8_t(r.u8());

        // Range validation: corrupted values never reach the game.
        if(p.encounters_cleared >> encounters_count || p.gauntlet_best > 6 || p.story_world > 5 || p.worlds_seen >> 6 ||
           p.music_unlocked >> music_tracks_count || p.tutorial_lessons >> tutorial_lessons_count || flags >> 3)
        {
            return false;
        }

        // A best move count exists exactly for the cleared encounters.
        for(int i = 0; i < encounters_count; ++i)
        {
            if((p.encounter_best[i] != 0) != bool((p.encounters_cleared >> i) & 1))
            {
                return false;
            }
        }

        return true;
    }

    void write_stats(ByteWriter& w, const SaveStats& s)
    {
        w.u32(s.games_played);
        w.u32(s.games_won);
        w.u32(s.games_lost);
        w.u32(s.games_drawn);
        w.u32(s.checkmates);
        w.u32(s.captures);
        w.u32(s.checks);
        w.u32(s.promotions);
        w.u32(s.castles);
        w.u32(s.en_passants);
        w.u32(s.queens_captured);

        for(uint32_t spell : s.spells)
        {
            w.u32(spell);
        }

        w.u32(s.play_seconds);
        w.u16(s.win_streak);
        w.u16(s.best_streak);
        w.u8(s.best_difficulty_beaten);
    }

    bool read_stats(ByteReader& r, SaveStats& s)
    {
        s.games_played = r.u32();
        s.games_won = r.u32();
        s.games_lost = r.u32();
        s.games_drawn = r.u32();
        s.checkmates = r.u32();
        s.captures = r.u32();
        s.checks = r.u32();
        s.promotions = r.u32();
        s.castles = r.u32();
        s.en_passants = r.u32();
        s.queens_captured = r.u32();

        for(uint32_t& spell : s.spells)
        {
            spell = r.u32();
        }

        s.play_seconds = r.u32();
        s.win_streak = uint16_t(r.u16());
        s.best_streak = uint16_t(r.u16());
        s.best_difficulty_beaten = uint8_t(r.u8());
        return s.best_difficulty_beaten <= 4;
    }

    constexpr char fen_piece_chars[16] = { 0, 'P', 'N', 'B', 'R', 'Q', 'K', 0, 0, 'p', 'n', 'b', 'r', 'q', 'k', 0 };
}

uint32_t crc32(const uint8_t* data, int size, uint32_t crc)
{
    uint32_t c = crc ^ 0xFFFFFFFFu;

    for(int i = 0; i < size; ++i)
    {
        c = crc_table.entries[(c ^ data[i]) & 0xFF] ^ (c >> 8);
    }

    return c ^ 0xFFFFFFFFu;
}

int write_profile_record(const Profile& profile, uint32_t sequence, uint8_t* buffer, int capacity)
{
    ByteWriter w(buffer, capacity);
    begin_record(w, RecordKind::PROFILE, sequence);
    write_settings(w, profile.settings);
    write_progress(w, profile.progress);
    write_stats(w, profile.stats);
    return finish_record(w);
}

RecordStatus read_profile_record(const uint8_t* buffer, int size, Profile& profile, uint32_t& sequence)
{
    ByteReader r(buffer, size);
    int payload = 0;
    RecordStatus status = check_header(r, buffer, size, RecordKind::PROFILE, sequence, payload);

    if(status != RecordStatus::VALID)
    {
        return status;
    }

    Profile result;
    bool ok = read_settings(r, result.settings) && read_progress(r, result.progress) && read_stats(r, result.stats);

    if(! ok || r.failed() || r.position() != header_size + payload)
    {
        return RecordStatus::CORRUPTED;
    }

    profile = result;
    return RecordStatus::VALID;
}

int write_position(const Position& pos, uint8_t* buffer)
{
    ByteWriter w(buffer, position_bytes);

    for(int i = 0; i < 32; ++i)
    {
        w.u8(uint32_t(pos.board[sq88(i * 2)]) | (uint32_t(pos.board[sq88(i * 2 + 1)]) << 4));
    }

    w.u8(pos.side);
    w.u8(pos.castling);
    w.u8(pos.ep);
    w.u16(pos.halfmove);
    w.u16(pos.ply);
    w.u16(pos.fullmove);
    w.u8(pos.mana[0]);
    w.u8(pos.mana[1]);
    w.u8(pos.escapes[0]);
    w.u8(pos.escapes[1]);
    w.u8(pos.home[0]);
    w.u8(pos.home[1]);
    w.u8(pos.rules.arcane);
    w.u8(pos.rules.max_mana);
    w.u8(pos.rules.regeneration);
    w.u8(pos.rules.ability_mask[0]);
    w.u8(pos.rules.ability_mask[1]);

    for(uint8_t cost : pos.rules.cost)
    {
        w.u8(cost);
    }

    return w.size();
}

bool read_position(const uint8_t* buffer, Position& pos)
{
    ByteReader r(buffer, position_bytes);
    Piece squares[64];

    for(int i = 0; i < 32; ++i)
    {
        uint32_t pair = r.u8();
        squares[i * 2] = Piece(pair & 15);
        squares[i * 2 + 1] = Piece(pair >> 4);
    }

    const uint32_t side = r.u8();
    const uint32_t castling = r.u8();
    const uint32_t ep = r.u8();
    const uint32_t halfmove = r.u16();
    const uint32_t ply = r.u16();
    const uint32_t fullmove = r.u16();
    uint32_t mana[2] = { r.u8(), r.u8() };
    uint32_t escapes[2] = { r.u8(), r.u8() };
    uint32_t home[2] = { r.u8(), r.u8() };
    RuleSet rules;
    rules.arcane = r.u8() != 0;
    rules.max_mana = uint8_t(r.u8());
    rules.regeneration = uint8_t(r.u8());
    rules.ability_mask[0] = uint8_t(r.u8());
    rules.ability_mask[1] = uint8_t(r.u8());

    for(uint8_t& cost : rules.cost)
    {
        cost = uint8_t(r.u8());
    }

    if(r.failed() || side > 1 || castling > 15 || halfmove > 1000 || fullmove == 0 || escapes[0] > 3 ||
       escapes[1] > 3 || rules.max_mana > 15 || rules.regeneration > 6)
    {
        return false;
    }

    // Rebuild through the validating FEN parser.
    char fen[160];
    int length = 0;
    auto put = [&](char c)
    {
        if(length < int(sizeof(fen)) - 1)
        {
            fen[length++] = c;
        }
    };
    auto put_int = [&](uint32_t value)
    {
        char digits[10];
        int count = 0;

        do
        {
            digits[count++] = char('0' + value % 10);
            value /= 10;
        }
        while(value);

        while(count)
        {
            put(digits[--count]);
        }
    };

    for(int rank = 7; rank >= 0; --rank)
    {
        int empty = 0;

        for(int file = 0; file < 8; ++file)
        {
            Piece piece = squares[rank * 8 + file];

            if(piece == EMPTY)
            {
                ++empty;
                continue;
            }

            if(! fen_piece_chars[piece])
            {
                return false;
            }

            if(empty)
            {
                put(char('0' + empty));
                empty = 0;
            }

            put(fen_piece_chars[piece]);
        }

        if(empty)
        {
            put(char('0' + empty));
        }

        if(rank)
        {
            put('/');
        }
    }

    put(' ');
    put(side ? 'b' : 'w');
    put(' ');

    if(! castling)
    {
        put('-');
    }
    else
    {
        if(castling & CASTLE_WK) put('K');
        if(castling & CASTLE_WQ) put('Q');
        if(castling & CASTLE_BK) put('k');
        if(castling & CASTLE_BQ) put('q');
    }

    put(' ');

    if(ep == NO_SQUARE)
    {
        put('-');
    }
    else
    {
        if(! on_board(int(ep)))
        {
            return false;
        }

        put(char('a' + file_of(int(ep))));
        put(char('1' + rank_of(int(ep))));
    }

    put(' ');
    put_int(halfmove);
    put(' ');
    put_int(fullmove);
    put(' ');
    put('m'); put('a'); put('n'); put('a'); put('=');
    put_int(mana[0]);
    put('/');
    put_int(mana[1]);
    put(' ');
    put('e'); put('s'); put('c'); put('a'); put('p'); put('e'); put('=');
    put_int(escapes[0]);
    put('/');
    put_int(escapes[1]);
    put(' ');
    put('h'); put('o'); put('m'); put('e'); put('=');
    constexpr const char* hex = "0123456789abcdef";
    put(hex[(home[0] >> 4) & 15]);
    put(hex[home[0] & 15]);
    put('/');
    put(hex[(home[1] >> 4) & 15]);
    put(hex[home[1] & 15]);
    fen[length] = 0;

    Position result;

    if(result.set_fen(fen, rules) != FenError::NONE)
    {
        return false;
    }

    // The ep square in the record is the internal (capturable) one; set_fen re-derives the same value.
    if(result.ep != ep || result.castling != castling || result.home[0] != home[0] || result.home[1] != home[1])
    {
        return false;
    }

    result.ply = uint16_t(ply);
    pos = result;
    return true;
}

int write_suspend_record(const SuspendedMatch& match, uint32_t sequence, uint8_t* buffer, int capacity)
{
    ByteWriter w(buffer, capacity);
    begin_record(w, RecordKind::SUSPEND, sequence);
    w.u8(match.mode);
    w.u8(match.human_mask);
    w.u8(match.difficulty);
    w.u8(match.personality);
    w.u8(match.board_theme);
    w.u8(match.opponent_faction);
    w.u8(uint8_t(match.encounter));
    w.u8(uint8_t(match.challenge));
    w.u8(uint8_t(match.gauntlet_stage));
    w.u32(match.match_seed);
    w.u32(match.objective_state);

    for(uint8_t byte : match.recorder)
    {
        w.u8(byte);
    }

    uint8_t position[position_bytes];
    write_position(match.base, position);

    for(uint8_t byte : position)
    {
        w.u8(byte);
    }

    w.u16(match.move_count);

    for(int i = 0; i < match.move_count; ++i)
    {
        w.u32(match.moves[i].data());
    }

    return finish_record(w);
}

RecordStatus read_suspend_record(const uint8_t* buffer, int size, SuspendedMatch& match, uint32_t& sequence)
{
    ByteReader r(buffer, size);
    int payload = 0;
    RecordStatus status = check_header(r, buffer, size, RecordKind::SUSPEND, sequence, payload);

    if(status != RecordStatus::VALID)
    {
        return status;
    }

    match.mode = uint8_t(r.u8());
    match.human_mask = uint8_t(r.u8());
    match.difficulty = uint8_t(r.u8());
    match.personality = uint8_t(r.u8());
    match.board_theme = uint8_t(r.u8());
    match.opponent_faction = uint8_t(r.u8());
    match.encounter = int8_t(r.u8());
    match.challenge = int8_t(r.u8());
    match.gauntlet_stage = int8_t(r.u8());
    match.match_seed = r.u32();
    match.objective_state = r.u32();

    for(uint8_t& byte : match.recorder)
    {
        byte = uint8_t(r.u8());
    }

    uint8_t position[position_bytes];

    for(uint8_t& byte : position)
    {
        byte = uint8_t(r.u8());
    }

    match.move_count = uint16_t(r.u16());

    if(r.failed() || match.move_count > Game::max_window || match.human_mask == 0 || match.human_mask > 3 ||
       match.difficulty > 3 || match.board_theme > 7 || match.opponent_faction > 6)
    {
        return RecordStatus::CORRUPTED;
    }

    for(int i = 0; i < match.move_count; ++i)
    {
        match.moves[i] = Move(r.u32());
    }

    if(r.failed() || r.position() != header_size + payload || ! read_position(position, match.base))
    {
        return RecordStatus::CORRUPTED;
    }

    return RecordStatus::VALID;
}

int choose_record(RecordStatus a, uint32_t sequence_a, RecordStatus b, uint32_t sequence_b)
{
    const bool a_valid = a == RecordStatus::VALID;
    const bool b_valid = b == RecordStatus::VALID;

    if(a_valid && b_valid)
    {
        // Wrap-safe "newer than" comparison.
        return int32_t(sequence_b - sequence_a) > 0 ? 1 : 0;
    }

    if(a_valid)
    {
        return 0;
    }

    if(b_valid)
    {
        return 1;
    }

    return -1;
}

void capture_game(const Game& game, SuspendedMatch& match)
{
    match.base = game.window_base();
    match.move_count = uint16_t(game.window_count());

    for(int i = 0; i < match.move_count; ++i)
    {
        match.moves[i] = game.window_move(i);
    }
}

bool restore_game(const SuspendedMatch& match, Game& game)
{
    return game.restore(match.base, match.moves, match.move_count, GameResult::ONGOING, ResultReason::NONE);
}

}
