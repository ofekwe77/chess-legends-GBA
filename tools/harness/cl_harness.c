/*
 * CHESS LEGENDS: ARCANE WARS - headless emulator test harness.
 *
 * Runs the real ROM inside the mGBA core (libmgba, statically linked), drives the joypad from a scenario script,
 * captures screenshots and audio statistics, and reads the ROM's test interface (core/cl_test_interface.h).
 *
 * Usage:
 *   cl_harness --rom ChessLegends.gba --script scenario.txt [--save file.sav] [--out dir] [--quiet] [--log]
 *
 * Scenario commands (one per line, '#' starts a comment):
 *   frames N                  run N frames with the currently held keys
 *   hold KEY...               press and keep holding keys (A B SELECT START RIGHT LEFT UP DOWN R L)
 *   release KEY...|all        release keys
 *   tap KEY [N]               hold KEY for N frames (default 2), release, run 4 frames
 *   shot NAME                 write the current frame to OUT/NAME.png
 *   audio_begin               start accumulating audio statistics
 *   audio_end NAME            stop, print RMS / peak, write OUT/NAME.wav
 *   audio_check NAME MIN [MAX] stop, fail unless MIN <= RMS <= MAX
 *   wait FIELD OP VALUE MAX   run frames until FIELD OP VALUE holds (OP: == != >= <= > < has), fail after MAX frames
 *   expect FIELD OP VALUE     fail unless the condition holds now
 *   snap FIELD / same FIELD / delta FIELD OP VALUE   compare with a remembered value
 *   cmd ID [A0 A1 A2 A3]      post a mailbox command and wait for the acknowledgement
 *   print [FIELD...]          print test-interface fields (all if none given)
 *   echo TEXT                 print text
 *   reset                     reset the console (the save file is kept)
 *
 * Exit code: 0 if every command succeeded, 1 on a failed expectation / wait, 2 on setup errors.
 */
#include <mgba/flags.h>
#include <mgba/core/core.h>
#include <mgba/core/blip_buf.h>
#include <mgba/core/log.h>
#include <mgba-util/vfs.h>

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../core/cl_test_interface.h"

#define SCREEN_W 240
#define SCREEN_H 160
#define AUDIO_RATE 32768

static struct mCore* g_core;
static uint32_t g_framebuffer[256 * SCREEN_H];
static uint32_t g_keys;
static const char* g_out_dir = ".";
static int g_quiet;
static int g_game_log;
static uint32_t g_ti_address;
static unsigned long g_frames_run;

/* ------------------------------------------------------------------ audio capture */
static int g_audio_capturing;
static int16_t* g_audio_buf;
static size_t g_audio_len, g_audio_cap;
static double g_audio_sumsq;
static int g_audio_peak;

static void drain_audio(void)
{
    struct blip_t* left = g_core->getAudioChannel(g_core, 0);
    struct blip_t* right = g_core->getAudioChannel(g_core, 1);
    short tmp[2048];

    while(blip_samples_avail(left) > 0)
    {
        int n = blip_read_samples(left, tmp, 1024, 1);
        blip_read_samples(right, tmp + 1, n, 1);

        if(! g_audio_capturing)
        {
            continue;
        }

        for(int i = 0; i < n * 2; ++i)
        {
            int v = tmp[i];
            g_audio_sumsq += (double) v * v;

            if(abs(v) > g_audio_peak)
            {
                g_audio_peak = abs(v);
            }
        }

        if(g_audio_len + (size_t) n * 2 > g_audio_cap)
        {
            g_audio_cap = (g_audio_cap + (size_t) n * 2) * 2;
            g_audio_buf = realloc(g_audio_buf, g_audio_cap * sizeof(int16_t));
        }

        memcpy(g_audio_buf + g_audio_len, tmp, (size_t) n * 2 * sizeof(int16_t));
        g_audio_len += (size_t) n * 2;
    }
}

static void run_frame(void)
{
    g_core->setKeys(g_core, g_keys);
    g_core->runFrame(g_core);
    drain_audio();
    ++g_frames_run;
}

