#include <winsock2.h>
#include <stdio.h>
#include "terminal.h"
#include "session.h"
#include "config.h"


void terminal_send(struct session *sess, char *buf, int len) {
#if DEBUG_OUTGOING
    printf("#%d <- ", sess->id);
    for (int i = 0; i < len; ++i) {
        printf("%02x ", (unsigned char) buf[i]);
    }
    printf("\n");
#endif

    send(sess->client_sock, buf, len, 0);
}

void terminal_recv(struct session *sess, char *buf, int len) {
#if DEBUG_INCOMING
    printf("#%d -> ", sess->id);
    for (int i = 0; i < len; ++i) {
        printf("%02x ", (unsigned char) buf[i]);
    }
    printf("\n");
#endif

    terminal_parse(sess, buf, len);
}

#define STATE sess->state.terminal_state

#define INPUT_BUF STATE.input_buf
#define INPUT_BUF_READ STATE.input_buf_read
#define INPUT_BUF_WRITE STATE.input_buf_write

#define INPUT_BUF_MASK(val) ((val) & (INPUT_BUF_LEN - 1))
#define INPUT_BUF_SIZE (STATE.input_buf_write - STATE.input_buf_read)
#define INPUT_BUF_EMPTY (STATE.input_buf_read == STATE.input_buf_write)
#define INPUT_BUF_FULL (INPUT_BUF_SIZE == INPUT_BUF_LEN)

void terminal_parse(struct session *sess, char *buf, int len) {
    // Check if there's room in the input buffer, otherwise we're discarding the incoming data!
    if (INPUT_BUF_FULL) {
        fprintf(stderr, "%d: input buffer full\n", sess->id);
        return;
    }

    // Naively feed data to the input buffer
    //TODO don't do this
    for (int i = 0; i < len && !INPUT_BUF_FULL; ++i) {
        INPUT_BUF[INPUT_BUF_MASK(INPUT_BUF_WRITE++)] = buf[i];
    }
}

unsigned terminal_available(struct session *sess) {
    return INPUT_BUF_SIZE;
}

char terminal_read(struct session *sess) {
    return INPUT_BUF[INPUT_BUF_MASK(INPUT_BUF_READ++)];
}

char terminal_peek(struct session *sess, unsigned n) {
    return INPUT_BUF[INPUT_BUF_MASK(INPUT_BUF_READ + n)];
}

void terminal_write(struct session *sess, char *buf) {
    terminal_send(sess, buf, (int) strlen(buf));
}

#define ESC "\e"
#define CSI ESC "["

void terminal_move(struct session *sess, unsigned x, unsigned y) {
    char buf[20];
    int len = snprintf(buf, 20, CSI "%d;%dH", y + 1, x + 1);
    terminal_send(sess, buf, len);
}

void terminal_clear(struct session *sess) {
    terminal_write(sess, CSI "2J");
}

void terminal_cursor(struct session *sess, bool state) {
    terminal_write(sess, state ? CSI "?25h" : CSI "?25l");
}

void terminal_rect(struct session *sess, unsigned x, unsigned y, unsigned w, unsigned h,
                   char symbol) {
    char *buf = malloc(sizeof(char) * w);
    memset(buf, symbol, w);

    terminal_move(sess, x, y);
    terminal_send(sess, buf, w);
    for (unsigned row = y; row < y + h - 1; ++row) {
        terminal_move(sess, x, row);
        terminal_send(sess, buf, 1);
        terminal_move(sess, x + w - 1, row);
        terminal_send(sess, buf, 1);
    }
    terminal_move(sess, x, y + h - 1);
    terminal_send(sess, buf, w);

    free(buf);
}

void terminal_write_at(struct session *sess, unsigned x, unsigned y, char *buf) {
    terminal_move(sess, x, y);
    terminal_write(sess, buf);
}

void terminal_reset(struct session *sess) {
    terminal_write(sess, CSI "m");
}

void terminal_bold(struct session *sess, bool state) {
    terminal_write(sess, state ? CSI "1m" : CSI "21m");
}

void terminal_underline(struct session *sess, bool state) {
    terminal_write(sess, state ? CSI "4m" : CSI "24m");
}

void terminal_inverse(struct session *sess, bool state) {
    terminal_write(sess, state ? CSI "7m" : CSI "27m");
}

void terminal_init(struct terminal_state *state) {
    state->input_buf_read = state->input_buf_write = 0;
}

void terminal_read_menu_input(struct session *sess, struct menu_input *input) {
    input->esc = input->enter = input->space = false;
    input->x = input->y = 0;

    int available;
    while ((available = terminal_available(sess)) > 0) {
        char val0 = terminal_read(sess), val1 = terminal_peek(sess, 0),
                val2 = terminal_peek(sess, 1);
        switch (available) {
        default:
        case 3:
            if (val0 == '\x1b' && val1 == '[' && val2 == 'A') {
                input->y = 1;
                goto consume_2;
            }
            if (val0 == '\x1b' && val1 == '[' && val2 == 'B') {
                input->y = -1;
                goto consume_2;
            }
            if (val0 == '\x1b' && val1 == '[' && val2 == 'C') {
                input->x = 1;
                goto consume_2;
            }
            if (val0 == '\x1b' && val1 == '[' && val2 == 'D') {
                input->x = -1;
                goto consume_2;
            }

        case 2:
            if (val0 == '\x0d' && (val1 == '\x00' || val1 == '\x0a')) {
                input->enter = true;
                goto consume_1;
            }

        case 1:
            // Given that there's no telling ESC followed by a letter from its
            // ALT counterpart, we'll just hope that no other characters were
            // received when ESC was explicitly pressed
            if (val0 == '\x1b' && available == 1) {
                input->esc = true;
            }
            if (val0 == '\x20') {
                input->space = true;
            }
        }
    }

    return;

    // Get rid of the extra consumed bytes that were only peeked at
consume_2:
    terminal_read(sess);
consume_1:
    terminal_read(sess);
}
