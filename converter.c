#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <alloca.h>
#include <ctype.h>
#include <assert.h>


#ifdef XPM_FILE
# include XPM_FILE
#else
# error XPM_FILE not defined
#endif


#define MAX_SCREEN5_COLORS 16
#define MAX_V9990_COLORS   16
#define USED               true
#define IGNORED            false
#define RGB_TXT_SIZE       782
// error codes
#define OK                 0
#define PARAMETER_EXPECTED 1
#define PARAMETER_TYPE     2
#define MODE_MISMATCH      3
#define INVALID_VALUE      4
#define FILE_EXPECTED      5

enum DATA_TYPE {
    NONE = 0,
    IMAGE = 1,                  // output image data (pattern) only
    PALETTE = 2,                // output palette data (colors) only
    BOTH = 3,
} data = NONE;

enum MODE {
    STDOUT = 0,                 // send C-style data to standard output
    HEADER,                     // no pattern and palette data, just the declarations
    RAW,                        // raw mode prints data byte directly to binary file
    BASIC,                      // raw mode + width/height header
} mode = STDOUT;

enum SCREEN_TYPE {
    SCREEN5 = 0,                // MSX2 SCREEN5 output (V9938 GRAPHICS4)
    P1,                         // GFX9000 pattern mode (V9990 P1)
} screen = SCREEN5;

struct palette {
    bool used;
    int8_t ci;                  // Color Index
    uint8_t r;                  // 5bit red (P1) / 3bit red (SC5)
    uint8_t rr;                 // original red
    uint8_t g;                  // 5bit green (P1) / 3bit green (SC5)
    uint8_t gg;                 // original green
    uint8_t b;                  // 5bit blue (P1) / 3bit blue (SC5)
    uint8_t bb;                 // original blue
    const char* s;              // encoded pixels
};

struct palette* palette_ptr;

// add rgb.txt as binary data
extern const char _binary_rgb_txt_start[];
extern const char _binary_rgb_txt_end[];

// map rgb.txt to palette map
struct palette_map {
    char name[100];
    int r;
    int g;
    int b;
} rgb_map[RGB_TXT_SIZE];

// set specific color as transparent (index 0)
char  trans_color_s[10] = { '#', 0 };
char* trans_color = NULL;
bool  trans_color_found = false;


bool is_transparent_color(const char* s)
{
    char color[100];
    char c;

    if (sscanf(s, "%c c %s", &c, &color) != 2) {
        fprintf(stderr, XPM_LABEL ": parse error at \"%s\": color ignored.\n", s);
        return false;
    }
    if (strcmp(color, "None") == 0) {
        if (trans_color != NULL) {
            fprintf(stderr, XPM_LABEL ": WARNING: both alpha channel and transparent color defined.\n");
        }
        return true;
    }
    if (trans_color != NULL && strcmp(color, trans_color) == 0) return true;
    return false;
}


void create_rgb_map()
{
    const char* ptr = _binary_rgb_txt_start;

    for (unsigned i = 0; i < RGB_TXT_SIZE; ++i) {
        uint8_t r, g, b;
        sscanf(ptr, "%hhu %hhu %hhu %[^\n]", &rgb_map[i].r, &rgb_map[i].g, &rgb_map[i].b,
        	rgb_map[i].name);
        while (*ptr != '\n') ptr++;
        ptr++;
    }
}


int scan_rgb(const char* color, int* r, int* g, int* b)
{
    for (int i = 0; i < RGB_TXT_SIZE; ++i) {
        if (strcmp(color, rgb_map[i].name) == 0) {
            *r = rgb_map[i].r;
            *g = rgb_map[i].g;
            *b = rgb_map[i].b;
            return 3;
        }
    }
    return 0;
}


void do_register_color(struct palette* palette, int hw_index, const char* color)
{
    int r = 0;
    int g = 0;
    int b = 0;

    // special case for alpha channel
    if (strcmp(color, "None") != 0) {
        if (sscanf(color, "#%02x%02x%02x", &r, &g, &b) == 3) {
            goto ok;
        } else if (!scan_rgb(color, &r, &g, &b)) {
            fprintf(stderr, XPM_LABEL ": parse error at \"%s\".\n", color);
            return;
        }
    }

ok:
    r &= 0xff;
    g &= 0xff;
    b &= 0xff;

    palette->ci = hw_index;
    const int component = (screen == P1) ? 31 : 7;
    palette->r = (uint8_t) ((double) r) * component / 0xff;
    palette->g = (uint8_t) ((double) g) * component / 0xff;
    palette->b = (uint8_t) ((double) b) * component / 0xff;

    palette->rr = r;
    palette->gg = g;
    palette->bb = b;
}


