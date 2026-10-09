/** @file astra_text.c
 * @brief 按真实字模测宽，完整保留大数字；不足时缩短标签，禁止文字越过容器。
 */
#include "astra_text.h"
#include <string.h>
extern const uint8_t board_font[], board_small_font[];
static unsigned next_char(const char *s, unsigned i)
{
    ++i;
    while (s[i] && ((unsigned char)s[i] & 0xc0) == 0x80)
        ++i;
    return i;
}
static void shorten(u8g2_t *c, const char *text, unsigned limit, int width, char *out)
{
    unsigned at = 0, good = 0;
    if (limit < 128)
    {
        memcpy(out, text, limit);
        out[limit] = 0;
        if (u8g2_GetUTF8Width(c, out) <= width)
            return;
    }
    out[0] = 0;
    while (at < limit && text[at])
    {
        unsigned next = next_char(text, at);
        if (next > limit || next + 3 >= 128)
            break;
        memcpy(out, text, next);
        memcpy(out + next, "..", 3);
        if (u8g2_GetUTF8Width(c, out) > width)
            break;
        good = next;
        at = next;
    }
    if (!good)
    {
        out[0] = 0;
        return;
    }
    memcpy(out, text, good);
    memcpy(out + good, "..", 3);
}
static int ascii(const char *s)
{
    for (; *s; ++s)
        if ((unsigned char)*s >= 128)
            return 0;
    return 1;
}
void Astra_TextFit(u8g2_t *c, int x, int y, const char *text, int width)
{
    if (x < 0 || x >= 128 || y <= 0 || y > 76 || width <= 0)
        return;
    if (width > 128 - x)
        width = 128 - x;
    const uint8_t *font = c->font;
    u8g2_SetClipWindow(c, x, 0, x + width, 64);
    if (u8g2_GetUTF8Width(c, text) <= width)
        u8g2_DrawUTF8(c, x, y, text);
    else if (ascii(text))
    {
        u8g2_SetFont(c, board_small_font);
        if (u8g2_GetUTF8Width(c, text) <= width)
            u8g2_DrawUTF8(c, x, y - 2, text);
        else
        {
            char fit[128];
            shorten(c, text, strlen(text), width, fit);
            u8g2_DrawUTF8(c, x, y - 2, fit);
        }
    }
    else
    {
        const char *colon = strchr(text, ':');
        if (colon && colon[1])
        {
            const char *value = colon + 1;
            int value_width = u8g2_GetUTF8Width(c, value);
            const uint8_t *value_font = font;
            if (ascii(value))
            {
                value_font = board_small_font;
                u8g2_SetFont(c, value_font);
                value_width = u8g2_GetUTF8Width(c, value);
                u8g2_SetFont(c, font);
            }
            int label_width = width - value_width - 2;
            char label[128];
            shorten(c, text, colon - text + 1, label_width, label);
            u8g2_DrawUTF8(c, x, y, label);
            u8g2_SetFont(c, value_font);
            u8g2_DrawUTF8(c, x + width - value_width, y - (value_font == board_small_font ? 2 : 0),
                          value);
        }
        else
        {
            char fit[128];
            shorten(c, text, strlen(text), width, fit);
            u8g2_DrawUTF8(c, x, y, fit);
        }
    }
    u8g2_SetFont(c, font);
    u8g2_SetMaxClipWindow(c);
}
