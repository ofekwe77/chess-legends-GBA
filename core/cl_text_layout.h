// CHESS LEGENDS: ARCANE WARS - text measurement and line wrapping in the real font metrics.
//
// Shared by the ROM (sprite text) and the host content validator, so every string the validator accepts is laid out
// on screen exactly as it was measured. The box sizes below are the single source of truth for the text boxes.
#ifndef CL_TEXT_LAYOUT_H
#define CL_TEXT_LAYOUT_H

#include <stdint.h>

namespace cl
{

enum class FontId : uint8_t
{
    SMALL,      // 8 px rows, proportional
    BIG         // 16 px rows, proportional (titles)
};

struct TextSpan
{
    int start = 0;
    int length = 0;
};

namespace layout
{
    constexpr int dialogue_text_width = 216;    // dialogue box text area (small font)
    constexpr int dialogue_lines = 3;
    constexpr int briefing_text_width = 220;    // encounter briefing panel
    constexpr int briefing_lines = 5;
    constexpr int hint_text_width = 220;        // challenge hint / description panel
    constexpr int hint_lines = 3;
    constexpr int list_text_width = 168;        // one row of a menu list (small font)
    constexpr int title_text_width = 228;       // big-font titles (world names, encounter titles)
    constexpr int name_tag_width = 104;         // speaker name tag (small font)
    constexpr int screen_text_width = 232;      // any full-width small-font line
    constexpr int card_text_width = 208;        // rule card / gallery lore text (small font)
    constexpr int card_lines = 6;
    constexpr int help_key_width = 44;          // help page left column
    constexpr int help_text_width = 168;        // help page right column
    constexpr int help_lines = 3;               // lines per help entry
}

// Width in pixels of `length` bytes of UTF-8 text, or -1 if a character is missing from the font.
[[nodiscard]] int text_pixel_width(FontId font, const char* text, int length);
[[nodiscard]] int text_pixel_width(FontId font, const char* text);

// True if the font can draw every character (line breaks excluded).
[[nodiscard]] bool font_supports(FontId font, const char* text);

// Greedy word wrap at spaces (and explicit '\n'); a word wider than the box is split. Returns the number of lines;
// `overflow` is set when text remains after max_lines.
int wrap_lines(FontId font, const char* text, int length, int max_width, TextSpan* lines, int max_lines,
               bool* overflow = nullptr);

}

#endif
