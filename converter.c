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
    int8_t ci;                  // color index
    uint8_t r;                  // 5bit red (P1) / 3bit red (SC5)
    uint8_t rr;                 // original red
    uint8_t g;                  // 5bit green (P1) / 3bit green (SC5)
    uint8_t gg;                 // original green
    uint8_t b;                  // 5bit blue (P1) / 3bit blue (SC5)
    uint8_t bb;                 // original blue
    const char* s;              // encoded pixels
};

// set specific color as transparent (index 0)
char* trans_color = NULL;


void register_color(struct palette* palette, int file_index, int8_t hw_index, int cpp)
{
    unsigned int rr, gg, bb;
    const char* s = XPM_DATA[file_index];
    int component = (screen == P1) ? 31 : 7;

    sscanf(&s[cpp], "\tc #%02x%02x%02x", &rr, &gg, &bb);
    rr = rr & 0xff;
    gg = gg & 0xff;
    bb = bb & 0xff;

    palette->ci = hw_index;
    palette->r = (uint8_t) ((double) rr) * component / 0xff;
    palette->g = (uint8_t) ((double) gg) * component / 0xff;
    palette->b = (uint8_t) ((double) bb) * component / 0xff;
    palette->s = s;

    palette->rr = rr;
    palette->gg = gg;
    palette->bb = bb;
}


int8_t find_color(struct palette* palette, int colors, const char* const pos, int cpp)
{
    static int cached_color = 0;

    if (palette[cached_color].ci && strncmp(pos, palette[cached_color].s, cpp) == 0) {
        return palette[cached_color].ci;
    }

    fprintf(stderr, XPM_LABEL ": used colors = %i\n", colors);
    for (int i = (trans_color != NULL ? 0 : 1); i < colors; ++i) {
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
                fprintf(stderr, XPM_LABEL ": expects number after \"--trans-color\"\n");
                exit(PARAMETER_EXPECTED);
            }
            sscanf(argv[i + 1], "%as", &trans_color);
            if (trans_color[0] == '#') ++trans_color;
            fprintf(stderr, "trans color = %s\n", trans_color);
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

    struct palette* palette = alloca(sizeof(struct palette) * colors);
    memset(palette, 0, sizeof(struct palette) * colors);
    struct palette* ptr = &palette[1];
    int replace_count = 0;
    used_colors = trans_color != NULL ? 0 : 1;

    /* find colors first */
    for (int c = 1; c < colors + 1; ++c) {
        char* s = XPM_DATA[c];
        char* rgb = s + cpp + 4;
        //fprintf(stderr, XPM_LABEL ": color [%s] = #%i\n", rgb, c);
        if (trans_color != NULL && strcmp(rgb, trans_color) == 0) {
            if (!replace_count) {
                fprintf(stderr, XPM_LABEL ": trans_color [%i] found\n", c);
                register_color(&palette[0], c, 0, cpp);
            } else {
                fprintf(stderr, XPM_LABEL ": WARNING: trans_color found at %i\n", c);
            }
            replace_count++;
        }
        else if (keep_unused && used_colors < MAX_SCREEN5_COLORS) {
            register_color(ptr++, c, used_colors++, cpp);
        } else {
            if (is_used_color(s, cpp, colors, width, height) != false) {
                register_color(ptr++, c, used_colors++, cpp);
                fprintf(stderr, XPM_LABEL ": string '%.*s' registered as color #%i from table at %i'\n",
                        cpp, s, used_colors, c);
            } else {
                fprintf(stderr, XPM_LABEL ": string '%.*s' discarded because not used\n", cpp, s);
            }
        }
    }
    fprintf(stderr, XPM_LABEL ": %i color(s) discarted\n", colors + (trans_color != NULL ? 0 : 1) - used_colors);
    fprintf(stderr, XPM_LABEL ": used colors = %i\n", used_colors - (trans_color != NULL ? 0 : 1));
    fprintf(stderr, XPM_LABEL ": trans color is %s\n", trans_color == NULL ? "(nil)" : "not (nil)");

    if (screen == SCREEN5 && used_colors > MAX_SCREEN5_COLORS) {
        fprintf(stderr, XPM_LABEL ": expects max of 15 used colors, got %i (first color (0) is transparent)\n", used_colors);
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
            printf("extern const uint8_t %s_palette[];\n\n", strlower(XPM_LABEL));
        }
    }

    else if (mode == STDOUT) {
        if (data == PALETTE || data == BOTH) {
            printf("static const uint8_t %s_palette[] = {\n", strlower(XPM_LABEL));

            for (int8_t i = 0; i < used_colors; ++i) {
                // set YS bit on color 0 (transparent)
                uint8_t ys = (screen == P1 && i == 0) ? 128 : 0;
                if (palette[i].ci) {
                    printf("\t%2i, %u,%u,%u, /* 0x%02X, 0x%02X, 0x%02X */\n",
                           palette[i].ci,
                           ys | palette[i].r, palette[i].g, palette[i].b,
                           palette[i].rr, palette[i].gg, palette[i].bb);
                }
            }

            printf("};\n\n");
        }
    }

    else if (mode == RAW) {
        if (data == PALETTE) {
            for (int8_t i = 0; i < used_colors; ++i) {
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
        if (data == PALETTE) {
            printf("10 SCREEN 5\r\n");
            for (int8_t i = 1; i < used_colors; ++i) {
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
        printf("extern const uint8_t %s_data[];\n\n", strlower(XPM_LABEL));
        break;

    case STDOUT:
        printf("static const uint8_t %s_data[] = {\n\t", strlower(XPM_LABEL));
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

