/*******************************************************************************
 * SPDX-License-Identifier: Apache-2.0
 *
 * Exp08: LUMI.BUDDY temperature clock on LCD12864 (ST7920 graphic mode)
 * Board : Puzhong 51 / STC89C52 compatible board
 *
 * Required wiring from the laboratory handout:
 *   LCD DB0-DB7=P0, RS=P2.6, RW=P2.5, E=P2.7, PSB=P3.2
 *   DS1302 IO=P3.4, RST=P3.5, CLK=P3.6
 *   DS18B20 DQ=P3.7 in the handout; kept as a reserved interface here.
 *
 * Design notes:
 *   - Graphic mode is used for a pixel avatar and a composed dashboard.
 *   - Temperature is represented in tenths of a degree: no float/sprintf.
 *   - Current demo uses reserved temperature mode because the lab board DQ
 *     line was observed stuck low; DS18B20 functions remain for future use.
 *   - No 1024-byte framebuffer is allocated; one 16-byte scan line is streamed
 *     into the LCD to fit the limited internal RAM of an 8051.
 ******************************************************************************/
#include "reg52.h"
#include "intrins.h"

typedef unsigned char u8;
typedef unsigned int u16;

#define LCD12864_DATA P0

sbit LCD_RS  = P2^6;
sbit LCD_RW  = P2^5;
sbit LCD_E   = P2^7;
sbit LCD_PSB = P3^2;

sbit RTC_IO  = P3^4;
sbit RTC_RST = P3^5;
sbit RTC_CLK = P3^6;
sbit TEMP_DQ = P3^7;

#define LOOP_MS                 20
#define RTC_POLL_TICKS          10
#define TEMP_READ_TICKS         100

#define LCD_SETUP_MS            1
#define LCD_HIGH_MS             1
#define LCD_HOLD_MS             1
#define TEMP_RESERVED_MODE      1
#define TEMP_RAW_DIAG           0

/* Set to 1 for one download after editing the initial time, then set to 0. */
#define RTC_FORCE_SET_ON_BOOT   0
#define INIT_YEAR               26
#define INIT_MONTH              5
#define INIT_DAY                27
#define INIT_WEEK               3
#define INIT_HOUR               12
#define INIT_MINUTE             0
#define INIT_SECOND             0

#define BCD(n) ((((n) / 10) << 4) | ((n) % 10))

typedef enum {
    FACE_IDLE = 0,
    FACE_SLEEP,
    FACE_HOT,
    FACE_COLD,
    FACE_HEART,
    FACE_WAVE,
    FACE_ERROR,
    FACE_COUNT
} FaceState;

typedef struct {
    u8 sec;
    u8 min;
    u8 hour;
    u8 day;
    u8 month;
    u8 week;
    u8 year;
} Clock;

u8 code rtc_read_addr[7] = {0x81, 0x83, 0x85, 0x87, 0x89, 0x8b, 0x8d};
u8 code rtc_write_addr[7] = {0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c};
u8 rtc_raw[7];

u8 line_buffer[16];
char header_text[16] = "LUMI  00/00/00";
char time_text[6] = "00:00";
char temp_text[9] = "T: --.-C";
char status_text[11] = "00 A IDLE";
char code footer_text[] = "SIM TEMP CLOCK";
char code day_of_week[7][4] = {"SUN","MON","TUE","WED","THU","FRI","SAT"};

Clock clock_now;
int temperature10 = 0;
u8 temperature_valid = 0;
u8 temp_raw_low = 0xff;
u8 temp_raw_high = 0xff;
u8 temp_present_before = 0;
u8 temp_present_after = 0;
u8 temp_dq_idle = 1;
u8 diag_dq_release = 1;
u8 diag_p3_release = 0xff;
FaceState visible_face = FACE_IDLE;