int register_color(struct palette* palette, int file_index, int8_t hw_index, int cpp, bool is_used)
{
    const char* s = XPM_DATA[file_index];
    char color[100];
    char c;

    if (sscanf(&s[cpp], "%c c %s", &c, &color) != 2) {
        fprintf(stderr, XPM_LABEL ": parse error at line %i: color ignored.\n", file_index);
        return -1;
    }

    if (palette->used && strcmp(palette->s, s) != 0) {
        fprintf(stderr, XPM_LABEL ": WARNING: palette color %i already used: color overwritten.\n", file_index);
    }

    do_register_color(palette, hw_index, color);
    // set extra color metadata
    palette->used = is_used;
    palette->s = s;

    fprintf(stderr, XPM_LABEL ": string '%.*s' registered as color #%i (%s) from array at \"%s\" (line %i)%s.\n",
            cpp, s, hw_index, color, s, file_index, is_used ? "" : " (not used)");

    return 0;
}


int8_t find_color(struct palette* palette, int colors, const char* const pos, int cpp)
{
    static int cached_color = 0;

    // last color scanned is cached and checked
    if (palette[cached_color].ci && strncmp(pos, palette[cached_color].s, cpp) == 0) {
        return palette[cached_color].ci;
    }

    for (int i = 0; i < colors; ++i) {
        // skip ignored color
        if (!palette[i].used) continue;
        if (strncmp(pos, palette[i].s, cpp) == 0) {
            cached_color = i;
            return palette[cached_color].ci;
        }
    }

    fprintf(stderr, XPM_LABEL ": color index '%.*s' unknown\n", cpp, pos);
    exit(INVALID_VALUE);
}


bool is_used_color(const char* const cs, int cpp, int colors, int width, int height)
{
    for (int y = 0; y < height; ++y) {
        const char* ptr = XPM_DATA[y + colors + 1];

        for (int x = 0; x < width; x++) {
            if (strncmp(ptr, cs, cpp) == 0) {
                return true;
            }
            ptr += cpp;
        }
    }

    return false;
}


char* strlower(char* const s)
{
    static char t[50];
    char *q = t;

    for (char* p = s; *p; ++p, ++q) *q = tolower(*p);
    return t;
}


char* strupper(char* const s)
{
    static char t[50];
    char *q = t;

    for (char* p = s; *p; ++p, ++q) *q = toupper(*p);
    return t;
}


void print_help()
{
    fprintf(stdout, "\
--screen5                Outputs a SCREEN5 compatible image (256 width max. with 15 colors).\n\
--v9990                  Outputs a P1 mode compatible image (V9990 graphics card).\n\
--keep-unused            Unused palette colors are still converted to output file.\n\
--contains-palette       Get palette colors and order from first disposable line of the image.\n\
--palette                Outputs palette too.\n\
--image                  Outputs image too.\n\
--both                   Outputs both palette and image.\n\
--header                 Outputs C-style header file.\n\
--file <filename>        Output binary output to file.\n\
--raw                    Outputs raw file (RAM memory dump without dimension data).\n\
--basic                  Outputs BASIC to stdout and respective VRAM COPY to file.\n\
--trans-color <RRGGBB>   Supply hex string RRGGBB as a transparent color replacement.\n\
--skip0                  Skip color 0 (transparent) in output files.\n");
}


