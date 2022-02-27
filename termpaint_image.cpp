// SPDX-License-Identifier: BSL-1.0
#include "termpaint_image.h"

#include <stdio.h>
#include <stdarg.h>

#include <type_traits>

#ifndef BUNDLED_PICOJSON
#include "picojson.h"
#else
#include "third-party/picojson.h"
#endif

#include <termpaint_utf8.h>

static const char *const names[16] = {
    "black",
    "red",
    "green",
    "yellow",
    "blue",
    "magenta",
    "cyan",
    "white",
    "bright black",
    "bright red",
    "bright green",
    "bright yellow",
    "bright blue",
    "bright magenta",
    "bright cyan",
    "bright white",
};

namespace {

struct FileWriter {
    FILE *file;
};

void write_string(FileWriter *w, const char *s) {
    fputs(s, w->file);
}

void write_char(FileWriter *w, int c) {
    putc(c, w->file);
}

struct StringWriter {
    std::string data;
};

void write_string(StringWriter *w, const char *s) {
    w->data += s;
}

void write_char(StringWriter *w, int c) {
    w->data += c;
}


template <typename WRITER>
void write_printf(WRITER *w, const char *format, ...) {
    char buff[100];
    va_list aptr;
    va_start(aptr, format);
    int len = vsnprintf(buff, sizeof(buff), format, aptr);
    va_end(aptr);
    if (len < 0) {
        abort();
    }
    if (len < static_cast<int>(sizeof(buff)) - 1) {
        write_string(w, buff);
    } else {
        char *p = (char*)malloc(len + 1);
        if (!p) {
            abort();
        }
        va_start(aptr, format);
        int res = vsnprintf(p, len + 1, format, aptr);
        if (res < 0) {
            abort();
        }
        va_end(aptr);
        write_string(w, p);
        free(p);
    }
}


template <typename WRITER>
static void print_color(WRITER* f, const char* name, unsigned color) {
    if (color != TERMPAINT_DEFAULT_COLOR) {
        write_printf(f, ", \"%s\": \"", name);
        if ((color & 0xff000000) == TERMPAINT_RGB_COLOR_OFFSET) {
            write_printf(f, "#%02x%02x%02x\"", (color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
        } else if (TERMPAINT_NAMED_COLOR <= color && TERMPAINT_NAMED_COLOR + 15 >= color) {
            write_printf(f, "%s\"", names[color & 0xf]);
        } else if (TERMPAINT_INDEXED_COLOR <= color && TERMPAINT_INDEXED_COLOR + 255 >= color) {
            write_printf(f, "%i\"", color & 0xff);
        }
    }
}

template <typename WRITER>
static int print_style(WRITER* f, int style, const char* name, int flag) {
    if (style & flag) {
        style &= ~flag;
        write_printf(f, ", \"%s\": true", name);
    }
    return style;
}

template <typename WRITER>
static void print_string(WRITER* f, const char* s_signed, size_t len) {
    const unsigned char* s = (const unsigned char*)s_signed;
    for (size_t i = 0; i < len;) {
        int l = termpaintp_utf8_len(s[i]);
        if (i + l > len) {
            break;
        }
        int ch = termpaintp_utf8_decode_from_utf8(s + i, l);
        if (s[i] >= 32 && s[i] <= 126 && s[i] != '"' && s[i] != '\\') {
            write_char(f, s[i]);
        } else {
            unsigned both = termpaintp_utf16_split(ch);
            write_printf(f, "\\u%04x", both & 0xffff);
            if (both > 0xffff) {
                write_printf(f, "\\u%04x", both >> 16);
            }
        }
        i += l;
    }
}

}


template <typename WRITER>
static bool termpaintp_image_save_impl(termpaint_surface *surface, WRITER &writer) {
    bool ok = true;
    const int width = termpaint_surface_width(surface);
    const int height = termpaint_surface_height(surface);

    write_string(&writer, "{\"termpaint_image\": true,\n");

    write_printf(&writer, "  \"width\": %d, \"height\": %d, \"version\": 0, \"cells\":[\n", width, height);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            write_printf(&writer, "    {\"x\": %d, \"y\": %d,\n", x, y);
            write_printf(&writer, "     \"t\": \"");
            int len, left, right;
            const char *text = termpaint_surface_peek_text(surface, x, y, &len, &left, &right);
            if (left != x) {
                ok = false;
            }
            if (len != 1 || text[0] != '\x7f') {
                print_string(&writer, text, len);
                write_string(&writer, "\"");
            } else {
                write_string(&writer, " \", \"cleared\": true");
            }

            if (right - left) {
                write_printf(&writer, ", \"width\": %i", right - left + 1);
            }

            print_color(&writer, "fg", termpaint_surface_peek_fg_color(surface, x, y));
            print_color(&writer, "bg", termpaint_surface_peek_bg_color(surface, x, y));
            print_color(&writer, "deco", termpaint_surface_peek_deco_color(surface, x, y));


            int style = termpaint_surface_peek_style(surface, x, y);
            style = print_style(&writer, style, "bold", TERMPAINT_STYLE_BOLD);
            style = print_style(&writer, style, "italic", TERMPAINT_STYLE_ITALIC);
            style = print_style(&writer, style, "blink", TERMPAINT_STYLE_BLINK);
            style = print_style(&writer, style, "overline", TERMPAINT_STYLE_OVERLINE);
            style = print_style(&writer, style, "inverse", TERMPAINT_STYLE_INVERSE);
            style = print_style(&writer, style, "strike", TERMPAINT_STYLE_STRIKE);
            style = print_style(&writer, style, "underline", TERMPAINT_STYLE_UNDERLINE);
            style = print_style(&writer, style, "double underline", TERMPAINT_STYLE_UNDERLINE_DBL);
            style = print_style(&writer, style, "curly underline", TERMPAINT_STYLE_UNDERLINE_CURLY);

            if (style != 0) {
                ok = false;
            }

            if (termpaint_surface_peek_softwrap_marker(surface, x, y)) {
                write_string(&writer, ", \"x-termpaint-softwrap\": true");
            }

            const char* setup;
            const char* cleanup;
            bool optimize;
            termpaint_surface_peek_patch(surface, x, y, &setup, &cleanup, &optimize);
            if (setup || cleanup) {
                write_string(&writer, ", \"patch\": { \"setup\": ");
                if (setup) {
                    write_string(&writer, "\"");
                    print_string(&writer, setup, strlen(setup));
                    write_string(&writer, "\"");
                } else {
                    write_string(&writer, "null");
                }
                write_string(&writer, ", \"cleanup\": ");
                if (cleanup) {
                    write_string(&writer, "\"");
                    print_string(&writer, cleanup, strlen(cleanup));
                    write_string(&writer, "\"");
                } else {
                    write_string(&writer, "null");
                }
                write_printf(&writer, ", \"optimize\": %s}", optimize ? "true" : "false");
            }

            x = right;

            if (x == width-1 && y == height - 1) {
                write_string(&writer, "}\n");
            } else {
                write_string(&writer, "},\n");
            }
        }
        write_string(&writer, "\n");
    }
    write_string(&writer, "]}\n");