/* ------------------------------------------------------------------ PNG writer (stored deflate blocks) */
static uint32_t crc_table[256];

static void crc_init(void)
{
    for(uint32_t n = 0; n < 256; ++n)
    {
        uint32_t c = n;

        for(int k = 0; k < 8; ++k)
        {
            c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        }

        crc_table[n] = c;
    }
}

static uint32_t crc_update(uint32_t crc, const uint8_t* buf, size_t len)
{
    uint32_t c = crc ^ 0xFFFFFFFFu;

    for(size_t i = 0; i < len; ++i)
    {
        c = crc_table[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
    }

    return c ^ 0xFFFFFFFFu;
}

static void put_be32(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t) (v >> 24);
    p[1] = (uint8_t) (v >> 16);
    p[2] = (uint8_t) (v >> 8);
    p[3] = (uint8_t) v;
}

static void write_chunk(FILE* f, const char* type, const uint8_t* data, uint32_t len)
{
    uint8_t header[8];
    put_be32(header, len);
    memcpy(header + 4, type, 4);
    fwrite(header, 1, 8, f);

    if(len)
    {
        fwrite(data, 1, len, f);
    }

    uint32_t crc = crc_update(0, (const uint8_t*) type, 4);
    crc = crc_update(crc, data, len);
    uint8_t c[4];
    put_be32(c, crc);
    fwrite(c, 1, 4, f);
}

static int save_png(const char* name)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s.png", g_out_dir, name);
    FILE* f = fopen(path, "wb");

    if(! f)
    {
        fprintf(stderr, "cannot write %s\n", path);
        return 0;
    }

    static const uint8_t signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    fwrite(signature, 1, 8, f);
    uint8_t ihdr[13];
    put_be32(ihdr, SCREEN_W);
    put_be32(ihdr + 4, SCREEN_H);
    ihdr[8] = 8;
    ihdr[9] = 2;
    ihdr[10] = 0;
    ihdr[11] = 0;
    ihdr[12] = 0;
    write_chunk(f, "IHDR", ihdr, 13);

    size_t raw_len = (size_t) SCREEN_H * (1 + SCREEN_W * 3);
    uint8_t* raw = malloc(raw_len);
    size_t p = 0;

    for(int y = 0; y < SCREEN_H; ++y)
    {
        raw[p++] = 0;

        for(int x = 0; x < SCREEN_W; ++x)
        {
            uint32_t c = g_framebuffer[y * 256 + x];
            raw[p++] = (uint8_t) (c & 0xFF);
            raw[p++] = (uint8_t) ((c >> 8) & 0xFF);
            raw[p++] = (uint8_t) ((c >> 16) & 0xFF);
        }
    }

    size_t blocks = (raw_len + 65534) / 65535;
    size_t z_len = 2 + raw_len + blocks * 5 + 4;
    uint8_t* z = malloc(z_len);
    size_t q = 0;
    z[q++] = 0x78;
    z[q++] = 0x01;
    uint32_t a = 1, b = 0;

    for(size_t off = 0; off < raw_len; off += 65535)
    {
        size_t n = raw_len - off < 65535 ? raw_len - off : 65535;
        z[q++] = (off + n >= raw_len) ? 1 : 0;
        z[q++] = (uint8_t) (n & 0xFF);
        z[q++] = (uint8_t) (n >> 8);
        z[q++] = (uint8_t) (~n & 0xFF);
        z[q++] = (uint8_t) ((~n >> 8) & 0xFF);
        memcpy(z + q, raw + off, n);
        q += n;
    }

    for(size_t i = 0; i < raw_len; ++i)
    {
        a = (a + raw[i]) % 65521;
        b = (b + a) % 65521;
    }

    put_be32(z + q, (b << 16) | a);
    q += 4;
    write_chunk(f, "IDAT", z, (uint32_t) q);
    write_chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(raw);
    free(z);

    if(! g_quiet)
    {
        printf("shot %s\n", path);
    }

    return 1;
}