int main(int argc, char **argv)
{
    int width;
    int height;
    int colors;
    int used_colors = 1;
    int unused_colors = 0;
    int skip0 = 0;
    char* filename = NULL;
    FILE* file = NULL;
    int cpp;
    bool keep_unused = false;           // when true, unused colors are preserved in the palette.
    bool contains_palette = false;      // when true, first line starts with a palette that is removed after processing.
    int change_to_black = -1;           // when greater than -1, color index is encoded as black
    bool skip_next = false;

    for (int i = 1; i < argc; ++i) {
        if (skip_next) {
            skip_next = false;
            continue;
        }
        // help message
        if (strcmp(argv[i], "--help") == 0) {
            print_help();
            exit(OK);
        }
        // screen type
        if (strcmp(argv[i], "--screen5") == 0) {
            screen = SCREEN5;
            continue;
        }
        if (strcmp(argv[i], "--v9990") == 0) {
            screen = P1;
            continue;
        }
        // data
        if (strcmp(argv[i], "--change-to-black") == 0) {
            if (argc == i + 1) {
                fprintf(stderr, XPM_LABEL ": expects number after \"--change-to-black\"\n");
                exit(PARAMETER_EXPECTED);
            }
            sscanf(argv[i + 1], "%d", &change_to_black);
            fprintf(stderr, "change-to-black = %i\n", change_to_black);
            if (change_to_black < 0 || change_to_black > 15) {
                fprintf(stderr, XPM_LABEL ": expects valid index after \"--convert-to-black\", not \"%s\"\n",
                        argv[i + 1]);
                exit(PARAMETER_TYPE);
            }
            skip_next = true;
            continue;
        }
        if (strcmp(argv[i], "--trans-color") == 0) {
            if (argc == i + 1) {
                fprintf(stderr, XPM_LABEL ": expects hex color  after \"--trans-color\"\n");
                exit(PARAMETER_EXPECTED);
            }
            sscanf(argv[i + 1], "%7s", &trans_color_s[1]);
            // convert to uppercase
            for (unsigned char* s = trans_color_s; *s != 0; ++s) {
                *s = toupper(*s);
            }
            // skip # marker and get parse the color value
            if (trans_color_s[1] == '#') trans_color = trans_color_s + 1;
            else trans_color = trans_color_s;
            fprintf(stderr, XPM_LABEL ": transparent color set to \"%s\"\n", trans_color);
            skip_next = true;
            continue;
        }
        if (strcmp(argv[i], "--keep-unused") == 0) {
            keep_unused = true;
            continue;
        }
        if (strcmp(argv[i], "--contains-palette") == 0) {
            contains_palette = true;
            continue;
        }
        if (strcmp(argv[i], "--palette") == 0) {
            data |= PALETTE;
            continue;
        }
        if (strcmp(argv[i], "--image") == 0) {
            data |= IMAGE;
            continue;
        }
        if (strcmp(argv[i], "--both") == 0) {
            data = BOTH;
            continue;
        }
        if (strcmp(argv[i], "--skip0") == 0) {
            skip0 = 1;
            continue;
        }
        if (strcmp(argv[i], "--file") == 0) {
            if (argc == i + 1) {
                fprintf(stderr, XPM_LABEL ": expects file name after \"--file\"\n");
                exit(PARAMETER_EXPECTED);
            }
            filename = argv[i + 1];
            skip_next = true;
            continue;
        }
        // mode
        if (mode == STDOUT && strcmp(argv[i], "--header") == 0) {
            mode = HEADER;
            continue;
        }
        if (strcmp(argv[i], "--raw") == 0) {
            mode = RAW;
            continue;
        }
        if (strcmp(argv[i], "--basic") == 0) {
            mode = BASIC;
            continue;
        }
        // fails on anything else
        fprintf(stderr, XPM_LABEL ": unknown option \"%s\"\n", argv[i]);
        exit(PARAMETER_TYPE);
    }
    if (data == NONE) {
        fprintf(stderr, XPM_LABEL ": No data type defined for output\n");
        exit(PARAMETER_EXPECTED);
    }
    if (data == BOTH && mode == RAW) {
        fprintf(stderr, XPM_LABEL ": raw mode expects a single output\n");
        exit(MODE_MISMATCH);
    }
    if (!(data & PALETTE) && keep_unused) {
        fprintf(stderr, XPM_LABEL ": \"--keep-unused\" expects palette output\n");
        exit(MODE_MISMATCH);
    }
    if (!(data & PALETTE) && change_to_black > -1) {
        fprintf(stderr, XPM_LABEL ": \"--change-to-black\" expects palette output\n");
        exit(MODE_MISMATCH);
    }
    if (mode == STDOUT && filename != NULL) {
        fprintf(stderr, XPM_LABEL ": file parameter expects \"--raw\" OR \"--basic\" mode only\n");
        exit(PARAMETER_TYPE);
    }
    if (mode == RAW && filename == NULL) {
        fprintf(stderr, XPM_LABEL ": RAW mode expects an output file.\n");
        exit(FILE_EXPECTED);
    }
    if (mode == BASIC && (data & IMAGE) && filename == NULL) {
        fprintf(stderr, XPM_LABEL ": BASIC mode expects an output file.\n");
        exit(FILE_EXPECTED);
    }
    // open specified file for output
    if (filename != NULL) {
        file = fopen(filename, "wb");
    }
    // scan first XPM line
    sscanf(XPM_DATA[0], "%d %d %d %d", &width, &height, &colors, &cpp);
    if (width & 1) {
        fprintf(stderr, XPM_LABEL ": expects even width\n");
        exit(INVALID_VALUE);
    }
    if (width > 256) {
        fprintf(stderr, XPM_LABEL ": expects maximum width of 256 pixels\n");
        exit(INVALID_VALUE);
    }

    create_rgb_map();

    struct palette* palette = alloca(sizeof(struct palette) * (colors + 1));
    memset(palette, 0, sizeof(struct palette) * (colors + 1));
    // reserve first element for transparent color
    palette_ptr = &palette[1];
    struct palette* ptr = palette_ptr;
    int replace_count = 0;

    // find colors first
    for (int file_index = 1; file_index < colors + 1; ++file_index) {
        char* s = XPM_DATA[file_index];
        if (is_transparent_color(s)) {
            if (!replace_count) {
                trans_color_found = true;
                if (register_color(&palette[0], file_index, 0, cpp, USED) == 0) {
                    fprintf(stderr, XPM_LABEL ": transparent color at line %i (\"%s\") found\n", file_index, s);
                } else {
                    fprintf(stderr, XPM_LABEL ": error at %s\n", s);
                }
            } else {
                fprintf(stderr, XPM_LABEL ": WARNING: transparent color repeated at position %i will be ignored.\n", file_index);
                if (keep_unused) register_color(ptr++, file_index, used_colors++, cpp, IGNORED);
            }
            replace_count++;
        }
        // process normal used colour
        else if (is_used_color(s, cpp, colors, width, height)) {
            register_color(ptr++, file_index, used_colors++, cpp, USED);
        }
        // process normal unused colour
        else if (keep_unused && used_colors < MAX_SCREEN5_COLORS) {
            register_color(ptr++, file_index, used_colors++, cpp, IGNORED);
            unused_colors++;
        } else {
            fprintf(stderr, XPM_LABEL ": WARNING: unused color at #%i (\"%s\") found\n", file_index, s);
            unused_colors++;
        }
    }
    // if trans_color is defined, overwrite color 0
    if (trans_color != NULL) do_register_color(&palette[0], 0, trans_color);

    if (!keep_unused) fprintf(stderr, XPM_LABEL ": %i color(s) discarted\n", unused_colors);

    if (trans_color != NULL && !trans_color_found) {
        fprintf(stderr, XPM_LABEL ": WARNING: transparent color %s not found in palette\n", trans_color);
    }
    fprintf(stderr, XPM_LABEL ": Number of colors: %i\n", used_colors);

    // check colors used
    if (screen == SCREEN5 && used_colors > MAX_SCREEN5_COLORS) {
        fprintf(stderr, XPM_LABEL ": expects max of 15 used colors, got %i (first color (0) is transparent)\n",
                used_colors);
        exit(INVALID_VALUE);
    }
    if (screen = P1 && used_colors > MAX_V9990_COLORS) {
        fprintf(stderr, XPM_LABEL ": expects max of 16 used colors\n");
        exit(INVALID_VALUE);
    }

    // converting defined color to black
    if (change_to_black > -1) {
        uint8_t i = change_to_black;
        fprintf(stderr, XPM_LABEL ": color #%i is now #0,#0,#0\n", i);
        palette[i].r = 0;
        palette[i].g = 0;
        palette[i].b = 0;
        palette[i].rr = 0;
        palette[i].gg = 0;
        palette[i].bb = 0;
    }

    if (mode == HEADER) {
        printf("#include <stdint.h>\n\n");

        if (data & PALETTE) {
            printf("extern const uint8_t %s_palette[%i];\n\n", strlower(XPM_LABEL), (used_colors - skip0) * 2);
        }

        if (data & IMAGE) {
            printf("#define " XPM_LABEL "_WIDTH %u\n", width);
            printf("#define " XPM_LABEL "_HEIGHT %u\n\n", height - (contains_palette ? 1 : 0));
            printf("extern const uint8_t %s_pattern[%u];\n\n", strlower(XPM_LABEL), width / 2 * height);
        }

        exit(OK);
    }

    else if (mode == STDOUT) {
        if (data & PALETTE) {
            printf("#include <stdint.h>\n\n");
            printf("const uint8_t %s_palette[%i] = {\n", strlower(XPM_LABEL), (used_colors - skip0) * 2);

            for (int8_t i = skip0; i < used_colors; i++) {
                // set YS bit on color 0 (transparent)
                printf("\t0x%02X,0x%02X, /* %02d: 0x%02X, 0x%02X, 0x%02X %s*/\n",
                       palette[i].r * 16 + palette[i].b, palette[i].g,
                       palette[i].ci, palette[i].rr, palette[i].gg, palette[i].bb,
                       palette[i].used ? "" : "(not used) ");
            }
            printf("};\n\n");
        }
    }

    else if (mode == RAW) {
        if (data == PALETTE) {
            for (int8_t i = skip0; i < used_colors; ++i) {
                // set YS bit on color 0 (transparent)
                uint8_t ys = (screen == P1 && i == 0) ? 128 : 0;
                char data[3] = { ys | palette[i].r, palette[i].g, palette[i].b, };
                fwrite(data, 1, 3, file);
            }
            fclose(file);
            exit(OK);
        }
    }

    else if (mode == BASIC) {
        int lineno = 0;
        printf("%i SCREEN 5\r\n", lineno += 10);
        if (data & PALETTE) {
            for (int8_t i = skip0; i < used_colors; ++i) {
                printf("%i COLOR=(%i,%i,%i,%i)\r\n", lineno += 10, i, palette[i].r, palette[i].g, palette[i].b);
            }
        }
        if (data & IMAGE) {
            printf("%i COPY \"%s\" TO (0,0),0\r\n", lineno += 10, strupper(filename));
            printf("%i IF INKEY$=\"\" GOTO %i\r\n", lineno += 10, lineno);
        }
    }

    if (!(data & IMAGE)) {
        // no image output, only palette.
        exit(0);
    }

    if (mode == STDOUT) {
        if (!(data & PALETTE)) printf("#include <stdint.h>\n\n");
        printf("const uint8_t %s_pattern[%u] = {\n\t", strlower(XPM_LABEL), width / 2 * height);
    }

    // output pattern
    unsigned int pos = 0;

    if (mode == BASIC) {
        uint8_t header[4] = { width & 0xff, width >> 8, (height - (contains_palette ? 1 : 0)) & 0xff, (height - (contains_palette ? 1 : 0)) >> 8 };
        fwrite(header, 1, 4, file);
    }

    for (int y = contains_palette ? 1 : 0; y < height ; ++y) {
        const char* ptr = XPM_DATA[y + colors + 1];
        uint8_t pixel1, pixel2;

        for (int x = 0; x < width; x += 2) {
            pixel1 = find_color(palette, used_colors, ptr, cpp);
            ptr += cpp;

            pixel2 = find_color(palette, used_colors, ptr, cpp);
            ptr += cpp;

            switch (mode) {
                case STDOUT:
                    if (pos > 0 && pos % 24 == 0) printf("\n\t");
                    printf("0x%02X,", (pixel1 << 4) | pixel2);
                    break;

                case BASIC: case RAW: {
                    uint8_t data[1] = { (pixel1 << 4) | pixel2 };
                    fwrite(data, 1, 1, file);
                    break;
            }}

            pos += 2;
        }
    }

    if (mode == STDOUT) {
        if (pos > 0) printf("\n");
        printf("};\n\n");
    }

    if (mode != STDOUT) {
        fclose(file);
    }
}

