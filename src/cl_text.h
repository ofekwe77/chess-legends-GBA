// CHESS LEGENDS: ARCANE WARS - text rendering helpers (sprite fonts, screen pixel coordinates).
#ifndef CL_TEXT_H
#define CL_TEXT_H

#include "bn_sprite_ptr.h"
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

[[nodiscard]] int text_width(const bn::string_view& text);
[[nodiscard]] int big_text_width(const bn::string_view& text);

// Splits text into lines of at most max_width pixels (small font). Returns the number of lines written.
int wrap_text(const bn::string_view& text, int max_width, bn::string_view* lines, int max_lines);

}

#endif