static void write_wav(const char* name)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s.wav", g_out_dir, name);
    FILE* f = fopen(path, "wb");

    if(! f)
    {
        return;
    }

    uint32_t data_len = (uint32_t) (g_audio_len * sizeof(int16_t));
    uint8_t h[44];
    memcpy(h, "RIFF", 4);
    uint32_t v = 36 + data_len;
    memcpy(h + 4, &v, 4);
    memcpy(h + 8, "WAVEfmt ", 8);
    v = 16;
    memcpy(h + 16, &v, 4);
    uint16_t s = 1;
    memcpy(h + 20, &s, 2);
    s = 2;
    memcpy(h + 22, &s, 2);
    v = AUDIO_RATE;
    memcpy(h + 24, &v, 4);
    v = AUDIO_RATE * 4;
    memcpy(h + 28, &v, 4);
    s = 4;
    memcpy(h + 32, &s, 2);
    s = 16;
    memcpy(h + 34, &s, 2);
    memcpy(h + 36, "data", 4);
    memcpy(h + 40, &data_len, 4);
    fwrite(h, 1, 44, f);
    fwrite(g_audio_buf, sizeof(int16_t), g_audio_len, f);
    fclose(f);
}

/* ------------------------------------------------------------------ test interface */
struct ti_field
{
    const char* name;
    size_t offset;
};

#define CL_TI_TABLE_ENTRY(name) { #name, offsetof(struct cl_test_interface, name) },
static const struct ti_field ti_fields[] = { CL_TI_FIELDS(CL_TI_TABLE_ENTRY) };
#undef CL_TI_TABLE_ENTRY
#define TI_FIELD_COUNT (sizeof(ti_fields) / sizeof(ti_fields[0]))

static int find_test_interface(void)
{
    static const uint32_t ranges[][2] = { { 0x02000000u, 0x02040000u }, { 0x03000000u, 0x03008000u } };

    for(int r = 0; r < 2; ++r)
    {
        for(uint32_t addr = ranges[r][0]; addr < ranges[r][1]; addr += 4)
        {
            if(g_core->busRead32(g_core, addr) == CL_TI_MAGIC0 && g_core->busRead32(g_core, addr + 4) == CL_TI_MAGIC1)
            {
                g_ti_address = addr;
                return 1;
            }
        }
    }

    return 0;
}

static const struct ti_field* lookup_field(const char* name)
{
    for(size_t i = 0; i < TI_FIELD_COUNT; ++i)
    {
        if(strcmp(ti_fields[i].name, name) == 0)
        {
            return &ti_fields[i];
        }
    }

    return NULL;
}

static int ensure_ti(void)
{
    if(g_ti_address)
    {
        return 1;
    }

    for(int tries = 0; tries < 600 && ! find_test_interface(); ++tries)
    {
        run_frame();
    }

    if(! g_ti_address)
    {
        fprintf(stderr, "test interface not found in RAM\n");
        return 0;
    }

    if(! g_quiet)
    {
        printf("test interface at 0x%08X\n", g_ti_address);
    }

    return 1;
}

static uint32_t ti_read(const struct ti_field* f)
{
    return g_core->busRead32(g_core, g_ti_address + 8 + (uint32_t) (f->offset - offsetof(struct cl_test_interface, frame)));
}

static void ti_write(const char* name, uint32_t value)
{
    const struct ti_field* f = lookup_field(name);
    g_core->busWrite32(g_core, g_ti_address + 8 + (uint32_t) (f->offset - offsetof(struct cl_test_interface, frame)),
                       value);
}

static int compare(uint32_t a, const char* op, uint32_t b)
{
    if(! strcmp(op, "==")) return a == b;
    if(! strcmp(op, "!=")) return a != b;
    if(! strcmp(op, ">=")) return a >= b;
    if(! strcmp(op, "<=")) return a <= b;
    if(! strcmp(op, ">")) return a > b;
    if(! strcmp(op, "<")) return a < b;
    if(! strcmp(op, "has")) return (a & b) == b;
    return 0;
}

