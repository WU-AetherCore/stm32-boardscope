/** @file astra_editor.cpp
 * @brief 128×64 编辑弹窗，数值和提示独立分区，最大 Flash 地址也不溢出。
 */
#include "hal/hal.h"
#include "astra_text.h"
#include <cstdio>
#include <algorithm>
extern const unsigned char board_small_font[];
extern "C" void Astra_DrawEditor(int kind, int value)
{
    const char *title = kind == 1   ? "选择串口"
                        : kind == 2 ? "屏幕亮度"
                        : kind == 3 ? "温度上限"
                                    : "Flash地址";
    char number[32];
    if (kind == 4)
        snprintf(number, sizeof number, "0x%06X", (unsigned)value);
    else
        snprintf(number, sizeof number, "%d%s", value, kind == 2 ? " %" : kind == 3 ? " C" : "");
    HAL::setDrawType(0);
    HAL::drawRBox(3, 2, 122, 60, 3);
    HAL::setDrawType(1);
    HAL::drawRFrame(3, 2, 122, 60, 3);
    std::string heading(title), numeric(number);
    HAL::drawChinese((128 - HAL::getFontWidth(heading)) / 2, 17, heading);
    // 框宽来自数值实际宽度，上限112，下限58；左右至少8像素。
    int width = std::max(58, std::min(112, (int)HAL::getFontWidth(numeric) + 16));
    HAL::drawRFrame((128 - width) / 2, 21, width, 22, 2);
    HAL::drawChinese((128 - HAL::getFontWidth(numeric)) / 2, 37, numeric);
    HAL::drawChinese(8, 57, "短按保存");
    HAL::drawChinese(68, 57, "长按取消");
}
