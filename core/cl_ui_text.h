// CHESS LEGENDS: ARCANE WARS - fixed wording of the match side panel.
//
// The side panel is 84 px wide (layout::panel_text_width). Its fixed strings live here so tools/validation can
// check every one of them, and the worst case of each formatted line, against the real font metrics.
#ifndef CL_UI_TEXT_H
#define CL_UI_TEXT_H

namespace cl::ui
{
    constexpr const char* panel_empty_square = "PIECE  ◇ Ⓡ";     // cursor on an empty square (then the square)
    constexpr const char* panel_threats = "THREATS  ";          // followed by the square
    constexpr const char* panel_threats_note = "Enemies marked";
    constexpr const char* panel_captured = "CAPTURED";
    constexpr const char* panel_last = "Last ";                 // followed by the coordinate notation
    constexpr const char* panel_sealed = "Sealed here";
    constexpr const char* panel_pick_target = "Pick a target";
    constexpr const char* panel_cast = "Ⓛ to cast";
    constexpr const char* panel_escape_ready = "Escape ready";
    constexpr const char* panel_escape_used = "Escape used";
    constexpr const char* panel_check = "CHECK!";

    // Pixel offset of the cost line (right of the 16 px ability icon).
    constexpr int panel_icon_text_offset = 20;
    // Minimum gap between the chess name and the class name on the panel header line.
    constexpr int panel_header_gap = 3;
    // Status line above the board (x = 4 up to the panel at x = 144).
    constexpr int status_text_width = 136;
}

#endif