/* 5x7 uppercase font: each byte is one vertical column, bit 0 is the top. */
u8 code font_digit[10][5] = {
    {0x3e, 0x51, 0x49, 0x45, 0x3e},
    {0x00, 0x42, 0x7f, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46},
    {0x21, 0x41, 0x45, 0x4b, 0x31},
    {0x18, 0x14, 0x12, 0x7f, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39},
    {0x3c, 0x4a, 0x49, 0x49, 0x30},
    {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36},
    {0x06, 0x49, 0x49, 0x29, 0x1e}
};

u8 code font_alpha[26][5] = {
    {0x7e, 0x11, 0x11, 0x11, 0x7e}, /* A */
    {0x7f, 0x49, 0x49, 0x49, 0x36}, /* B */
    {0x3e, 0x41, 0x41, 0x41, 0x22}, /* C */
    {0x7f, 0x41, 0x41, 0x22, 0x1c}, /* D */
    {0x7f, 0x49, 0x49, 0x49, 0x41}, /* E */
    {0x7f, 0x09, 0x09, 0x09, 0x01}, /* F */
    {0x3e, 0x41, 0x49, 0x49, 0x7a}, /* G */
    {0x7f, 0x08, 0x08, 0x08, 0x7f}, /* H */
    {0x00, 0x41, 0x7f, 0x41, 0x00}, /* I */
    {0x20, 0x40, 0x41, 0x3f, 0x01}, /* J */
    {0x7f, 0x08, 0x14, 0x22, 0x41}, /* K */
    {0x7f, 0x40, 0x40, 0x40, 0x40}, /* L */
    {0x7f, 0x02, 0x0c, 0x02, 0x7f}, /* M */
    {0x7f, 0x04, 0x08, 0x10, 0x7f}, /* N */
    {0x3e, 0x41, 0x41, 0x41, 0x3e}, /* O */
    {0x7f, 0x09, 0x09, 0x09, 0x06}, /* P */
    {0x3e, 0x41, 0x51, 0x21, 0x5e}, /* Q */
    {0x7f, 0x09, 0x19, 0x29, 0x46}, /* R */
    {0x46, 0x49, 0x49, 0x49, 0x31}, /* S */
    {0x01, 0x01, 0x7f, 0x01, 0x01}, /* T */
    {0x3f, 0x40, 0x40, 0x40, 0x3f}, /* U */
    {0x1f, 0x20, 0x40, 0x20, 0x1f}, /* V */
    {0x3f, 0x40, 0x38, 0x40, 0x3f}, /* W */
    {0x63, 0x14, 0x08, 0x14, 0x63}, /* X */
    {0x07, 0x08, 0x70, 0x08, 0x07}, /* Y */
    {0x61, 0x51, 0x49, 0x45, 0x43}  /* Z */
};

void delay_10us(u16 count)
{
    while (count--)
    {
        _nop_();
        _nop_();
        _nop_();
        _nop_();
        _nop_();
    }
}

void delay_ms(u16 ms)
{
    u16 i;
    u16 j;

    for (i = 0; i < ms; i++)
    {
        for (j = 0; j < 110; j++);
    }
}

void release_exp08_buses(void)
{
    RTC_RST = 0;
    RTC_CLK = 0;
    RTC_IO = 1;
    TEMP_DQ = 1;
    delay_10us(5);
}

void lcd_write_raw(u8 is_data, u8 value)
{
    LCD_RS = is_data;
    LCD_RW = 0;
    LCD_E = 0;
    LCD12864_DATA = value;
    delay_ms(LCD_SETUP_MS);
    LCD_E = 1;
    delay_ms(LCD_HIGH_MS);
    LCD_E = 0;
    delay_ms(LCD_HOLD_MS);
}

void lcd_cmd(u8 command)
{
    lcd_write_raw(0, command);
}

void lcd_data(u8 value)
{
    lcd_write_raw(1, value);
}

void lcd_init_graphic(void)
{
    LCD_PSB = 1;
    delay_ms(50);
    lcd_cmd(0x30);
    delay_ms(5);
    lcd_cmd(0x0c);
    delay_ms(5);
    lcd_cmd(0x01);
    delay_ms(5);
    lcd_cmd(0x34);
    delay_ms(5);
    lcd_cmd(0x36);
    delay_ms(5);
}

