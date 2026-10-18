// CHESS LEGENDS: ARCANE WARS - text rendering helpers.
#include "cl_text.h"

#include "bn_sprite_text_generator.h"
#include "bn_sprite_palette_item.h"

#include "cl_font_big.h"
#include "cl_font_small.h"
#include "cl_text_layout.h"

#include "bn_sprite_palette_items_text_white.h"
#include "bn_sprite_palette_items_text_gold.h"
#include "bn_sprite_palette_items_text_gray.h"
#include "bn_sprite_palette_items_text_red.h"
#include "bn_sprite_palette_items_text_cyan.h"
#include "bn_sprite_palette_items_text_dark.h"
#include "bn_sprite_palette_items_title_gold.h"
#include "bn_sprite_palette_items_title_silver.h"
#include "bn_sprite_palette_items_title_ruby.h"
#include "bn_sprite_palette_items_title_arcane.h"
#include "bn_sprite_palette_items_title_dim.h"

namespace cl::gba
{

namespace
{
    bn::sprite_text_generator& small_generator()
    {
        static bn::sprite_text_generator generator(fonts::small);
        return generator;
    }

    bn::sprite_text_generator& big_generator()
    {
        static bn::sprite_text_generator generator(fonts::big);
        return generator;
    }

    const bn::sprite_palette_item& small_palette(TextColor color)
    {
        switch(color)
        {
        case TextColor::GOLD: return bn::sprite_palette_items::text_gold;
        case TextColor::GRAY: return bn::sprite_palette_items::text_gray;
        case TextColor::RED: return bn::sprite_palette_items::text_red;
        case TextColor::CYAN: return bn::sprite_palette_items::text_cyan;
        case TextColor::DARK: return bn::sprite_palette_items::text_dark;
        default: return bn::sprite_palette_items::text_white;
        }
    }

    const bn::sprite_palette_item& big_palette(TitleColor color)
    {
        switch(color)
        {
        case TitleColor::SILVER: return bn::sprite_palette_items::title_silver;
        case TitleColor::RUBY: return bn::sprite_palette_items::title_ruby;
        case TitleColor::ARCANE: return bn::sprite_palette_items::title_arcane;
        case TitleColor::DIM: return bn::sprite_palette_items::title_dim;
        default: return bn::sprite_palette_items::title_gold;
        }
    }

    void configure(bn::sprite_text_generator& generator, Align align, int bg_priority)
    {
        switch(align)
        {
        case Align::CENTER:
            generator.set_center_alignment();
            break;

        case Align::RIGHT:
            generator.set_right_alignment();
            break;

        default:
            generator.set_left_alignment();
            break;
        }

        generator.set_bg_priority(bg_priority);
    }
}

void print(bn::ivector<bn::sprite_ptr>& output, int x, int y, const bn::string_view& text, TextColor color,
           Align align, int bg_priority)
{
    bn::sprite_text_generator& generator = small_generator();
    generator.set_palette_item(small_palette(color));
    configure(generator, align, bg_priority);
    generator.generate(x - 120, y + 4 - 80, text, output);
}

void print_big(bn::ivector<bn::sprite_ptr>& output, int x, int y, const bn::string_view& text, TitleColor color,
               Align align, int bg_priority)
{
    bn::sprite_text_generator& generator = big_generator();
    generator.set_palette_item(big_palette(color));
    configure(generator, align, bg_priority);
    generator.generate(x - 120, y + 8 - 80, text, output);
}

int text_width(const bn::string_view& text)
{
    return small_generator().width(text);
}

int big_text_width(const bn::string_view& text)
{
    return big_generator().width(text);
}

int wrap_text(const bn::string_view& text, int max_width, bn::string_view* lines, int max_lines)
{
    // Same layout code as the host content validator (core/cl_text_layout.cpp).
    TextSpan spans[8];
    const int limit = max_lines < 8 ? max_lines : 8;
    const int count = wrap_lines(FontId::SMALL, text.data(), text.size(), max_width, spans, limit);

    for(int i = 0; i < count; ++i)
    {
        lines[i] = text.substr(spans[i].start, spans[i].length);
    }

    return count;
}

}
