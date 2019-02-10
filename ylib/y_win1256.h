#pragma once

typedef unsigned UnicodeChar;
typedef unsigned SizeType;

struct UTF8Char_ReadResult {
    bool valid;
    unsigned read_bytes;   // If this is zero, means an error occured or there was no character to read. NOTE: a NUL character is still a character.
    UnicodeChar char_code;
};

inline UTF8Char_ReadResult ReadChar_UTF8 (char const * utf8_str) {
    UTF8Char_ReadResult ret = {};
    if (utf8_str) {
        unsigned char c = static_cast<unsigned char>(utf8_str[ret.read_bytes++]);
        ret.valid = true;
        int remaining_bytes = 0;
        if (c <= 0x7F) {
            ret.char_code = c;
        } else if (c <= 0xBF) {
            ret.valid = false;
        } else if (c <= 0xDF) {
            ret.char_code = c & 0x1F;
            remaining_bytes = 1;
        } else if (c <= 0xEF) {
            ret.char_code = c & 0x0F;
            remaining_bytes = 2;
        } else if (c <= 0xF7) {
            ret.char_code = c & 0x07;
            remaining_bytes = 3;
        } else {
            ret.valid = false;
        } 
        for (int i = 0; i < remaining_bytes; ++i) {
            unsigned char next = static_cast<unsigned char>(utf8_str[ret.read_bytes++]);
            assert((next & 0xC0) == 0x80);
            ret.char_code = (ret.char_code << 6) | (next & 0x3F);
        }
    }
    return ret;
}

struct UTF8Char_WriteResult {
    bool valid;             // Character was valid or not
    bool succeeded;         // Succeeded in writing or not (enough room, etc.)
    unsigned wrote_bytes;   // How many bytes did we write?
};
inline UTF8Char_WriteResult WriteChar_UTF8 (char * out_utf8_ptr, SizeType size_bytes, UnicodeChar char_code) {
    UTF8Char_WriteResult ret = {};
    if (out_utf8_ptr) {
        if (char_code < (1U << 7)) {
            ret.valid = true;
            if (size_bytes >= 1) {
                out_utf8_ptr[0] = char(0x00) | char((char_code >> 0) & 0x7F);
                ret.succeeded = true;
                ret.wrote_bytes = 1;
            }
        } else if (char_code < (1U << 11)) {
            ret.valid = true;
            if (size_bytes >= 2) {
                out_utf8_ptr[0] = char(0xC0) | char((char_code >> 6) & 0x1F);
                out_utf8_ptr[1] = char(0x80) | char((char_code >> 0) & 0x3F);
                ret.succeeded = true;
                ret.wrote_bytes = 2;
            }
        } else if (char_code < (1U << 16)) {
            ret.valid = true;
            if (size_bytes >= 3) {
                out_utf8_ptr[0] = char(0xE0) | char((char_code >>12) & 0x0F);
                out_utf8_ptr[1] = char(0x80) | char((char_code >> 6) & 0x3F);
                out_utf8_ptr[2] = char(0x80) | char((char_code >> 0) & 0x3F);
                ret.succeeded = true;
                ret.wrote_bytes = 3;
            }
        } else if (char_code < (1U << 21)) {
            ret.valid = true;
            if (size_bytes >= 4) {
                out_utf8_ptr[0] = char(0xF0) | char((char_code >>18) & 0x07);
                out_utf8_ptr[1] = char(0x80) | char((char_code >>12) & 0x3F);
                out_utf8_ptr[2] = char(0x80) | char((char_code >> 6) & 0x3F);
                out_utf8_ptr[3] = char(0x80) | char((char_code >> 0) & 0x3F);
                ret.succeeded = true;
                ret.wrote_bytes = 3;
            }
        }
    }
    return ret;
}

