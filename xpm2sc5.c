#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <alloca.h>
#include <ctype.h>

#ifdef XPM_FILE
# include XPM_FILE
#else
# error XPM_FILE not defined
#endif

#define MAX_COLORS 16

struct palette {
    int8_t ci;
    uint8_t r;
    uint8_t rr;
    uint8_t g;
    uint8_t gg;
    uint8_t b;
    uint8_t bb;
    const char* s;
};


void register_color(struct palette* palette, int index, int8_t hw_color, int cpp)
{
    unsigned int rr, gg, bb;
    const char* s = XPM_DATA[index + 1];

    sscanf(&s[cpp], "\tc #%02x%02x%02x", &rr, &gg, &bb);
    rr = rr & 0xff;
    gg = gg & 0xff;
    bb = bb & 0xff;

    palette[index].ci = hw_color;
    palette[index].r = (uint8_t) ((double) rr) * 7 / 0xff;
    palette[index].g = (uint8_t) ((double) gg) * 7 / 0xff;
    palette[index].b = (uint8_t) ((double) bb) * 7 / 0xff;
    palette[index].s = s;

    palette[index].rr = rr;
    palette[index].gg = gg;
    palette[index].bb = bb;
}


int8_t find_color(struct palette* palette, int colors, const char* const pos, int cpp)
{
    static int8_t cached_color = 0;

    if (palette[cached_color].ci && strncmp(pos, palette[cached_color].s, cpp) == 0) {
        return palette[cached_color].ci;
    }

    for (int8_t i = 0; i < colors; ++i) {
        if (palette[i].ci && strncmp(pos, palette[i].s, cpp) == 0) {
            cached_color = i;
            return palette[cached_color].ci;
        }
    }

    fprintf(stderr, XPM_LABEL ": color index '%.*s' unknown\n", cpp, pos);
    exit(-5);
}


bool is_used_color(const char* const cs, int cpp, int colors, int width, int height)
{
    for (int y = 0; y < height ; ++y) {
        const char* pos = XPM_DATA[y + colors + 1];

        for (int x = 0; x < width; x++) {
            if (strncmp(pos, cs, cpp) == 0) {
                return true;
            }
            pos += cpp;
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


enum MODE {
    STDOUT = 0,                 // send C-style data to standard output
    HEADER,                     // header mode has no pattern and palette data
    RAW,                        // raw mode prints data byte directly to binary file
};


int main(int argc, char **argv)
{
    int width;
    int height;
    int colors;
    int used_colors;
    enum MODE mode = STDOUT;
    FILE *file = NULL;
    int cpp;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--header") == 0) {
            mode = HEADER;
	}
	if (strcmp(argv[i], "--raw") == 0) {
            mode = RAW;
            if (argc == i + 1) {
                fprintf(stderr, XPM_LABEL ": expects raw filename\n");
                exit(-1);
            }
	    file = fopen(argv[i+1], "wb");
	}
    }

    sscanf(XPM_DATA[0], "%d %d %d %d", &width, &height, &colors, &cpp);
    if (width & 1) {
        fprintf(stderr, XPM_LABEL ": expects even width\n");
        exit(-2);
    }
    if (width > 256) {
        fprintf(stderr, XPM_LABEL ": expects maximum width of 256 pixels\n");
        exit(-3);
    }

    struct palette* palette = alloca(sizeof(struct palette) * colors);
    memset(palette, 0, sizeof(struct palette) * colors);
    used_colors = 0;

    /* find used colors first */
    for (int c = 0; c < colors; ++c) {
        char* s = XPM_DATA[c + 1];
        fprintf(stderr, XPM_LABEL ": color [%s]\n", s);
        if (is_used_color(s, cpp, colors, width, height) != false) {
            ++used_colors;
            fprintf(stderr, XPM_LABEL ": pixel '%.*s' registered as color #%i from table at %i'\n",
                    cpp, s, used_colors, c + 1);
            register_color(palette, c, used_colors, cpp);
        }
    }
    if (used_colors >= MAX_COLORS) {
        fprintf(stderr, XPM_LABEL ": expects max of 15 used colors\n");
        exit(-4);
    }

    switch (mode) {

    case HEADER:
        printf("#include <stdint.h>\n\n");
        printf("#define " XPM_LABEL "_WIDTH %u\n", width);
        printf("#define " XPM_LABEL "_HEIGHT %u\n\n", height);
        printf("extern const uint8_t %s_palette[];\n\n", strlower(XPM_LABEL));
	break;

    case STDOUT:
        printf("static const uint8_t %s_palette[] = {\n", strlower(XPM_LABEL));

        for (int8_t i = 0; i <= colors; ++i) {
            if (palette[i].ci) {
                printf("\t%2i, %u,%u,%u, /* 0x%02X, 0x%02X, 0x%02X */\n",
                       palette[i].ci,
                       palette[i].r, palette[i].g, palette[i].b,
                       palette[i].rr, palette[i].gg, palette[i].bb);
            }
        }
        printf("};\n\n");
	break;

    default:

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
        for (int y = 0; y < height ; ++y) {
            const char* line = XPM_DATA[y + colors + 1];

            for (int x = 0; x < width; x += 2) {
                uint8_t pixel1, pixel2;

                pixel1 = find_color(palette, colors, line, cpp);
                line += cpp;

                pixel2 = find_color(palette, colors, line, cpp);
                line += cpp;

                switch (mode) {

                case STDOUT:
                     if (pos > 0 && pos % 24 == 0) printf("\n\t");
                     printf("0x%02X,", (pixel1 << 4) | pixel2);
                     break;

		case RAW: {
                     char data[1] = { (pixel1 << 4) | pixel2 };
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
        pos += height * width;
    }

    switch (mode) {

    case HEADER:
        printf("#define " XPM_LABEL "_SIZE %u\n", pos / 2);
	break;

    case RAW:
        fclose(file);
	break;

    default:
    }
}

