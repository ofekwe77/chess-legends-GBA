"""Achievement icons of CHESS LEGENDS: ARCANE WARS (16x16, UI palette): one per achievement plus a padlock.

Icons are hand-written pixel templates (12x12 drawing area, centred) outlined automatically in ink. The six spell
achievements reuse the ability icon artwork of gen_ui.py so the same symbol means the same spell everywhere.
Order = cl::Achievement order (core/cl_progress.h); frame 32 is the locked icon.
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'gfx'))

from canvas import Canvas  # noqa: E402
from imageio import write_bmp, write_json, write_png  # noqa: E402
import palettes as P  # noqa: E402
import gen_ui  # noqa: E402

# UI palette letters: k ink, d/D panel darks, m/M panel mids, g/G/y/Y golds, w white, u muted, r red, c cyan,
# v violet, e green.
LETTERS = {'.': 0, 'k': 1, 'd': 2, 'D': 3, 'm': 4, 'M': 5, 'g': 6, 'G': 7, 'y': 8, 'Y': 9, 'w': 10, 'u': 11,
           'r': 12, 'c': 13, 'v': 14, 'e': 15}

T = {
    'trophy': [
        '............',
        '.yYYYYYYYYy.',
        'gy.yYYYYy.yg',
        'g..yYYYYy..g',
        '.g.GyYYyG.g.',
        '..ggGyyGgg..',
        '....GyyG....',
        '.....Gg.....',
        '.....Gg.....',
        '....GyyG....',
        '...gGGGGg...',
        '...gggggg...'],
    'book': [
        '............',
        '.wwww..wwww.',
        'wMMMMw.wMMMw',
        'wwwwwwkwwwww',
        'wuuuuwkwuuuw',
        'wwwwwwkwwwww',
        'wuuuuwkwuuuw',
        'wwwwwwkwwwww',
        'wuuuuwkwuuuw',
        'wwwwwwkwwwww',
        '.ggggg.gggg.',
        '............'],
    'check': [
        '...rrrrrr...',
        '..rrrrrrrr..',
        '.rrrrwwrrrr.',
        'rrrrrwwrrrrr',
        'rrrrrwwrrrrr',
        'rrrrrwwrrrrr',
        'rrrrrwwrrrrr',
        'rrrrrrrrrrrr',
        'rrrrrwwrrrrr',
        '.rrrrwwrrrr.',
        '..rrrrrrrr..',
        '...rrrrrr...'],
    'queensbane': [
        'y..y....y..y',
        'yy.yy..yy.yy',
        'yyyyyyyyyyyy',
        '.gyyyyyyyyg.',
        '.gggggggggg.',
        '....rr......',
        '...rr.......',
        'rrrrrrrrrrrr',
        '.......rr...',
        '......rr....',
        '............',
        '............'],
    'pawn': [
        '............',
        '.....ww.....',
        '....wwww....',
        '....wwww....',
        '.....ww.....',
        '....wwww....',
        '.....ww.....',
        '.....ww.....',
        '....wwww....',
        '...wwwwww...',
        '..wwwwwwww..',
        '..uuuuuuuu..'],
    'toppled_king': [
        '............',
        '............',
        '............',
        '..........y.',
        '.........yyy',
        '..wwww.wwwy.',
        '.wwwwwwwwww.',
        'wwwwwwwwwww.',
        '.wwwwwwwwww.',
        '..wwww.www..',
        'uuuuuuuuuuuu',
        '............'],
    'coronation': [
        'y.y.y.y.....',
        'yyyyyyy.....',
        '.ggggg......',
        '............',
        '.....ww.....',
        '....wwww....',
        '.....ww.....',
        '....wwww....',
        '.....ww.....',
        '...wwwwww...',
        '..wwwwwwww..',
        '..uuuuuuuu..'],
    'castle': [
        'M.M.MM.M.M..',
        'MMMMMMMMMM..',
        '.MmmmmmmM...',
        '.MmmmmmmM...',
        '.MmmkkmmM.c.',
        '.MmmkkmmMccc',
        '.MmmmmmmMccc',
        '.MmmmmmmMccc',
        '.MmmmmmmM.c.',
        'MMMMMMMMMM..',
        'mmmmmmmmmm..',
        '............'],
    'en_passant': [
        '............',
        '..ww....uu..',
        '.wwww..uuuu.',
        '..ww....uu..',
        '.wwww..uuuu.',
        'wwwwwwuuuuuu',
        '............',
        '...ccccc....',
        '..c.........',
        '.cccccccc...',
        '..c.........',
        '...ccccc....'],
    'spark': [
        '.....c......',
        '.....c......',
        '....ccc.....',
        '.c..cwc..c..',
        '..ccwwwcc...',
        'ccwwwYwwwcc.',
        '..ccwwwcc...',
        '.c..cwc..c..',
        '....ccc.....',
        '.....c......',
        '.....c......',
        '............'],
    'sword': [
        '..........ww',
        '.........www',
        '........www.',
        '.......www..',
        '......www...',
        '.....www....',
        '..g.www.....',
        '..ggww......',
        '...gg.......',
        '..gyyg......',
        '.gy..g......',
        'gg..........'],
    'crown_sword': [
        'y.y.y.....ww',
        'yyyyy....www',
        '.ggg....www.',
        '.......www..',
        '......www...',
        '.....www....',
        '..g.www.....',
        '..ggww......',
        '...gg.......',
        '..gyyg......',
        '.gy..g......',
        'gg..........'],
    'gems': [
        '...cc..cc...',
        '..cwcccwcc..',
        '..cccc.ccc..',
        '...cc..cc...',
        '.cc..cc..cc.',
        'cwcccwcccwcc',
        'cccc.ccc.ccc',
        '.cc..cc..cc.',
        '............',
        '...vvvvvv...',
        '..v......v..',
        '............'],
    'weave': [
        '...vvvv.....',
        '..v....v....',
        '.v..cccc....',
        '.v.c..v.c...',
        '.v.c..v.c...',
        '..vcvv..c...',
        '...c....c...',
        '...cv.vvcvv.',
        '....cc.c...v',
        '.......v...v',
        '........vvv.',
        '............'],
    'shield': [
        'GGGGGGGGGGGG',
        'GccccccccccG',
        'GccwccccccyG',
        'GcwwcccccyyG',
        'GccccccccccG',
        'GccccyycccG.',
        '.GcccyyccccG',
        '.GccccccccG.',
        '..GccccccG..',
        '...GccccG...',
        '....GccG....',
        '.....GG.....'],
    'double_check': [
        '.rr....rr...',
        '.rr....rr...',
        '.rr....rr...',
        '.rr....rr...',
        '.rr....rr...',
        '.rr....rr...',
        '.rr....rr...',
        '............',
        '.rr....rr...',
        '.rr....rr...',
        '............',
        '............'],
    'diamond': [
        '............',
        '...wwwwww...',
        '..wcwwccwc..',
        '.wccwccwccw.',
        'wwwwwwwwwwww',
        '.cccccccccc.',
        '..ccccccwc..',
        '...cccccc...',
        '....cccc....',
        '.....cc.....',
        '............',
        '............'],
    'hourglass': [
        'gggggggggggg',
        '.yyyyyyyyyy.',
        '.yYYYYYYYYy.',
        '..yYYYYYYy..',
        '...yYYYYy...',
        '....yYYy....',
        '....y..y....',
        '...y.YY.y...',
        '..y.YYYY.y..',
        '.yYYYYYYYYy.',
        '.yyyyyyyyyy.',
        'gggggggggggg'],
    'leaf': [
        '.........ee.',
        '.......eeee.',
        '.....eeeeee.',
        '...eeeweeee.',
        '..eeeweeeee.',
        '.eeeweeeee..',
        '.eewweeeee..',
        '.eweeeeee...',
        '.weeeeee....',
        'w.eeee......',
        'w...........',
        '............'],
    'three_crowns': [
        'y.y.y.......',
        'yyyyy.......',
        '.ggg........',
        '...y.y.y....',
        '...yyyyy....',
        '....ggg.....',
        '......y.y.y.',
        '......yyyyy.',
        '.......ggg..',
        '............',
        '............',
        '............'],
    'grand_crown': [
        '............',
        'y....y....y.',
        'yy..yyy..yy.',
        'yyy.yyy.yyy.',
        'yyyyyyyyyyy.',
        'yyrryccyrry.',
        'yyrryccyrry.',
        'yyyyyyyyyyy.',
        'gggggggggggg',
        'GGGGGGGGGGGG',
        '............',
        '............'],
    'key': [
        '..yyyy......',
        '.y....y.....',
        'y..yy..y....',
        'y..yy..y....',
        '.y....yyyyyy',
        '..yyyy....gy',
        '.........g.y',
        '.........g..',
        '............',
        '............',
        '............',
        '............'],
    'medals': [
        '.rr..cc..vv.',
        '.rr..cc..vv.',
        '..r..c...v..',
        '.yyy.uuu.ggg',
        'yYYYuwwwgGGG',
        'yYYYuwwwgGGG',
        '.yyy.uuu.ggg',
        '............',
        '....eeeee...',
        '...e.....e..',
        '............',
        '............'],
    'crossed_swords': [
        'ww........ww',
        'www......www',
        '.www....www.',
        '..www..www..',
        '...wwwwww...',
        '....wwww....',
        '...gwwwwg...',
        '..gwwggwwg..',
        '.gg.g..g.gg.',
        'yg........gy',
        'yy........yy',
        '............'],
    'rivals': [
        '............',
        '..ww....MM..',
        '.wwww..MMMM.',
        '.wwww..MMMM.',
        '..ww....MM..',
        '.wwww..MMMM.',
        '..ww....MM..',
        '.wwww..MMMM.',
        'wwwwwwMMMMMM',
        '............',
        '...y....y...',
        '............'],
    'scroll': [
        '..ywwwwwwww.',
        '.ywuuuuuuuw.',
        '.yw.......w.',
        '..wuuuuuuuw.',
        '..w.......w.',
        '..wuuuuuuuw.',
        '..w.......w.',
        '..wuuuuuuuw.',
        '..w.......wy',
        '..wwwwwwwwwy',
        '...yyyyyyyy.',
        '............'],
    'padlock': [
        '....uuuu....',
        '...u....u...',
        '..u......u..',
        '..u......u..',
        '..u......u..',
        '.MMMMMMMMMM.',
        '.MmmmmmmmmM.',
        '.MmmmkkmmmM.',
        '.MmmmkkmmmM.',
        '.MmmmmkmmmM.',
        '.MmmmmmmmmM.',
        '.MMMMMMMMMM.'],
}

ORDER = ['trophy', 'book', 'check', 'queensbane', 'pawn', 'toppled_king', 'coronation', 'castle', 'en_passant',
         'spark', 'spell:0', 'spell:1', 'spell:2', 'spell:3', 'spell:4', 'spell:5', 'sword', 'crown_sword', 'gems',
         'weave', 'shield', 'double_check', 'diamond', 'hourglass', 'leaf', 'three_crowns', 'grand_crown', 'key',
         'medals', 'crossed_swords', 'rivals', 'scroll', 'padlock']


def icon(name):
    c = Canvas(16, 16)
    rows = T[name]
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            if LETTERS[ch]:
                c.set(2 + x, 2 + y, LETTERS[ch])
    c.outline(1)
    return c


def generate(root):
    graphics = os.path.join(root, 'graphics')
    spells = ['shadow_step', 'phase_shift', 'ghost_walk', 'castle_swap', 'royal_command', 'royal_escape']
    sheet = Canvas(16, 16 * len(ORDER))
    for i, name in enumerate(ORDER):
        if name.startswith('spell:'):
            art = gen_ui.ability_icon(spells[int(name[6:])], True)
        else:
            art = icon(name)
        sheet.blit(art, 0, i * 16)
    write_bmp(os.path.join(graphics, 'achievement_icons.bmp'), sheet, P.UI, bpp=4)
    write_json(os.path.join(graphics, 'achievement_icons.json'), {'type': 'sprite', 'height': 16})
    preview = Canvas(16 * 11, 16 * 3, fill=3)
    for i in range(len(ORDER)):
        preview.blit(sheet.crop(0, i * 16, 16, 16), (i % 11) * 16, (i // 11) * 16)
    write_png(os.path.join(root, 'assets_source', 'previews', 'achievement_icons.png'), preview, P.UI, scale=4,
              transparent_color=None)


if __name__ == '__main__':
    generate(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    print('icons generated')
