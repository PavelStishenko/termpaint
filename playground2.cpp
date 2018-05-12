#include <unistd.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>

#include <vector>
#include <string>

typedef bool _Bool;

#include "termpaint.h"
#include "termpaintx.h"
#include "termpaint_input.h"

struct DisplayEvent {
    std::string raw;
    std::string eventString;
};

std::vector<DisplayEvent> ring;
std::string peek_buffer;
termpaint_terminal *terminal;
termpaint_surface *surface;
time_t last_q;
bool quit;
std::string terminal_info;


template <typename X>
unsigned char u8(X); // intentionally undefined

template <>
unsigned char u8(char ch) {
    return ch;
}

_Bool raw_filter(void *user_data, const char *data, unsigned length, _Bool overflow) {
    (void)user_data;
    std::string event { data, length };
    ring.emplace_back();
    ring.back().raw = event;

    if (event == "q") {
        time_t now = time(0);
        if (last_q && (now - last_q) == 3) {
            quit = true;
        }
        last_q = now;
    } else {
        last_q = 0;
    }
    return 0;
}

void event_handler(void *user_data, termpaint_event *event) {
    (void)user_data;
    std::string pretty;

    if (event->type == 0) {
        pretty = "unknown";
    } else if (event->type == TERMPAINT_EV_KEY) {
        pretty = "K: ";
        if ((event->modifier & ~(TERMPAINT_MOD_SHIFT|TERMPAINT_MOD_ALT|TERMPAINT_MOD_CTRL)) == 0) {
            pretty += (event->modifier & TERMPAINT_MOD_SHIFT) ? "S" : " ";
            pretty += (event->modifier & TERMPAINT_MOD_ALT) ? "A" : " ";
            pretty += (event->modifier & TERMPAINT_MOD_CTRL) ? "C" : " ";
        } else {
            char buf[100];
            snprintf(buf, 100, "%03d", event->modifier);
            pretty += buf;
        }
        pretty += " ";
        pretty += event->atom_or_string;
    } else if (event->type == TERMPAINT_EV_CHAR) {
        pretty = "C: ";
        pretty += (event->modifier & TERMPAINT_MOD_SHIFT) ? "S" : " ";
        pretty += (event->modifier & TERMPAINT_MOD_ALT) ? "A" : " ";
        pretty += (event->modifier & TERMPAINT_MOD_CTRL) ? "C" : " ";
        pretty += " ";
        pretty += std::string { event->atom_or_string, event->length };
    } else if (event->type == TERMPAINT_EV_CURSOR_POSITION) {
        pretty = "Cursor position report: x=" + std::to_string(event->x) + " y=" + std::to_string(event->y);
    } else {
        pretty = "Other event no. " + std::to_string(event->type);
    }

    if (ring.empty() || ring.back().eventString.size()) {
        ring.emplace_back();
    }
    ring.back().eventString = pretty;
}