inline UnicodeChar ConvChar_from_Windows1256 (char windows1256_char) {
    static const UnicodeChar windows1256_to_unicode_upper_half [128] = {
/*128*/ 0x20AC, 0x067E, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0679, 0x2039, 0x0152, 0x0686, 0x0698, 0x0688,
/*144*/ 0x06AF, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x06A9, 0x2122, 0x0691, 0x203A, 0x0153, 0x200C, 0x200D, 0x06BA,
/*160*/ 0x00A0, 0x060C, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x06BE, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
/*176*/ 0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x061B, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x061F,
/*192*/ 0x06C1, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627, 0x0628, 0x0629, 0x062A, 0x062B, 0x062C, 0x062D, 0x062E, 0x062F,
/*208*/ 0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x00D7, 0x0637, 0x0638, 0x0639, 0x063A, 0x0640, 0x0641, 0x0642, 0x0643,
/*224*/ 0x00E0, 0x0644, 0x00E2, 0x0645, 0x0646, 0x0647, 0x0648, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x0649, 0x064A, 0x00EE, 0x00EF,
/*240*/ 0x064B, 0x064C, 0x064D, 0x064E, 0x00F4, 0x064F, 0x0650, 0x00F7, 0x0651, 0x00F9, 0x0652, 0x00FB, 0x00FC, 0x200E, 0x200F, 0x06D2,
    };
    return (windows1256_char < 128)
        ? windows1256_char
        : windows1256_to_unicode_upper_half[unsigned(windows1256_char) - 128];
}

struct Windows1256Char_ConvResult {
    bool valid;
    char char_code;
};
inline Windows1256Char_ConvResult ConvChar_to_Windows1256 (UnicodeChar unicode_char) {
    struct UnicodeToWindows1256 {
        UnicodeChar uni;
        unsigned char win1256;
    };
    static const UnicodeToWindows1256 unicode_to_windows1256_upper_half [] {
        {0x00A0, 160}, {0x00A2, 162}, {0x00A3, 163}, {0x00A4, 164}, {0x00A5, 165}, {0x00A6, 166}, {0x00A7, 167}, {0x00A8, 168},
        {0x00A9, 169}, {0x00AB, 171}, {0x00AC, 172}, {0x00AD, 173}, {0x00AE, 174}, {0x00AF, 175}, {0x00B0, 176}, {0x00B1, 177},
        {0x00B2, 178}, {0x00B3, 179}, {0x00B4, 180}, {0x00B5, 181}, {0x00B6, 182}, {0x00B7, 183}, {0x00B8, 184}, {0x00B9, 185},
        {0x00BB, 187}, {0x00BC, 188}, {0x00BD, 189}, {0x00BE, 190}, {0x00D7, 215}, {0x00E0, 224}, {0x00E2, 226}, {0x00E7, 231},
        {0x00E8, 232}, {0x00E9, 233}, {0x00EA, 234}, {0x00EB, 235}, {0x00EE, 238}, {0x00EF, 239}, {0x00F4, 244}, {0x00F7, 247},
        {0x00F9, 249}, {0x00FB, 251}, {0x00FC, 252}, {0x0152, 140}, {0x0153, 156}, {0x0192, 131}, {0x02C6, 136}, {0x060C, 161},
        {0x061B, 186}, {0x061F, 191}, {0x0621, 193}, {0x0622, 194}, {0x0623, 195}, {0x0624, 196}, {0x0625, 197}, {0x0626, 198},
        {0x0627, 199}, {0x0628, 200}, {0x0629, 201}, {0x062A, 202}, {0x062B, 203}, {0x062C, 204}, {0x062D, 205}, {0x062E, 206},
        {0x062F, 207}, {0x0630, 208}, {0x0631, 209}, {0x0632, 210}, {0x0633, 211}, {0x0634, 212}, {0x0635, 213}, {0x0636, 214},
        {0x0637, 216}, {0x0638, 217}, {0x0639, 218}, {0x063A, 219}, {0x0640, 220}, {0x0641, 221}, {0x0642, 222}, {0x0643, 223},
        {0x0644, 225}, {0x0645, 227}, {0x0646, 228}, {0x0647, 229}, {0x0648, 230}, {0x0649, 236}, {0x064A, 237}, {0x064B, 240},
        {0x064C, 241}, {0x064D, 242}, {0x064E, 243}, {0x064F, 245}, {0x0650, 246}, {0x0651, 248}, {0x0652, 250},
    /**/{0x066A,  37},                                                                                                         /**/
                                                                                                                 {0x0679, 138},
        {0x067E, 129}, {0x0686, 141}, {0x0688, 143}, {0x0691, 154}, {0x0698, 142}, {0x06A9, 152}, {0x06AF, 144}, {0x06BA, 159},
        {0x06BE, 170}, {0x06C1, 192},
    /**/{0x06CC, 237},                                                                                                         /**/
                                      {0x06D2, 255},
    /**/{0x06F0,  48}, {0x06F1,  49}, {0x06F2,  50}, {0x06F3,  51}, {0x06F4,  52}, {0x06F5,  53}, {0x06F6,  54}, {0x06F7,  55},/**/
    /**/{0x06F8,  56}, {0x06F9,  57},                                                                                          /**/
                                                     {0x200C, 157}, {0x200D, 158}, {0x200E, 253}, {0x200F, 254}, {0x2013, 150},
        {0x2014, 151}, {0x2018, 145}, {0x2019, 146}, {0x201A, 130}, {0x201C, 147}, {0x201D, 148}, {0x201E, 132}, {0x2020, 134},
        {0x2021, 135}, {0x2022, 149}, {0x2026, 133}, {0x2030, 137}, {0x2039, 139}, {0x203A, 155}, {0x20AC, 128}, {0x2122, 153},
    };
    Windows1256Char_ConvResult ret = {};
    if (unicode_char < 128) {
        ret.valid = true;
        ret.char_code = char(unicode_char);
    } else {
        int lo = 0, hi = sizeof(unicode_to_windows1256_upper_half) / sizeof(unicode_to_windows1256_upper_half[0]);
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            UnicodeToWindows1256 p = unicode_to_windows1256_upper_half[mid];
            if (p.uni == unicode_char) {
                ret.valid = true;
                ret.char_code = char(p.win1256);
                break;
            } else if (p.uni < unicode_char) {
                lo = mid + 1;
            } else {    // if (p.uni > unicode_char)
                hi = mid;
            }
        }
    }
    return ret;
}