void lcd_write_scanline(u8 y)
{
    u8 i;

    lcd_cmd(0x80 | (y & 0x1f));
    lcd_cmd((y < 32) ? 0x80 : 0x88);
    for (i = 0; i < 16; i++)
    {
        lcd_data(line_buffer[i]);
    }
}

void line_clear(void)
{
    u8 i;

    for (i = 0; i < 16; i++)
    {
        line_buffer[i] = 0;
    }
}

void line_set_pixel(u8 x)
{
    if (x < 128)
    {
        line_buffer[x >> 3] |= (0x80 >> (x & 0x07));
    }
}

void line_clear_pixel(u8 x)
{
    if (x < 128)
    {
        line_buffer[x >> 3] &= (u8)~(0x80 >> (x & 0x07));
    }
}

void line_set_span(u8 left, u8 right)
{
    u8 x;

    if (right >= 128)
    {
        right = 127;
    }
    for (x = left; x <= right; x++)
    {
        line_set_pixel(x);
    }
}

void line_clear_span(u8 left, u8 right)
{
    u8 x;

    for (x = left; x <= right; x++)
    {
        line_clear_pixel(x);
    }
}

u8 font_column(char ch, u8 column)
{
    if (column >= 5)
    {
        return 0;
    }
    if (ch >= '0' && ch <= '9')
    {
        return font_digit[ch - '0'][column];
    }
    if (ch >= 'A' && ch <= 'Z')
    {
        return font_alpha[ch - 'A'][column];
    }

    switch (ch)
    {
        case ':': return (column == 2) ? 0x24 : 0x00;
        case '.': return (column == 2) ? 0x40 : 0x00;
        case '-': return (column < 4) ? 0x08 : 0x00;
        case '/': return (column < 5) ? (0x40 >> column) : 0x00;
        case '>': return (column == 1) ? 0x22 :
                         ((column == 2) ? 0x14 :
                         ((column == 3) ? 0x08 : 0x00));
        default: return 0x00;
    }
}

void render_text_row(u8 x, u8 y, u8 scale, char *text, u8 scan_y)
{
    u8 glyph_row;
    u8 column;
    u8 stretch;
    u8 cursor = x;

    if (scan_y < y || scan_y >= (u8)(y + 7 * scale))
    {
        return;
    }

    glyph_row = (scan_y - y) / scale;
    while (*text != '\0' && cursor < 127)
    {
        for (column = 0; column < 5; column++)
        {
            if (font_column(*text, column) & (0x01 << glyph_row))
            {
                for (stretch = 0; stretch < scale; stretch++)
                {
                    line_set_pixel(cursor + column * scale + stretch);
                }
            }
        }
        cursor += 6 * scale;
        text++;
    }
}

void render_code_text_row(u8 x, u8 y, u8 scale, char code *text, u8 scan_y)
{
    u8 glyph_row;
    u8 column;
    u8 stretch;
    u8 cursor = x;

    if (scan_y < y || scan_y >= (u8)(y + 7 * scale))
    {
        return;
    }

    glyph_row = (scan_y - y) / scale;
    while (*text != '\0' && cursor < 127)
    {
        for (column = 0; column < 5; column++)
        {
            if (font_column(*text, column) & (0x01 << glyph_row))
            {
                for (stretch = 0; stretch < scale; stretch++)
                {
                    line_set_pixel(cursor + column * scale + stretch);
                }
            }
        }
        cursor += 6 * scale;
        text++;
    }
}

void avatar_span(u8 left, u8 right)
{
    line_set_span(6 + left, 6 + right);
}

void avatar_cut(u8 left, u8 right)
{
    line_clear_span(6 + left, 6 + right);
}

