#ifndef SSB_CANVAS_H
#define SSB_CANVAS_H

#include <stdbool.h>

enum color {
    black,
    red,
    green,
    yellow,
    blue,
    magenta,
    cyan,
    white
};

struct cell {
    unsigned long code_point : 21;
    unsigned long italic : 1;
    unsigned long underline : 1;
    unsigned long blink : 1;
    enum color foreground : 3;
    enum color background : 3;
    unsigned long bold : 1;
    unsigned long dirty : 1;
};

// Zero out a cell
#define CANVAS_CELL_CLEAR(cell) do { (cell).code_point = 0; (cell).italic = false; \
        (cell).underline = false; (cell).blink = false; (cell).foreground = black; \
        (cell).background = black; (cell).bold = false; (cell).dirty = false; } while (0)

// Check if two cells are equivalent besides their dirty flags
#define CANVAS_CELL_EQ(a, b) ((a).code_point == (b).code_point && (a).italic == (b).italic && \
    (a).underline == (b).underline && (a).blink == (b).blink && (a).bold == (b).bold && \
    (a).foreground == (b).foreground && (a).background == (b).background && (a).bold == (b).bold)

struct canvas {
    struct cell *canvas;
    unsigned w, h;

    bool window;
    struct canvas *window_canvas;
    unsigned window_x1, window_y1, window_x2, window_y2;

    struct cell style;

    size_t flush_index;
    struct cell flush_state;
    size_t flush_encode_offset;
    size_t flush_last_index;
};

void canvas_init(struct canvas *canvas, unsigned w, unsigned h);

void canvas_free(struct canvas *canvas);

void canvas_resize(struct canvas *canvas, unsigned w, unsigned h);

void canvas_erase(struct canvas *canvas);

bool canvas_flush(struct canvas *canvas, char *buf, size_t len, size_t *len_written);

bool canvas_forced_flush(struct canvas *canvas, char *buf, size_t len, size_t *len_written);

void canvas_write_all_utf32(struct canvas *canvas, unsigned long *buf, size_t len);

void canvas_put(struct canvas *canvas, unsigned x, unsigned y, unsigned long c);

void canvas_create_window(struct canvas *canvas, unsigned x1, unsigned y1,
                          unsigned x2, unsigned y2, struct canvas *window);

void canvas_rect(struct canvas *canvas, unsigned x, unsigned y, unsigned w, unsigned h,
                 unsigned long symbol);

void canvas_reset(struct canvas *canvas);

void canvas_italic(struct canvas *canvas, bool state);

void canvas_underline(struct canvas *canvas, bool state);

void canvas_blink(struct canvas *canvas, bool state);

void canvas_bold(struct canvas *canvas, bool state);

void canvas_foreground(struct canvas *canvas, enum color color);

void canvas_background(struct canvas *canvas, enum color color);

#define CANVAS_PUT(x, y, sym) do { canvas_put(CANVAS, (x), (y), (sym)); } while (0)

#define CANVAS_ITALIC(x) do { \
        canvas_italic(CANVAS, true); do { x } while (0); canvas_italic(CANVAS, false); \
    } while(0)


#endif //SSB_CANVAS_H
