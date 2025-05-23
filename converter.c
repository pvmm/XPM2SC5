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
// error codes
#define OK                 0
#define PARAMETER_EXPECTED 1
#define PARAMETER_TYPE     2
#define MODE_MISMATCH      3
#define INVALID_VALUE      4


enum DATA_TYPE {
    BOTH = 0,
    IMAGE,                      // output image data only
    PALETTE,                    // output palette data only
} data = BOTH;

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
    bool ignored;
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

// set specific color as transparent (index 0)
char  trans_color_s[10] = { '#', 0 };
char* trans_color = NULL;
bool  trans_color_found = false;
bool  has_trans_color = false;


void register_color(struct palette* palette, int file_index, int8_t hw_index, int cpp, bool ignored)
{
    unsigned int rr, gg, bb;
    const char* s = XPM_DATA[file_index];
    int component = (screen == P1) ? 31 : 7;

    sscanf(&s[cpp], "\tc #%02x%02x%02x", &rr, &gg, &bb);
    rr = rr & 0xff;
    gg = gg & 0xff;
    bb = bb & 0xff;

    palette->ignored = ignored;
    palette->ci = hw_index;
    palette->r = (uint8_t) ((double) rr) * component / 0xff;
    palette->g = (uint8_t) ((double) gg) * component / 0xff;
    palette->b = (uint8_t) ((double) bb) * component / 0xff;
    palette->s = s;

    palette->rr = rr;
    palette->gg = gg;
    palette->bb = bb;

    fprintf(stderr, XPM_LABEL ": string '%.*s' registered as color #%i from table at %i%s.\n",
            cpp, s, hw_index, file_index,
            ignored ? " (not used)" : "");
}