void render_avatar_row(FaceState face, u8 frame, u8 scan_y)
{
    int row = (int)scan_y - 12;
    u8 left;
    u8 right;

    if (row < 0 || row > 39)
    {
        return;
    }

    /* Hair ribbon and antenna-like accent. */
    if (row == 0) avatar_span(27, 30);
    if (row == 1) { avatar_span(24, 28); avatar_span(30, 34); }
    if (row == 2) { avatar_span(23, 26); avatar_span(32, 35); }
    if (row == 3) avatar_span(25, 33);

    /* Filled hair silhouette; a face opening is carved out below. */
    if (row >= 4 && row <= 29)
    {
        if (row < 8)
        {
            left = 15 - (row - 4) * 3;
            right = 25 + (row - 4) * 3;
        }
        else if (row < 24)
        {
            left = 5;
            right = 35;
        }
        else
        {
            left = 5 + (row - 24);
            right = 35 - (row - 24);
        }
        avatar_span(left, right);
    }

    if (row >= 10 && row <= 25)
    {
        if (row < 13)
        {
            avatar_cut(13 - (row - 10), 27 + (row - 10));
        }
        else if (row < 23)
        {
            avatar_cut(10, 30);
        }
        else
        {
            avatar_cut(11 + (row - 23), 29 - (row - 23));
        }
    }

    /* Bangs stay over the opening. */
    if (row == 10) { avatar_span(10, 18); avatar_span(24, 30); }
    if (row == 11) { avatar_span(10, 16); avatar_span(24, 28); }
    if (row == 12) { avatar_span(10, 13); avatar_span(23, 26); }

    /* Outfit and bow tie. */
    if (row == 29) avatar_span(18, 22);
    if (row == 30) avatar_span(15, 25);
    if (row == 31) { avatar_span(10, 17); avatar_span(23, 30); }
    if (row >= 32 && row <= 37) avatar_span(8, 32);
    if (row == 38) avatar_span(10, 30);
    if (row == 39) avatar_span(13, 27);
    if (row == 33 || row == 34) avatar_cut(18, 22);
    if (row == 35) avatar_span(19, 21);

    /* Facial expression and small animation effects. */
    if (face == FACE_SLEEP)
    {
        if (row == 17) { avatar_span(13, 16); avatar_span(24, 27); }
        if (row == 22) avatar_span(18, 22);
        if (row == 14) avatar_span(34, 37);
        if (row == 13) avatar_span(36, 38);
        if (row == 10 && frame) avatar_span(38, 39);
    }
    else if (face == FACE_HOT || face == FACE_ERROR)
    {
        if (row == 16 || row == 19)
        {
            avatar_span(13, 16);
            avatar_span(24, 27);
        }
        if (row == 17 || row == 18)
        {
            avatar_span(14, 15);
            avatar_span(25, 26);
        }
        if (row == 22) avatar_span(18, 22);
        if (face == FACE_HOT && row >= 12 && row <= 15)
        {
            line_set_pixel(43 - (u8)(row - 12));
        }
        if (face == FACE_ERROR && row >= 12 && row <= 17)
        {
            line_set_pixel(44);
            if (row == 17) line_set_pixel(43);
        }
    }
    else if (face == FACE_HEART)
    {
        if (row == 16) { avatar_span(12, 13); avatar_span(15, 16); avatar_span(24, 25); avatar_span(27, 28); }
        if (row == 17) { avatar_span(12, 16); avatar_span(24, 28); }
        if (row == 18) { avatar_span(13, 15); avatar_span(25, 27); }
        if (row == 19) { line_set_pixel(20); line_set_pixel(32); }
        if (row == 22) avatar_span(17, 23);
        if (row == 23) avatar_span(18, 22);
    }
    else
    {
        if (row == 17)
        {
            if (face == FACE_IDLE && frame)
            {
                avatar_span(13, 16);
                avatar_span(24, 27);
            }
            else
            {
                avatar_span(14, 15);
                avatar_span(25, 26);
            }
        }
        if (row == 21) { line_set_pixel(17); line_set_pixel(23); }
        if (row == 22) avatar_span(18, 22);
        if (face == FACE_COLD && (row == 27 || row == 29))
        {
            avatar_span(1, 3);
            avatar_span(37, 39);
        }
        if (face == FACE_WAVE)
        {
            if (row >= 21 && row <= 27)
            {
                line_set_pixel(39 - (u8)(row - 21) / 2);
            }
            if (row == 20) avatar_span(38, 40);
            if (row == 19 && frame) avatar_span(40, 42);
        }
    }
}