    return ok;
}

char *termpaint_image_save_alloc_buffer(termpaint_surface *surface) {
    StringWriter writer;
    bool res = termpaintp_image_save_impl(surface, writer);
    if (!res) {
        return nullptr;
    } else {
        return strdup(writer.data.data());
    }
}

void termpaint_image_save_dealloc_buffer(char *buffer) {
    free(buffer);
}

bool termpaint_image_save_to_file(termpaint_surface *surface, FILE *file) {
    FileWriter writer{file};
    return termpaintp_image_save_impl(surface, writer);
}

bool termpaint_image_save(termpaint_surface *surface, const char *name) {
    FILE* f = fopen(name, "w");
    if (!f) {
        return false;
    }

    bool ret = termpaint_image_save_to_file(surface, f);

    if (fclose(f) != 0) {
        ret = false;
    }

    return ret;
}

namespace {
    class fread_iterator {
    public:
        fread_iterator() = default;
        fread_iterator(FILE* f) : _f(f), _at_end(false) {
            ++*this;
        }

        bool operator==(const fread_iterator& other) const {
            return _at_end == other._at_end;
        }

        void operator++() {
            size_t ret;
            do {
                ret = fread(&_current, 1, 1, _f);
            } while (ret == 0 && errno == EINTR);
            if (ret < 1) {
                _at_end = true;
            }
        }

        char operator*() {
            return _current;
        }

    private:
        FILE* _f = nullptr;
        bool _at_end = true;
        char _current = 0;
    };

    template<typename T>
    static bool has(const picojson::object& obj, const char* name) {
        return obj.count(name) && obj.at(name).is<T>();
    }

    template<typename T1, typename T2>
    static bool has(const picojson::object& obj, const char* name) {
        return obj.count(name) && (obj.at(name).is<T1>() || obj.at(name).is<T2>());
    }