int8_t find_color(struct palette* palette, int colors, const char* const pos, int cpp)
{
    static int cached_color = 0;

    if (palette[cached_color].ci && strncmp(pos, palette[cached_color].s, cpp) == 0) {
        return palette[cached_color].ci;
    }

    for (int i = has_trans_color ? 0 : 1; i < has_trans_color ? colors + 1 : colors; ++i) {
        // skip ignored color
        if (palette[i].ignored) continue;
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


int main(int argc, char **argv)
{
    int width;
    int height;
    int colors;
    int used_colors;
    FILE *file = NULL;
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
            if (data == IMAGE) {
                fprintf(stderr, XPM_LABEL ": \"--change-to-black\" expects palette output\n");
                exit(MODE_MISMATCH);
            }
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
            if (trans_color_s[1] == '#') trans_color = trans_color_s + 1;
            else trans_color = trans_color_s;
            fprintf(stderr, XPM_LABEL ": transparent color = %s\n", trans_color);
            skip_next = true;
            continue;
        }
        if (strcmp(argv[i], "--keep-unused") == 0) {
            if (data == IMAGE) {
                fprintf(stderr, XPM_LABEL ": \"--keep-unused\" expects palette output\n");
                exit(MODE_MISMATCH);
            }
            keep_unused = true;
            continue;
        }
        if (strcmp(argv[i], "--contains-palette") == 0) {
            contains_palette = true;
            continue;
        }
        if (strcmp(argv[i], "--palette") == 0) {
            data = PALETTE;
            continue;
        }
        if (strcmp(argv[i], "--image") == 0) {
            data = IMAGE;
            continue;
        }
        // mode
        if (mode == STDOUT && strcmp(argv[i], "--header") == 0) {
            mode = HEADER;
            continue;
        }
        if ((mode == STDOUT || mode == BASIC) && strcmp(argv[i], "--raw") == 0) {
            if (mode == STDOUT) mode = RAW;
            if (argc == i + 1) {
                fprintf(stderr, XPM_LABEL ": expects raw filename after \"--raw\"\n");
                exit(PARAMETER_EXPECTED);
            }
            file = fopen(argv[i + 1], "wb");
            skip_next = true;
            continue;
        }
        if (strcmp(argv[i], "--basic") == 0) {
            mode = BASIC;
            continue;
        }
        if (strcmp(argv[i], "--") == 0) {
            fprintf(stderr, XPM_LABEL ": unknown option \"%s\"\n", argv[i]);
            exit(PARAMETER_TYPE);
        }
    }

    sscanf(XPM_DATA[0], "%d %d %d %d", &width, &height, &colors, &cpp);
    if (width & 1) {
        fprintf(stderr, XPM_LABEL ": expects even width\n");
        exit(INVALID_VALUE);
    }
    if (width > 256) {
        fprintf(stderr, XPM_LABEL ": expects maximum width of 256 pixels\n");
        exit(INVALID_VALUE);
    }

    struct palette* palette = alloca(sizeof(struct palette) * (colors + 1));
    memset(palette, 0, sizeof(struct palette) * (colors + 1));
    // reserve first element for transparent color
    palette_ptr = &palette[1];
    struct palette* ptr = palette_ptr;
    int replace_count = 0;
    used_colors = 1;

    /* find colors first */
    for (int file_index = 1; file_index < colors + 1; ++file_index) {
        char* s = XPM_DATA[file_index];
        char* rgb = s + strlen(s) - 7; // #<RRGGBB> at the end of the string
        // process transparent colour
        if (trans_color != NULL && strcmp(rgb, trans_color) == 0) {
            if (!replace_count) {
                fprintf(stderr, XPM_LABEL ": transparent color #%i (%s) found\n", file_index, rgb);
                register_color(&palette[0], file_index, 0, cpp, false);
                trans_color_found = true;
            } else {
                fprintf(stderr, XPM_LABEL ": WARNING: transparent color repeated at position %i will be ignored.\n", file_index);
                if (keep_unused) register_color(ptr++, file_index, used_colors++, cpp, true);
            }
            replace_count++;
        }
        // process normal used colour
        else if (is_used_color(s, cpp, colors, width, height)) {
            register_color(ptr++, file_index, used_colors++, cpp, false);
        }
        // process normal unused colour
        else if (keep_unused && used_colors < MAX_SCREEN5_COLORS) {
            register_color(ptr++, file_index, used_colors++, cpp, true);
        }
    }
    has_trans_color = trans_color != NULL && trans_color_found;
    if (!keep_unused) fprintf(stderr, XPM_LABEL ": %i color(s) discarted\n", colors - used_colors);
    used_colors -= has_trans_color ? 1 : 0;

    if (trans_color != NULL && !trans_color_found) {
        fprintf(stderr, XPM_LABEL ": WARNING: transparent color #%s not found in palette\n", trans_color);
    }
    fprintf(stderr, XPM_LABEL ": number of colors = %i\n", used_colors);

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

        if (data == IMAGE || data == BOTH) {
            printf("#define " XPM_LABEL "_WIDTH %u\n", width);
            printf("#define " XPM_LABEL "_HEIGHT %u\n\n", height - (contains_palette ? 1 : 0));
        }

        if (data == PALETTE || data == BOTH) {
            printf("extern const uint8_t %s_palette[%i];\n\n", strlower(XPM_LABEL), used_colors * 2);
        }
    }

    else if (mode == STDOUT) {
        if (data == PALETTE || data == BOTH) {
            printf("#include <stdint.h>\n\n");
            printf("const uint8_t %s_palette[%i] = {\n", strlower(XPM_LABEL), used_colors * 2);

            for (int8_t i = 0; i < used_colors; i++) {
                // set YS bit on color 0 (transparent)
                printf("\t0x%02X,0x%02X, /* %02d: 0x%02X, 0x%02X, 0x%02X %s*/\n",
                       palette_ptr[i].r * 16 + palette_ptr[i].b, palette_ptr[i].g,
                       palette_ptr[i].ci, palette_ptr[i].rr, palette_ptr[i].gg, palette_ptr[i].bb,
                       palette_ptr[i].ignored ? "(not used) " : "");
            }
            printf("};\n\n");
        }
    }

    else if (mode == RAW) {
        if (data == PALETTE) {
            for (int8_t i = 0; i < used_colors; ++i) {
                // set YS bit on color 0 (transparent)
                uint8_t ys = (screen == P1 && i == 0) ? 128 : 0;
                char data[3] = { ys | palette_ptr[i].r, palette_ptr[i].g, palette_ptr[i].b, };
                fwrite(data, 1, 3, file);
            }
            fclose(file);
            exit(OK);
        }
    }

    else if (mode == BASIC) {
        if (data == PALETTE) {
            printf("10 SCREEN 5\r\n");
            for (int8_t i = 0; i < used_colors; ++i) {
                printf("%i COLOR=(%i,%i,%i,%i)\r\n", (i + 1) * 10, i, palette[i].r, palette[i].g, palette[i].b);
            }
            int index = used_colors * 10;
            printf("%i COPY \"IMAGE.BIN\" TO (0,0),0\r\n", index += 10);
            printf("%i IF INKEY$=\"\" GOTO %i\r\n", index += 10, index);
            printf("%i COPY (0,0)-(254,212),0 TO (1,0),0,XOR\r\n", index += 10);
            printf("%i IF INKEY$=\"\" GOTO %i\r\n", index += 10, index);
            exit(OK);
        }
    }


    switch (mode) {
    case HEADER:
        printf("extern const uint8_t %s_pattern[%u];\n\n", strlower(XPM_LABEL), width / 2 * height);
        break;

    case STDOUT:
        printf("const uint8_t %s_pattern[%u] = {\n\t", strlower(XPM_LABEL), width / 2 * height);
        break;

    default:
    }

    unsigned int pos = 0;

    if (mode != HEADER) {
        if (mode == BASIC) {
            uint8_t header[4] = { width & 0xff, width >> 8, (height - (contains_palette ? 1 : 0)) & 0xff, (height - (contains_palette ? 1 : 0)) >> 8 };
            assert(file != NULL);
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
    } else {
        pos += (height - (contains_palette ? 1 : 0)) * width;
    }

    switch (mode) {
    case HEADER:
        printf("#define " XPM_LABEL "_SIZE %u\n", pos / 2);
        break;

    case BASIC: case RAW:
        fclose(file);
        break;

    default:
    }
}