void render_rows(u8 top, u8 bottom)
{
    u8 y;

    if (top > bottom)
    {
        return;
    }
    if (bottom > 63)
    {
        bottom = 63;
    }

    for (y = top; y <= bottom; y++)
    {
        line_clear();

        if (y == 0 || y == 10 || y == 52 || y == 63)
        {
            line_set_span(0, 127);
        }
        else
        {
            line_set_pixel(0);
            line_set_pixel(127);
        }

        render_text_row(4, 2, 1, header_text, y);
        render_avatar_row(visible_face, 0, y);
        render_text_row(60, 14, 2, time_text, y);
        render_text_row(60, 32, 1, temp_text, y);
        render_text_row(60, 42, 1, status_text, y);
        render_code_text_row(4, 55, 1, footer_text, y);

        lcd_write_scanline(y);
    }
}

void render_screen(void)
{
    render_rows(0, 63);
}

u8 bcd_to_dec(u8 value)
{
    return (value >> 4) * 10 + (value & 0x0f);
}

u8 is_valid_bcd(u8 value, u8 maximum)
{
    u8 decimal = bcd_to_dec(value);

    return ((value & 0x0f) <= 9 && ((value >> 4) & 0x0f) <= 9 &&
            decimal <= maximum);
}

void rtc_write_byte(u8 addr, u8 value)
{
    u8 i;

    RTC_RST = 0;
    RTC_CLK = 0;
    _nop_();
    RTC_RST = 1;
    _nop_();

    for (i = 0; i < 8; i++)
    {
        RTC_IO = addr & 0x01;
        addr >>= 1;
        RTC_CLK = 1;
        _nop_();
        RTC_CLK = 0;
        _nop_();
    }

    for (i = 0; i < 8; i++)
    {
        RTC_IO = value & 0x01;
        value >>= 1;
        RTC_CLK = 1;
        _nop_();
        RTC_CLK = 0;
        _nop_();
    }
    RTC_RST = 0;
    RTC_IO = 1;
}

u8 rtc_read_byte(u8 addr)
{
    u8 i;
    u8 bit_value;
    u8 value = 0;

    RTC_RST = 0;
    RTC_CLK = 0;
    _nop_();
    RTC_RST = 1;
    _nop_();

    for (i = 0; i < 8; i++)
    {
        RTC_IO = addr & 0x01;
        addr >>= 1;
        RTC_CLK = 1;
        _nop_();
        RTC_CLK = 0;
        _nop_();
    }

    RTC_IO = 1;
    for (i = 0; i < 8; i++)
    {
        bit_value = RTC_IO;
        value = (bit_value << 7) | (value >> 1);
        RTC_CLK = 1;
        _nop_();
        RTC_CLK = 0;
        _nop_();
    }
    RTC_RST = 0;
    _nop_();
    RTC_CLK = 1;
    _nop_();
    RTC_IO = 0;
    _nop_();
    RTC_IO = 1;
    _nop_();
    return value;
}

u8 rtc_data_valid(void)
{
    return (!(rtc_raw[0] & 0x80) &&
            is_valid_bcd(rtc_raw[0] & 0x7f, 59) &&
            is_valid_bcd(rtc_raw[1] & 0x7f, 59) &&
            is_valid_bcd(rtc_raw[2] & 0x3f, 23) &&
            is_valid_bcd(rtc_raw[3] & 0x3f, 31) &&
            bcd_to_dec(rtc_raw[3] & 0x3f) > 0 &&
            is_valid_bcd(rtc_raw[4] & 0x1f, 12) &&
            bcd_to_dec(rtc_raw[4] & 0x1f) > 0);
}