    template<typename T>
    static T get(const picojson::object& obj, const char* name) {
        return obj.at(name).get<T>();
    }
}

static int read_flag(const picojson::object& obj, const char* name, int flag) {
    if (has<bool>(obj, name) && get<bool>(obj, name)) {
        return flag;
    }
    return 0;
}

static unsigned parse_color(const std::string& s) {
    if (s.size() == 7 && s[0] == '#') {
        int r = 0, g = 0, b = 0;
        sscanf(s.data() + 1, "%02x%02x%02x", &r, &g, &b);
        return TERMPAINT_RGB_COLOR(r, g, b);
    }
    for (unsigned i = 0; i < std::extent<decltype (names)>(); i++) {
        if (s == names[i]) {
            return TERMPAINT_NAMED_COLOR + i;
        }
    }
    unsigned indexed = 0;
    for (unsigned i = 0; i < s.length(); i++) {
        if (s[i] < '0' || s[i] > '9') {
            break;
        }
        indexed *= 10;
        indexed += (unsigned)s[i] - '0';
        if (indexed > 0xff) {
            break;
        }
        if (i + 1 == s.length()) {
            return TERMPAINT_INDEXED_COLOR + indexed;
        }
    }
    return TERMPAINT_DEFAULT_COLOR;
}

static bool streq_nullsafe(const char* a, const char* b) {
    if (!a && !b) {
        return true;
    }
    if (!a || !b) {
        return false;
    }
    return strcmp(a, b) == 0;
}

