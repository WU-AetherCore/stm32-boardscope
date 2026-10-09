/** 固定屏幕内的 UTF-8 字宽适配；优先保留冒号后的数值。 */
#ifndef ASTRA_TEXT_H
#define ASTRA_TEXT_H
#include "u8g2.h"
#ifdef __cplusplus
extern "C"
{
#endif
    void Astra_TextFit(u8g2_t *canvas, int x, int baseline, const char *text, int width);
    void Astra_DrawEditor(int kind, int value);
#ifdef __cplusplus
}
#endif
#endif