u8 rtc_read_time(void)
{
    u8 i;

    for (i = 0; i < 7; i++)
    {
        rtc_raw[i] = rtc_read_byte(rtc_read_addr[i]);
    }

    if (!rtc_data_valid())
    {
        return 0;
    }

    clock_now.sec = bcd_to_dec(rtc_raw[0] & 0x7f);
    clock_now.min = bcd_to_dec(rtc_raw[1] & 0x7f);
    clock_now.hour = bcd_to_dec(rtc_raw[2] & 0x3f);
    clock_now.day = bcd_to_dec(rtc_raw[3] & 0x3f);
    clock_now.month = bcd_to_dec(rtc_raw[4] & 0x1f);
    clock_now.week = bcd_to_dec(rtc_raw[5] & 0x07);
    clock_now.year = bcd_to_dec(rtc_raw[6]);
    return 1;
}

void rtc_set_initial_time(void)
{
    u8 initial[7];
    u8 i;

    initial[0] = BCD(INIT_SECOND);
    initial[1] = BCD(INIT_MINUTE);
    initial[2] = BCD(INIT_HOUR);
    initial[3] = BCD(INIT_DAY);
    initial[4] = BCD(INIT_MONTH);
    initial[5] = BCD(INIT_WEEK);
    initial[6] = BCD(INIT_YEAR);

    rtc_write_byte(0x8e, 0x00);
    for (i = 0; i < 7; i++)
    {
        rtc_write_byte(rtc_write_addr[i], initial[i]);
    }
    rtc_write_byte(0x8e, 0x80);
}

void rtc_init(void)
{
    rtc_read_time();
#if RTC_FORCE_SET_ON_BOOT
    rtc_set_initial_time();
    rtc_read_time();
#else
    if (!rtc_data_valid())
    {
        rtc_set_initial_time();
        rtc_read_time();
    }
#endif
}

#if !TEMP_RESERVED_MODE
void temp_reset(void)
{
    TEMP_DQ = 0;
    delay_10us(75);
    TEMP_DQ = 1;
    delay_10us(2);
}

u8 temp_present(void)
{
    u8 timeout = 0;

    while (TEMP_DQ && timeout < 20)
    {
        timeout++;
        delay_10us(1);
    }
    if (timeout >= 20)
    {
        return 0;
    }

    timeout = 0;
    while (!TEMP_DQ && timeout < 25)
    {
        timeout++;
        delay_10us(1);
    }
    if (timeout >= 25)
    {
        return 0;
    }

    delay_10us(40);
    return 1;
}

void temp_write_byte(u8 dat)
{
    u8 i;
    u8 ea_bak;

    for (i = 0; i < 8; i++)
    {
        ea_bak = EA;
        EA = 0;

        TEMP_DQ = 0;
        _nop_();

        if (dat & 0x01)
        {
            TEMP_DQ = 1;
            delay_10us(6);
        }
        else
        {
            TEMP_DQ = 0;
            delay_10us(6);
            TEMP_DQ = 1;
        }

        EA = ea_bak;

        dat >>= 1;
        delay_10us(1);
    }
}

u8 temp_read_bit(void)
{
    u8 dat;
    u8 ea_bak;

    ea_bak = EA;
    EA = 0;

    TEMP_DQ = 0;
    _nop_();
    _nop_();

    TEMP_DQ = 1;
    _nop_();
    _nop_();
    _nop_();

    dat = TEMP_DQ;

    EA = ea_bak;

    delay_10us(5);

    return dat;
}

u8 temp_read_byte(void)
{
    u8 i;
    u8 value = 0;

    for (i = 0; i < 8; i++)
    {
        value >>= 1;
        if (temp_read_bit())
        {
            value |= 0x80;
        }
    }
    return value;
}
#endif