inline SizeType StrLen_UTF8 (char const * utf8_str, unsigned size_in_bytes) {
    SizeType ret = 0;
    if (utf8_str) {
        char const * const end = utf8_str + size_in_bytes;
        while (*utf8_str && utf8_str < end) {
            unsigned char c = static_cast<unsigned char>(*utf8_str);
            if (c <= 0x7F) {
                utf8_str += 1;
                ret += 1;
            } else if (c <= 0xBF) {
                utf8_str += 1;
            } else if (c <= 0xDF) {
                utf8_str += 2;
                ret += 1;
            } else if (c <= 0xEF) {
                utf8_str += 3;
                ret += 1;
            } else if (c <= 0xF7) {
                utf8_str += 4;
                ret += 1;
            } else {
                utf8_str += 1;
            } 
        }
    }
    return ret;
}

// Always writes a NUL to output (if out != NULL and out_size > 0), but the return size doesn't count the NUL.
inline SizeType ConvStr_UTF8_to_Windows1256 (char * out_windows1256_str, SizeType windows1256_size_bytes, char const * in_utf8_str, SizeType utf8_size_bytes) {
    SizeType ret = 0;
    if (in_utf8_str && utf8_size_bytes > 0 && out_windows1256_str && windows1256_size_bytes > 0) {
        char const * const in_end = in_utf8_str + utf8_size_bytes;
        while (*in_utf8_str && in_utf8_str < in_end && ret < windows1256_size_bytes - 1) {
            UTF8Char_ReadResult r = ReadChar_UTF8(in_utf8_str);
            in_utf8_str += r.read_bytes;
            if (r.valid) {
                Windows1256Char_ConvResult w = ConvChar_to_Windows1256(r.char_code);
                if (w.valid) {
                    out_windows1256_str[ret] = w.char_code;
                    ret += 1;
                }
            }
        }
        out_windows1256_str[ret] = '\0';    // NUL is NUL
    }
    return ret;
}

// Returns number of BYTES, not chars. Also, doesn't count the NUL at the end.
inline SizeType ConvStr_Windows1256_to_UTF8 (char * out_utf8_str, SizeType utf8_size_bytes, char const * in_windows1256_str, SizeType windows1256_size_bytes) {
    SizeType ret = 0;
    if (in_windows1256_str && windows1256_size_bytes > 0 && out_utf8_str && utf8_size_bytes > 0) {
        char const * const in_end = in_windows1256_str + windows1256_size_bytes;
        while (*in_windows1256_str && in_windows1256_str < in_end && ret < utf8_size_bytes - 1) {
            UnicodeChar r = ConvChar_from_Windows1256(*in_windows1256_str);
            in_windows1256_str += 1;
            UTF8Char_WriteResult w = WriteChar_UTF8(out_utf8_str, utf8_size_bytes, r);
            if (w.succeeded) {
                utf8_size_bytes -= w.wrote_bytes;
                ret += w.wrote_bytes;
            }
        }
        out_utf8_str[ret] = '\0';       // NUL is NUL
    }
    return ret;
}