static termpaint_surface *termpaintp_image_load_from_value(termpaint_terminal *term, picojson::value& rootValue) {
    termpaint_surface *surface = nullptr;

    if (!rootValue.is<picojson::object>()) {
        return nullptr;
    }
    picojson::object root = rootValue.get<picojson::object>();
    if (!has<bool>(root, "termpaint_image") || !has<double>(root, "version")
            || !has<double>(root, "width") || !has<double>(root, "height")
            || !has<picojson::array>(root, "cells")) {
        return nullptr;
    }

    int width = static_cast<int>(get<double>(root, "width"));
    int height = static_cast<int>(get<double>(root, "height"));

    surface = termpaint_terminal_new_surface(term, width, height);
    termpaint_attr* attr = termpaint_attr_new(TERMPAINT_DEFAULT_COLOR, TERMPAINT_DEFAULT_COLOR);

    picojson::array cells = get<picojson::array>(root, "cells");
    for (const auto& cellValue: cells) {
        if (!cellValue.is<picojson::object>()) {
            termpaint_surface_free(surface);
            termpaint_attr_free(attr);
            return nullptr;
        }
        picojson::object cell = cellValue.get<picojson::object>();
        if (!has<double>(cell, "x") || !has<double>(cell, "y")
                || !has<std::string>(cell, "t")) {
            termpaint_surface_free(surface);
            termpaint_attr_free(attr);
            return nullptr;
        }

        unsigned fg = TERMPAINT_DEFAULT_COLOR;
        if (has<std::string>(cell, "fg")) {
            fg = parse_color(get<std::string>(cell, "fg"));
        }
        termpaint_attr_set_fg(attr, fg);

        unsigned bg = TERMPAINT_DEFAULT_COLOR;
        if (has<std::string>(cell, "bg")) {
            bg = parse_color(get<std::string>(cell, "bg"));
        }
        termpaint_attr_set_bg(attr, bg);

        unsigned deco = TERMPAINT_DEFAULT_COLOR;
        if (has<std::string>(cell, "deco")) {
            deco = parse_color(get<std::string>(cell, "deco"));
        }
        termpaint_attr_set_deco(attr, deco);

        const char* setup = nullptr;
        std::string setup_str;
        const char* cleanup = nullptr;
        std::string cleanup_str;
        bool optimize = true;

        if (has<picojson::object>(cell, "patch")) {
            picojson::object patch = get<picojson::object>(cell, "patch");
            if (has<std::string, picojson::null>(patch, "setup")
                    && has<std::string, picojson::null>(patch, "cleanup")
                    && has<bool>(patch, "optimize")) {
                if (has<std::string>(patch, "setup")) {
                    setup_str = get<std::string>(patch, "setup");
                    setup = setup_str.data();
                }
                if (has<std::string>(patch, "cleanup")) {
                    cleanup_str = get<std::string>(patch, "cleanup");
                    cleanup = cleanup_str.data();
                }
                optimize = get<bool>(patch, "optimize");
                termpaint_attr_set_patch(attr, optimize, setup, cleanup);
            } else {
                termpaint_attr_set_patch(attr, false, nullptr, nullptr);
            }
        } else {
            termpaint_attr_set_patch(attr, false, nullptr, nullptr);
        }

        termpaint_attr_reset_style(attr);
        int style = 0;
        style |= read_flag(cell, "bold", TERMPAINT_STYLE_BOLD);
        style |= read_flag(cell, "italic", TERMPAINT_STYLE_ITALIC);
        style |= read_flag(cell, "blink", TERMPAINT_STYLE_BLINK);
        style |= read_flag(cell, "overline", TERMPAINT_STYLE_OVERLINE);
        style |= read_flag(cell, "inverse", TERMPAINT_STYLE_INVERSE);
        style |= read_flag(cell, "strike", TERMPAINT_STYLE_STRIKE);
        style |= read_flag(cell, "underline", TERMPAINT_STYLE_UNDERLINE);
        style |= read_flag(cell, "double underline", TERMPAINT_STYLE_UNDERLINE_DBL);
        style |= read_flag(cell, "curly underline", TERMPAINT_STYLE_UNDERLINE_CURLY);
        termpaint_attr_set_style(attr, style);

        int x = static_cast<int>(get<double>(cell, "x"));
        int y = static_cast<int>(get<double>(cell, "y"));
        int width = 1;
        if (has<double>(cell, "width")) {
            width = static_cast<int>(get<double>(cell, "width"));
        }
        std::string text = get<std::string>(cell, "t");

        bool erased = has<bool>(cell, "cleared") ? get<bool>(cell, "cleared") : false;

        if (erased) {
            text = "\x7f";
        }

        termpaint_surface_write_with_attr(surface, x, y, text.data(), attr);

        if (has<bool>(cell, "x-termpaint-softwrap") && get<bool>(cell, "x-termpaint-softwrap")) {
            termpaint_surface_set_softwrap_marker(surface, x, y, true);
        }

        {
            bool ok = true;
            int actual_len;
            int actual_left, actual_right;
            const char *actual_text = termpaint_surface_peek_text(surface, x, y, &actual_len, &actual_left, &actual_right);
            if (actual_text != text || actual_len != static_cast<int>(text.size()) || actual_left != x || actual_right != x + width - 1) {
                ok = false;
            }
            if (fg != termpaint_surface_peek_fg_color(surface, x, y)
                    || bg != termpaint_surface_peek_bg_color(surface, x, y)
                    || deco != termpaint_surface_peek_deco_color(surface, x, y)
                    || style !=  termpaint_surface_peek_style(surface, x, y)) {
                ok = false;
            }
            const char* actual_setup;
            const char* actual_cleanup;
            bool actual_optimize;
            termpaint_surface_peek_patch(surface, x, y, &actual_setup, &actual_cleanup, &actual_optimize);
            if (!streq_nullsafe(actual_setup, setup) || !streq_nullsafe(actual_cleanup, cleanup)
                    || actual_optimize != optimize) {
                ok = false;
            }

            if (!ok) {
                termpaint_surface_free(surface);
                termpaint_attr_free(attr);
                return nullptr;
            }
        }
    }
    termpaint_attr_free(attr);

    return surface;
}

termpaint_surface *termpaint_image_load_from_file(termpaint_terminal *term, FILE *file) {
    picojson::value rootValue;
    std::string err;
    picojson::parse(rootValue, fread_iterator(file), fread_iterator(), &err);

    if (err.size()) {
        return nullptr;
    }

    return termpaintp_image_load_from_value(term, rootValue);
}

termpaint_surface *termpaint_image_load(termpaint_terminal *term, const char *name) {
    FILE* f = fopen(name, "r");
    if (!f) {
        return nullptr;
    }

    picojson::value rootValue;
    std::string err;
    picojson::parse(rootValue, fread_iterator(f), fread_iterator(), &err);

    fclose(f);
    if (err.size()) {
        return nullptr;
    }

    return termpaintp_image_load_from_value(term, rootValue);
}

termpaint_surface *termpaint_image_load_from_buffer(termpaint_terminal *term, char *buffer, int length) {
    picojson::value rootValue;
    std::string err;
    picojson::parse(rootValue, buffer, buffer + length, &err);

    if (err.size()) {
        return nullptr;
    }

    return termpaintp_image_load_from_value(term, rootValue);
}
