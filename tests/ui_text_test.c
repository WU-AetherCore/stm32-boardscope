/** 使用真实 U8g2/板载字模验证裁剪边界和完整数字，而非模拟文字宽度。 */
#include "astra_text.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
extern const uint8_t board_font[], board_small_font[];
// 本测试只绘字；上游交互控件的链接入口没有连接任何实体按键。
uint8_t u8x8_GetMenuEvent(u8x8_t *u)
{
    (void)u;
    return 0;
}
static uint8_t dummy(u8x8_t *u, uint8_t m, uint8_t n, void *p)
{
    (void)u;
    (void)m;
    (void)n;
    (void)p;
    return 1;
}
static int pixel(u8g2_t *c, int x, int y)
{
    return (u8g2_GetBufferPtr(c)[(y / 8) * 128 + x] >> (y % 8)) & 1;
}
int main(void)
{
    u8g2_t actual, expected;
    u8g2_Setup_ssd1306_128x64_noname_f(&actual, U8G2_R0, dummy, dummy);
    u8g2_Setup_ssd1306_128x64_noname_f(&expected, U8G2_R0, dummy, dummy);
    const char *cases[] = {"串口发送完成字节:4294967295", "采样成功距今:4294967295 ms",
                           "堆空闲:65536 B", "Flash地址:0xFFFFF0"};
    for (unsigned i = 0; i < sizeof cases / sizeof *cases; ++i)
    {
        u8g2_ClearBuffer(&actual);
        u8g2_ClearBuffer(&expected);
        u8g2_SetFont(&actual, board_font);
        Astra_TextFit(&actual, 4, 14, cases[i], 116);
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 128; ++x)
                if (x < 4 || x >= 120)
                    assert(!pixel(&actual, x, y));
        // 超宽中文标签应缩短，右侧数值必须与独立绘制的原字符串逐像素相同。
        if (i < 2)
        {
            const char *value = strchr(cases[i], ':') + 1;
            u8g2_SetFont(&expected, board_small_font);
            int left = 120 - u8g2_GetUTF8Width(&expected, value);
            u8g2_DrawUTF8(&expected, left, 12, value);
            for (int y = 0; y < 64; ++y)
                for (int x = left; x < 120; ++x)
                    assert(pixel(&actual, x, y) == pixel(&expected, x, y));
        }
        assert(actual.font == board_font);
    }
    puts("PASS: UTF-8 clipping and complete uint32 values");
    return 0;
}
