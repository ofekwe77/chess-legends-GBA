// CHESS LEGENDS: ARCANE WARS - text rendering helpers (sprite fonts, screen pixel coordinates).
#ifndef CL_TEXT_H
#define CL_TEXT_H

#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_string_view.h"
#include "bn_vector.h"

namespace cl::gba
{

enum class TextColor : uint8_t
{
    WHITE,
    GOLD,
    GRAY,
    RED,
    CYAN,
    DARK
};

enum class TitleColor : uint8_t
{
    GOLD,
    SILVER,
    RUBY,
    ARCANE,
    DIM
};

enum class Align : uint8_t
{
    LEFT,
    CENTER,
    RIGHT
};

// Small font (8 px rows): x is the left edge / centre / right edge depending on align, y the top of the line.
void print(bn::ivector<bn::sprite_ptr>& output, int x, int y, const bn::string_view& text,
           TextColor color = TextColor::WHITE, Align align = Align::LEFT, int bg_priority = 0);

// Title font (16 px rows).
void print_big(bn::ivector<bn::sprite_ptr>& output, int x, int y, const bn::string_view& text,
               TitleColor color = TitleColor::GOLD, Align align = Align::LEFT, int bg_priority = 0);

// Small-font lines kept between redraws. A redraw pass (begin, print..., end) prints its lines again: a line
// identical to the previous pass (same place, text, color, alignment) keeps its sprites, a changed one is
// regenerated, and lines not printed again are removed. Generating text costs a large part of a frame on the
// console, while most lines of a panel stay the same from one turn to the next.
class TextCache
{
public:
    void begin();
    void print(int x, int y, const bn::string_view& text, TextColor color = TextColor::WHITE,
               Align align = Align::LEFT, int bg_priority = 0);
    void end();
    void clear();

private:
    struct Line
    {
        bn::vector<bn::sprite_ptr, 8> sprites;
        bn::string<48> text;
        int16_t x = 0;
        int16_t y = 0;
        TextColor color = TextColor::WHITE;
        Align align = Align::LEFT;
        int8_t bg_priority = 0;
        bool cached = false;    // text holds the line's content (lines longer than 48 bytes are never reused)
        bool used = false;
    };

    static constexpr int max_lines = 24;

    bn::vector<Line, max_lines> _lines;
};

[[nodiscard]] int text_width(const bn::string_view& text);
[[nodiscard]] int big_text_width(const bn::string_view& text);

// Splits text into lines of at most max_width pixels (small font). Returns the number of lines written.
int wrap_text(const bn::string_view& text, int max_width, bn::string_view* lines, int max_lines);

}

#endif