/* ------------------------------------------------------------------ keys */
static uint32_t key_bit(const char* k)
{
    static const char* names[] = { "A", "B", "SELECT", "START", "RIGHT", "LEFT", "UP", "DOWN", "R", "L" };

    for(int i = 0; i < 10; ++i)
    {
        if(! strcmp(names[i], k))
        {
            return 1u << i;
        }
    }

    fprintf(stderr, "unknown key %s\n", k);
    return 0;
}

static void print_fields(char** names, int count)
{
    if(! count)
    {
        for(size_t i = 0; i < TI_FIELD_COUNT; ++i)
        {
            printf("  %-18s %u\n", ti_fields[i].name, ti_read(&ti_fields[i]));
        }

        return;
    }

    for(int i = 0; i < count; ++i)
    {
        const struct ti_field* f = lookup_field(names[i]);

        if(f)
        {
            printf("  %-18s %u\n", f->name, ti_read(f));
        }
        else
        {
            printf("  %-18s (unknown field)\n", names[i]);
        }
    }
}

static int run_script(FILE* script)
{
    char line[1024];
    int line_no = 0;
    char snap_name[64] = "";
    uint32_t snap_value = 0;

    while(fgets(line, sizeof(line), script))
    {
        ++line_no;
        char* hash = strchr(line, '#');

        if(hash)
        {
            *hash = 0;
        }

        char* argv[16];
        int argc = 0;

        for(char* tok = strtok(line, " \t\r\n"); tok && argc < 16; tok = strtok(NULL, " \t\r\n"))
        {
            argv[argc++] = tok;
        }

        if(! argc)
        {
            continue;
        }

        const char* c = argv[0];

        if(! strcmp(c, "frames") && argc >= 2)
        {
            long n = strtol(argv[1], NULL, 0);

            for(long i = 0; i < n; ++i)
            {
                run_frame();
            }
        }
        else if(! strcmp(c, "hold"))
        {
            for(int i = 1; i < argc; ++i)
            {
                g_keys |= key_bit(argv[i]);
            }
        }
        else if(! strcmp(c, "release"))
        {
            for(int i = 1; i < argc; ++i)
            {
                if(! strcmp(argv[i], "all"))
                {
                    g_keys = 0;
                }
                else
                {
                    g_keys &= ~key_bit(argv[i]);
                }
            }
        }
        else if(! strcmp(c, "tap") && argc >= 2)
        {
            int n = argc >= 3 ? atoi(argv[2]) : 2;
            uint32_t bit = key_bit(argv[1]);
            g_keys |= bit;

            for(int i = 0; i < n; ++i)
            {
                run_frame();
            }

            g_keys &= ~bit;

            for(int i = 0; i < 4; ++i)
            {
                run_frame();
            }
        }
        else if(! strcmp(c, "shot") && argc >= 2)
        {
            if(! save_png(argv[1]))
            {
                return 2;
            }
        }
        else if(! strcmp(c, "audio_begin"))
        {
            g_audio_capturing = 1;
            g_audio_len = 0;
            g_audio_sumsq = 0;
            g_audio_peak = 0;
        }
        else if(! strcmp(c, "audio_end") && argc >= 2)
        {
            g_audio_capturing = 0;
            double rms = g_audio_len ? sqrt(g_audio_sumsq / (double) g_audio_len) : 0;
            printf("audio %s samples=%zu rms=%.1f peak=%d\n", argv[1], g_audio_len / 2, rms, g_audio_peak);
            write_wav(argv[1]);
        }
        else if(! strcmp(c, "audio_check") && argc >= 3)
        {
            g_audio_capturing = 0;
            double rms = g_audio_len ? sqrt(g_audio_sumsq / (double) g_audio_len) : 0;
            double lo = atof(argv[2]);
            double hi = argc >= 4 ? atof(argv[3]) : 1e12;

            if(rms < lo || rms > hi)
            {
                printf("FAIL line %d: audio %s rms %.1f outside range\n", line_no, argv[1], rms);
                return 1;
            }

            if(! g_quiet)
            {
                printf("ok audio %s rms=%.1f peak=%d\n", argv[1], rms, g_audio_peak);
            }
        }
        else if(! strcmp(c, "wait") && argc >= 5)
        {
            if(! ensure_ti())
            {
                return 2;
            }

            const struct ti_field* f = lookup_field(argv[1]);

            if(! f)
            {
                fprintf(stderr, "line %d: unknown field %s\n", line_no, argv[1]);
                return 2;
            }

            uint32_t value = (uint32_t) strtoul(argv[3], NULL, 0);
            long max = strtol(argv[4], NULL, 0);
            long waited = 0;

            while(! compare(ti_read(f), argv[2], value))
            {
                if(waited++ >= max)
                {
                    printf("FAIL line %d: wait %s %s %u timed out (value %u)\n", line_no, argv[1], argv[2], value,
                           ti_read(f));
                    return 1;
                }

                run_frame();
            }

            if(! g_quiet)
            {
                printf("ok wait %s %s %u (%ld frames)\n", argv[1], argv[2], value, waited);
            }
        }
        else if(! strcmp(c, "expect") && argc >= 4)
        {
            if(! ensure_ti())
            {
                return 2;
            }

            const struct ti_field* f = lookup_field(argv[1]);

            if(! f)
            {
                fprintf(stderr, "line %d: unknown field %s\n", line_no, argv[1]);
                return 2;
            }

            uint32_t value = (uint32_t) strtoul(argv[3], NULL, 0);
            uint32_t actual = ti_read(f);

            if(! compare(actual, argv[2], value))
            {
                printf("FAIL line %d: expect %s %s %u (actual %u)\n", line_no, argv[1], argv[2], value, actual);
                return 1;
            }

            if(! g_quiet)
            {
                printf("ok expect %s %s %u (actual %u)\n", argv[1], argv[2], value, actual);
            }
        }
        else if((! strcmp(c, "snap") || ! strcmp(c, "same") || ! strcmp(c, "delta")) && argc >= 2)
        {
            if(! ensure_ti())
            {
                return 2;
            }

            const struct ti_field* f = lookup_field(argv[1]);

            if(! f)
            {
                fprintf(stderr, "line %d: unknown field %s\n", line_no, argv[1]);
                return 2;
            }

            uint32_t actual = ti_read(f);

            if(! strcmp(c, "snap"))
            {
                snprintf(snap_name, sizeof(snap_name), "%s", argv[1]);
                snap_value = actual;
            }
            else if(strcmp(snap_name, argv[1]) != 0)
            {
                printf("FAIL line %d: %s %s: the field was not snapped\n", line_no, c, argv[1]);
                return 1;
            }
            else if(! strcmp(c, "same"))
            {
                if(actual != snap_value)
                {
                    printf("FAIL line %d: same %s (snapped %u, now %u)\n", line_no, argv[1], snap_value, actual);
                    return 1;
                }
            }
            else
            {
                if(argc < 4)
                {
                    fprintf(stderr, "line %d: usage: delta FIELD OP VALUE\n", line_no);
                    return 2;
                }

                long d = (long) (int32_t) (actual - snap_value);
                long v = strtol(argv[3], NULL, 0);
                const char* op = argv[2];
                int ok = ! strcmp(op, "==") ? d == v : ! strcmp(op, "!=") ? d != v : ! strcmp(op, ">=") ? d >= v :
                         ! strcmp(op, "<=") ? d <= v : ! strcmp(op, ">") ? d > v : ! strcmp(op, "<") ? d < v : 0;

                if(! ok)
                {
                    printf("FAIL line %d: delta %s %s %ld (actual %ld)\n", line_no, argv[1], op, v, d);
                    return 1;
                }
            }
        }
        else if(! strcmp(c, "cmd") && argc >= 2)
        {
            if(! ensure_ti())
            {
                return 2;
            }

            uint32_t id = (uint32_t) strtoul(argv[1], NULL, 0);
            const char* arg_names[4] = { "arg0", "arg1", "arg2", "arg3" };

            for(int i = 0; i < 4; ++i)
            {
                ti_write(arg_names[i], argc > 2 + i ? (uint32_t) strtoul(argv[2 + i], NULL, 0) : 0);
            }

            ti_write("ack", 0);
            ti_write("cmd", id);
            const struct ti_field* ack = lookup_field("ack");
            int waited = 0;

            while(ti_read(ack) != id)
            {
                if(waited++ > 900)
                {
                    printf("FAIL line %d: command %u not acknowledged\n", line_no, id);
                    return 1;
                }

                run_frame();
            }

            if(! g_quiet)
            {
                printf("ok cmd %u acknowledged after %d frames\n", id, waited);
            }
        }
        else if(! strcmp(c, "print"))
        {
            if(! ensure_ti())
            {
                return 2;
            }

            print_fields(argv + 1, argc - 1);
        }
        else if(! strcmp(c, "echo"))
        {
            for(int i = 1; i < argc; ++i)
            {
                printf("%s%s", argv[i], i + 1 < argc ? " " : "\n");
            }
        }
        else if(! strcmp(c, "reset"))
        {
            g_core->reset(g_core);
            g_ti_address = 0;
        }
        else
        {
            fprintf(stderr, "line %d: unknown command %s\n", line_no, c);
            return 2;
        }
    }

    return 0;
}

