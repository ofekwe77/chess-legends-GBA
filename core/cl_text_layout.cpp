// CHESS LEGENDS: ARCANE WARS - text measurement and line wrapping.
#include "cl_text_layout.h"

#include "cl_metrics_big.h"
#include "cl_metrics_small.h"

namespace cl
{

namespace
{
    int string_length(const char* text)
    {
        int length = 0;

        while(text[length])
        {
            ++length;
        }

        return length;
    }

    // Width of the glyph starting at text[index] (sets its byte count), -1 if the font lacks it.
    int glyph_width(FontId font, const char* text, int length, int index, int& bytes)
    {
        const int8_t* widths = font == FontId::SMALL ? metrics::small_widths : metrics::big_widths;
        const char* const* extras = font == FontId::SMALL ? metrics::small_extras : metrics::big_extras;
        const int extras_count = font == FontId::SMALL ? metrics::small_extras_count : metrics::big_extras_count;
        const unsigned char c = (unsigned char) text[index];

        if(c < 0x80)
        {
            bytes = 1;
            return c >= 32 && c <= 126 ? widths[c - 32] : -1;
        }

        bytes = c >= 0xF0 ? 4 : (c >= 0xE0 ? 3 : 2);

        for(int e = 0; e < extras_count; ++e)
        {
            const char* extra = extras[e];
            const int extra_length = string_length(extra);

            if(extra_length != bytes || index + extra_length > length)
            {
                continue;
            }

            bool same = true;

            for(int b = 0; b < extra_length && same; ++b)
            {
                same = extra[b] == text[index + b];
            }

            if(same)
            {
                return widths[95 + e];
            }
        }

        return -1;
    }
}

int text_pixel_width(FontId font, const char* text, int length)
{
    int width = 0;

    for(int i = 0; i < length;)
    {
        int bytes;
        const int w = glyph_width(font, text, length, i, bytes);

        if(w < 0)
        {
            return -1;
        }

        width += w;
        i += bytes;
    }

    return width;
}

int text_pixel_width(FontId font, const char* text)
{
    return text_pixel_width(font, text, string_length(text));
}

bool font_supports(FontId font, const char* text)
{
    const int length = string_length(text);

    for(int i = 0; i < length;)
    {
        if(text[i] == '\n')
        {
            ++i;
            continue;
        }

        int bytes;

        if(glyph_width(font, text, length, i, bytes) < 0)
        {
            return false;
        }

        i += bytes;
    }

    return true;
}

int wrap_lines(FontId font, const char* text, int length, int max_width, TextSpan* lines, int max_lines,
               bool* overflow)
{
    int count = 0;
    int line_start = 0;
    int last_break = -1;
    int width = 0;
    int i = 0;

    if(overflow)
    {
        *overflow = false;
    }

    while(i <= length)
    {
        if(count >= max_lines)
        {
            if(overflow && line_start < length)
            {
                *overflow = true;
            }

            break;
        }

        if(i == length || text[i] == '\n')
        {
            // A break at the very end of the text leaves nothing for one more line.
            if(i == length && i == line_start && count > 0 && text[i - 1] != '\n')
            {
                break;
            }

            lines[count].start = line_start;
            lines[count].length = i - line_start;
            ++count;
            line_start = i + 1;
            last_break = -1;
            width = 0;
            ++i;
            continue;
        }

        int bytes;
        int w = glyph_width(font, text, length, i, bytes);
        w = w < 0 ? 0 : w;

        if(text[i] == ' ')
        {
            last_break = i;
        }

        if(width + w > max_width && i > line_start)
        {
            const int cut = last_break > line_start ? last_break : i;
            lines[count].start = line_start;
            lines[count].length = cut - line_start;
            ++count;
            line_start = cut == last_break ? cut + 1 : cut;
            last_break = -1;
            width = 0;
            i = line_start;
            continue;
        }

        width += w;
        i += bytes;
    }

    return count;
}

}