u8 temp_read_temperature(int *result)
{
#if TEMP_RESERVED_MODE
    u8 phase;

    phase = clock_now.sec % 20;
    if (phase > 10)
    {
        phase = 20 - phase;
    }

    *result = 240 + (int)phase;
    temp_raw_low = 0x00;
    temp_raw_high = 0x00;
    temp_present_before = 0;
    temp_present_after = 0;
    temp_dq_idle = 1;
    return 1;
#else
    u8 low;
    u8 high;
    u16 magnitude;

    temp_reset();
    temp_present();
    temp_write_byte(0xcc);
    temp_write_byte(0x44);
    delay_ms(750);

    temp_reset();
    temp_present();
    temp_write_byte(0xcc);
    temp_write_byte(0xbe);
    low = temp_read_byte();
    high = temp_read_byte();
    magnitude = ((u16)high << 8) | low;

    if (magnitude == 0xffff || magnitude == 0x0000)
    {
        return 0;
    }

    if (magnitude & 0x8000)
    {
        magnitude = (~magnitude) + 1;
        *result = -((int)((magnitude * 10U + 8U) / 16U));
    }
    else
    {
        *result = (int)((magnitude * 10U + 8U) / 16U);
    }
    return 1;
#endif
}

void write_two_digits(char *dest, u8 value)
{
    dest[0] = (value / 10) + '0';
    dest[1] = (value % 10) + '0';
}

#if TEMP_RAW_DIAG
char hex_digit(u8 value)
{
    value &= 0x0f;
    return (value < 10) ? (value + '0') : (value - 10 + 'A');
}
#endif

void build_header(void)
{
    write_two_digits(&header_text[6], clock_now.year);
    write_two_digits(&header_text[9], clock_now.month);
    write_two_digits(&header_text[12], clock_now.day);
}

void build_time(void)
{
    write_two_digits(&time_text[0], clock_now.hour);
    write_two_digits(&time_text[3], clock_now.min);
}

void build_temperature(void)
{
#if TEMP_RAW_DIAG
    temp_text[0] = 'R';
    temp_text[1] = ':';
    temp_text[2] = hex_digit(temp_raw_high >> 4);
    temp_text[3] = hex_digit(temp_raw_high);
    temp_text[4] = hex_digit(temp_raw_low >> 4);
    temp_text[5] = hex_digit(temp_raw_low);
    temp_text[6] = ' ';
    temp_text[7] = ' ';
    temp_text[8] = '\0';
#else
    u16 value;
    u8 tens;

    if (!temperature_valid)
    {
        temp_text[3] = 'N';
        temp_text[4] = '/';
        temp_text[5] = 'A';
        temp_text[6] = ' ';
        temp_text[7] = ' ';
        temp_text[8] = '\0';
        return;
    }

    if (temperature10 < 0)
    {
        temp_text[2] = '-';
        value = (u16)(-temperature10);
    }
    else
    {
        temp_text[2] = ' ';
        value = (u16)temperature10;
    }
    if (value > 999)
    {
        value = 999;
    }
    tens = (u8)(value / 10);
    temp_text[3] = (tens >= 10) ? ((tens / 10) + '0') : ' ';
    temp_text[4] = (tens % 10) + '0';
    temp_text[5] = '.';
    temp_text[6] = (value % 10) + '0';
    temp_text[7] = 'C';
    temp_text[8] = '\0';
#endif
}

#if !TEMP_RAW_DIAG
char *face_name(FaceState face)
{
    switch (face)
    {
        case FACE_SLEEP: return "SLEEP";
        case FACE_HOT: return "HOT";
        case FACE_COLD: return "COLD";
        case FACE_HEART: return "HEART";
        case FACE_WAVE: return "WAVE";
        case FACE_ERROR: return "ERR";
        default: return "IDLE";
    }
}
#endif

FaceState choose_auto_face(void)
{
    if (!temperature_valid)
    {
        switch (clock_now.sec % 3)
        {
            case 0: return FACE_IDLE;
            case 1: return FACE_WAVE;
            default: return FACE_HEART;
        }
    }
    if (temperature10 >= 300)
    {
        return FACE_HOT;
    }
    if (temperature10 <= 150)
    {
        return FACE_COLD;
    }
    if (clock_now.hour >= 23 || clock_now.hour < 7)
    {
        return FACE_SLEEP;
    }
    return FACE_IDLE;
}