static void quiet_log(struct mLogger* logger, int category, enum mLogLevel level, const char* format, va_list args)
{
    (void) logger;
    (void) level;

    if(g_game_log)
    {
        const char* name = mLogCategoryName(category);

        if(name && strstr(name, "Debug"))
        {
            vfprintf(stdout, format, args);
            fputc('\n', stdout);
        }
    }
}

int main(int argc, char** argv)
{
    const char* rom = NULL;
    const char* script_path = NULL;
    const char* save_path = NULL;

    for(int i = 1; i < argc; ++i)
    {
        if(! strcmp(argv[i], "--rom") && i + 1 < argc) rom = argv[++i];
        else if(! strcmp(argv[i], "--script") && i + 1 < argc) script_path = argv[++i];
        else if(! strcmp(argv[i], "--save") && i + 1 < argc) save_path = argv[++i];
        else if(! strcmp(argv[i], "--out") && i + 1 < argc) g_out_dir = argv[++i];
        else if(! strcmp(argv[i], "--quiet")) g_quiet = 1;
        else if(! strcmp(argv[i], "--log")) g_game_log = 1;
        else
        {
            fprintf(stderr, "usage: %s --rom FILE --script FILE [--save FILE] [--out DIR] [--quiet] [--log]\n", argv[0]);
            return 2;
        }
    }

    if(! rom || ! script_path)
    {
        fprintf(stderr, "--rom and --script are required\n");
        return 2;
    }

    crc_init();
    static struct mLogger logger = { .log = quiet_log };
    mLogSetDefaultLogger(&logger);

    g_core = mCoreFind(rom);

    if(! g_core || ! g_core->init(g_core))
    {
        fprintf(stderr, "cannot create core for %s\n", rom);
        return 2;
    }

    mCoreInitConfig(g_core, NULL);
    g_core->setVideoBuffer(g_core, (color_t*) g_framebuffer, 256);
    g_core->setAudioBufferSize(g_core, 4096);
    blip_set_rates(g_core->getAudioChannel(g_core, 0), g_core->frequency(g_core), AUDIO_RATE);
    blip_set_rates(g_core->getAudioChannel(g_core, 1), g_core->frequency(g_core), AUDIO_RATE);

    if(! mCoreLoadFile(g_core, rom))
    {
        fprintf(stderr, "cannot load ROM %s\n", rom);
        return 2;
    }

    if(save_path)
    {
        struct VFile* save = VFileOpen(save_path, O_CREAT | O_RDWR);

        if(! save || ! g_core->loadSave(g_core, save))
        {
            fprintf(stderr, "cannot attach save file %s\n", save_path);
            return 2;
        }
    }

    g_core->reset(g_core);
    FILE* script = fopen(script_path, "r");

    if(! script)
    {
        fprintf(stderr, "cannot open script %s\n", script_path);
        return 2;
    }

    int result = run_script(script);
    fclose(script);
    printf("frames run: %lu, result: %s\n", g_frames_run, result == 0 ? "PASS" : "FAIL");
    g_core->unloadROM(g_core);
    g_core->deinit(g_core);
    return result;
}
