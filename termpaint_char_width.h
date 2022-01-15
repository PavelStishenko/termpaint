// SPDX-License-Identifier: BSL-1.0
#define NEW_WIDTH(num, width) ((num << 2) | (((unsigned)width) & 3))

#include "charclassification.inc"
#include "charclassification_konsole_2018.inc"
#include "charclassification_konsole_2022.inc"

#undef NEW_WIDTH

typedef struct termpaintp_width_ {
    const uint16_t* termpaint_char_width_offsets;
    const uint16_t* termpaint_char_width_data;
} termpaintp_width;

const termpaintp_width termpaintp_char_width_default = {
    .termpaint_char_width_offsets = termpaint_char_width_offsets_default,
    .termpaint_char_width_data = termpaint_char_width_data_default
};

const termpaintp_width termpaintp_char_width_konsole2018 = {
    .termpaint_char_width_offsets = termpaint_char_width_offsets_konsole_2018,
    .termpaint_char_width_data = termpaint_char_width_data_konsole_2018
};

const termpaintp_width termpaintp_char_width_konsole2022 = {
    .termpaint_char_width_offsets = termpaint_char_width_offsets_konsole_2022,
    .termpaint_char_width_data = termpaint_char_width_data_konsole_2022
};

static int termpaintp_char_width(const termpaintp_width *table, int ch) {
    if (ch >= 0x10ffff) {
        // outside of unicode, assume narrow
        return 1;
    }

    const uint16_t* termpaint_char_width_offsets = table->termpaint_char_width_offsets;
    const uint16_t* termpaint_char_width_data = table->termpaint_char_width_data;

    int low = termpaint_char_width_offsets[ch >> 14];
    int high = termpaint_char_width_offsets[(ch >> 14) + 1] - 1;

    int search = (ch & 0x3fff) << 2;

    while (1) {
        if (low == high) {
            if ((termpaint_char_width_data[low] & 0xfffc) > search) {
                // this is save as each section always starts with data for 0
                --low;
            }
            int val = termpaint_char_width_data[low] & 3;
            if (val == 3) {
                return -1;
            }
            return val;
        }

        int mid = low + (high - low) / 2;

        if (termpaint_char_width_data[mid] < search) {
            low = mid + 1;
        } else {
            if ((termpaint_char_width_data[mid] & 0xfffc) == search) {
                int val = termpaint_char_width_data[mid] & 3;
                if (val == 3) {
                    return -1;
                }
                return val;
            } else {
                high = mid;
            }
        }

    }
}