void build_status(void)
{
#if TEMP_RAW_DIAG
    status_text[0] = 'B';
    status_text[1] = diag_dq_release ? '1' : '0';
    status_text[2] = 'D';
    status_text[3] = temp_dq_idle ? '1' : '0';
    status_text[4] = 'P';
    status_text[5] = hex_digit(diag_p3_release >> 4);
    status_text[6] = hex_digit(diag_p3_release);
    status_text[7] = temperature_valid ? ' ' : '!';
    status_text[8] = ' ';
    status_text[9] = ' ';
    status_text[10] = '\0';
#else
    char *name;
    u8 i = 5;

    write_two_digits(&status_text[0], clock_now.sec);
    status_text[2] = ' ';
    status_text[3] = 'A';
    status_text[4] = ' ';
    name = face_name(visible_face);
    while (*name != '\0' && i < 10)
    {
        status_text[i++] = *name++;
    }
    while (i < 10)
    {
        status_text[i++] = ' ';
    }
    status_text[10] = '\0';
#endif
}

void update_view_model(void)
{
    build_header();
    build_time();
    build_temperature();
    visible_face = choose_auto_face();
    build_status();
}

void main(void)
{
    u8 rtc_ticks = RTC_POLL_TICKS;
    u8 temp_ticks = 0;
    u8 dirty = 0;
    u8 previous_second;
    u8 old_year;
    u8 old_month;
    u8 old_day;
    u8 old_hour;
    u8 old_min;
    u8 old_temp_valid;
    int old_temp10;
    FaceState old_face;
    u8 refresh_header = 0;
    u8 refresh_time = 0;
    u8 refresh_temp = 0;
    u8 refresh_face = 0;

    release_exp08_buses();
    lcd_init_graphic();
    rtc_init();
    temperature_valid = temp_read_temperature(&temperature10);
    update_view_model();
    render_screen();

    while (1)
    {
        delay_ms(LOOP_MS);

        rtc_ticks++;
        if (rtc_ticks >= RTC_POLL_TICKS)
        {
            rtc_ticks = 0;
            previous_second = clock_now.sec;
            old_year = clock_now.year;
            old_month = clock_now.month;
            old_day = clock_now.day;
            old_hour = clock_now.hour;
            old_min = clock_now.min;
            if (rtc_read_time())
            {
                if (clock_now.sec != previous_second)
                {
                    dirty = 1;
                }
                if (clock_now.year != old_year ||
                    clock_now.month != old_month ||
                    clock_now.day != old_day)
                {
                    refresh_header = 1;
                }
                if (clock_now.hour != old_hour || clock_now.min != old_min)
                {
                    refresh_time = 1;
                }
            }
        }

        temp_ticks++;
        if (temp_ticks >= TEMP_READ_TICKS)
        {
            temp_ticks = 0;
            old_temp_valid = temperature_valid;
            old_temp10 = temperature10;
            temperature_valid = temp_read_temperature(&temperature10);
            if (temperature_valid != old_temp_valid || temperature10 != old_temp10)
            {
                refresh_temp = 1;
                dirty = 1;
            }
        }

        if (dirty)
        {
            old_face = visible_face;
            update_view_model();

            if (visible_face != old_face)
            {
                refresh_face = 1;
            }
            if (refresh_header)
            {
                render_rows(2, 8);
            }
            if (refresh_time)
            {
                render_rows(14, 27);
            }
            if (refresh_temp)
            {
                render_rows(32, 38);
            }
            if (refresh_face)
            {
                render_rows(12, 51);
            }
            if (!refresh_face)
            {
                render_rows(42, 48);
            }
            refresh_header = 0;
            refresh_time = 0;
            refresh_temp = 0;
            refresh_face = 0;
            dirty = 0;
        }
    }
}