void display_esc(int x, int y, const std::string &data) {
    for (unsigned i = 0; i < data.length(); i++) {
        if (u8(data[i]) == '\e') {
            termpaint_surface_write_with_colors(surface, x, y, "^[", 0xffffff, 0x7f0000);
            x+=2;
        } else if (0xfc == (0xfe & u8(data[i])) && i+5 < data.length()) {
            char buf[7] = {data[i], data[i+1], data[i+2], data[i+3], data[i+4], data[i+5], 0};
            termpaint_surface_write_with_colors(surface, x, y, buf, 0xffffff, 0x7f7f7f);
            x += 1;
            i += 5;
        } else if (0xf8 == (0xfc & u8(data[i])) && i+4 < data.length()) {
            char buf[7] = {data[i], data[i+1], data[i+2], data[i+3], data[i+4], 0};
            termpaint_surface_write_with_colors(surface, x, y, buf, 0xffffff, 0x7f7f7f);
            x += 1;
            i += 4;
        } else if (0xf0 == (0xf8 & u8(data[i])) && i+3 < data.length()) {
            char buf[7] = {data[i], data[i+1], data[i+2], data[i+3], 0};
            termpaint_surface_write_with_colors(surface, x, y, buf, 0xffffff, 0x7f7f7f);
            x += 1;
            i += 3;
        } else if (0xe0 == (0xf0 & u8(data[i])) && i+2 < data.length()) {
            char buf[7] = {data[i], data[i+1], data[i+2], 0};
            termpaint_surface_write_with_colors(surface, x, y, buf, 0xffffff, 0x7f7f7f);
            x += 1;
            i += 2;
        } else if (0xc0 == (0xe0 & u8(data[i])) && i+1 < data.length()) {
            if (((unsigned char)data[i]) == 0xc2 && ((unsigned char)data[i+1]) < 0xa0) { // C1 and non breaking space
                char x = ((unsigned char)data[i+1]) >> 4;
                char a = char(x < 10 ? '0' + x : 'a' + x - 10);
                x = data[i+1] & 0xf;
                char b = char(x < 10 ? '0' + x : 'a' + x - 10);
                char buf[7] = {'\\', 'u', '0', '0', a, b, 0};
                termpaint_surface_write_with_colors(surface, x, y, buf, 0xffffff, 0x7f0000);
                x += 6;
            } else {
                char buf[7] = {data[i], data[i+1], 0};
                termpaint_surface_write_with_colors(surface, x, y, buf, 0xffffff, 0x7f7f7f);
                x += 1;
            }
            i += 1;
        } else if (data[i] < 32 || data[i] >= 127) {
            termpaint_surface_write_with_colors(surface, x, y, "\\x", 0xffffff, 0x7f0000);
            x += 2;
            char buf[3];
            sprintf(buf, "%02x", (unsigned char)data[i]);
            termpaint_surface_write_with_colors(surface, x, y, buf, 0xffffff, 0x7f0000);
            x += 2;
        } else {
            char buf[2] = {data[i], 0};
            termpaint_surface_write_with_colors(surface, x, y, buf, 0xffffff, 0x7f7f7f);
            x += 1;
        }
    }
}

void render() {
    termpaint_surface_clear(surface, 0x1000000, 0x1000000);

    termpaint_surface_write_with_colors(surface, 0, 0, "Input Decoding", 0x1ffffff, 0x1000000);
    termpaint_surface_write_with_colors(surface, 20, 0, terminal_info.data(), 0x1cccccc, 0x1000000);

    if (peek_buffer.length()) {
        termpaint_surface_write_with_colors(surface, 0, 23, "unmatched:", 0xff0000, 0x1000000);
        display_esc(11, 23, peek_buffer);
    }

    int y = 2;
    for (DisplayEvent &event : ring) {
        display_esc(5, y, event.raw);
        termpaint_surface_write_with_colors(surface, 20, y, event.eventString.data(), 0xff0000, 0x1000000);
        ++y;
    }

    if (y > 20) {
        ring.erase(ring.begin());
    }

    termpaint_terminal_flush(terminal, false);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    termpaint_integration *integration = termpaint_full_integration_from_fd(1, 0, "+kbdsigint +kbdsigtstp");
    if (!integration) {
        puts("Could not init!");
        return 1;
    }

    // xterm modify other characters: "\e[>4;2m" (disables ctrl-c)
    printf("%s", "\e[?66h");fflush(stdout);
    printf("%s", "\e[?1034l");fflush(stdout);
    printf("%s", "\e[?1036h");fflush(stdout);
    printf("%s", "\e[?1049h");fflush(stdout);

    terminal = termpaint_terminal_new(integration);
    termpaint_full_integration_set_terminal(integration, terminal);
    surface = termpaint_terminal_get_surface(terminal);
    termpaint_terminal_set_raw_input_filter_cb(terminal, raw_filter, 0);
    termpaint_terminal_set_event_cb(terminal, event_handler, 0);
    termpaint_terminal_auto_detect(terminal);
    termpaint_full_integration_wait_for_ready(integration);

    if (termpaint_terminal_auto_detect_state(terminal) == termpaint_auto_detect_done) {
        char buff[100];
        termpaint_terminal_auto_detect_result_text(terminal, buff, sizeof (buff));
        terminal_info = std::string(buff);
    }


    termpaint_surface_resize(surface, 80, 24);
    termpaint_surface_clear(surface, 0x1ffffff, 0x1000000);
    termpaint_terminal_flush(terminal, false);

    render();

    while (!quit) {
        if (!termpaint_full_integration_do_iteration(integration)) {
            // some kind of error
            break;
        }
        peek_buffer = std::string(termpaint_terminal_peek_input_buffer(terminal), termpaint_terminal_peek_input_buffer_length(terminal));
        render();
    }

    printf("%s", "\e[?66;1049l");fflush(stdout);

    termpaint_terminal_free(terminal);

    return 0;
}
