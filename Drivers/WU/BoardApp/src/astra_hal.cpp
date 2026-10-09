/** @file astra_hal.cpp
 * @brief F407 的 Astra HAL：U8g2 图形、硬件 I2C1 中断发送和 FreeRTOS 时间接口。
 * 仅 UI 任务访问画布，串口/业务任务不触碰显示缓冲区。
 */
#include "hal/hal.h"
extern "C"
{
#include "main.h"
#include "cmsis_os.h"
#include "astra_port.h"
#include "FreeRTOS.h"
#include "task.h"
#include "u8g2.h"
#include "astra_text.h"
    extern const uint8_t board_font[];
    uint8_t astra_framebuffer[1024];
    volatile uint32_t board_ui_heartbeat, board_ui_stack_free_words;
    volatile uint32_t board_oled_status = 1, board_ui_status = 1;
    volatile uint32_t astra_page, astra_selection, astra_alloc_failures;
}
#include <new>
#include <cstdlib>
#include <algorithm>
#include <cstring>
// C++ 容器和 FreeRTOS 共用受保护堆，避免独立 _sbrk 堆互相侵占。
void *operator new(std::size_t n)
{
    if (void *p = pvPortMalloc(n ? n : 1))
        return p;
    ++astra_alloc_failures;
    board_ui_status = 2;
    for (;;)
        osDelay(1000); // 保留通信任务运行，让电脑仍可看到内存故障。
}
void *operator new[](std::size_t n)
{
    return ::operator new(n);
}
void operator delete(void *p) noexcept
{
    vPortFree(p);
}
void operator delete[](void *p) noexcept
{
    vPortFree(p);
}
void operator delete(void *p, std::size_t) noexcept
{
    vPortFree(p);
}
void operator delete[](void *p, std::size_t) noexcept
{
    vPortFree(p);
}
class HALBoard final : public HAL
{
    u8g2_t canvas{};
    static u8g2_uint_t coord(float v)
    {
        return static_cast<int16_t>(v);
    }
    static uint8_t dummy(u8x8_t *, uint8_t, uint8_t, void *)
    {
        return 1;
    }

  public:
    void init() override
    {
        u8g2_Setup_ssd1306_128x64_noname_f(&canvas, U8G2_R0, dummy, dummy);
        u8g2_SetFontMode(&canvas, 1);
        u8g2_SetFont(&canvas, board_font);
        config.screenWeight = 128;
        config.screenHeight = 64;
    }
    void *_getCanvasBuffer() override
    {
        return u8g2_GetBufferPtr(&canvas);
    }
    unsigned char _getBufferTileHeight() override
    {
        return 8;
    }
    unsigned char _getBufferTileWidth() override
    {
        return 16;
    }
    void _canvasUpdate() override
    {
        memcpy(astra_framebuffer, u8g2_GetBufferPtr(&canvas), 1024);
        board_oled_status = Astra_DisplayWrite(astra_framebuffer);
    }
    void _canvasClear() override
    {
        u8g2_ClearBuffer(&canvas);
    }
    void _setFont(const unsigned char *f) override
    {
        u8g2_SetFont(&canvas, f);
    }
    unsigned char _getFontWidth(std::string &s) override
    {
        return std::min<unsigned>(255, u8g2_GetUTF8Width(&canvas, s.c_str()));
    }
    unsigned char _getFontHeight() override
    {
        return 13;
    }
    void _setDrawType(unsigned char t) override
    {
        u8g2_SetDrawColor(&canvas, t);
    }
    void _drawPixel(float x, float y) override
    {
        u8g2_DrawPixel(&canvas, coord(x), coord(y));
    }
    void _drawEnglish(float x, float y, const std::string &s) override
    {
        _drawChinese(x, y, s);
    }
    void _drawChinese(float x, float y, const std::string &s) override
    {
        Astra_TextFit(&canvas, (int)x, (int)y, s.c_str(), std::min(116, 128 - (int)x));
    }
    void _drawVLine(float x, float y, float h) override
    {
        if (h > 0)
            u8g2_DrawVLine(&canvas, coord(x), coord(y), coord(h));
    }
    void _drawHLine(float x, float y, float w) override
    {
        if (w > 0)
            u8g2_DrawHLine(&canvas, coord(x), coord(y), coord(w));
    }
    void _drawVDottedLine(float x, float y, float h) override
    {
        for (int i = 0; i < h; i += 2)
            _drawPixel(x, y + i);
    }
    void _drawHDottedLine(float x, float y, float w) override
    {
        for (int i = 0; i < w; i += 2)
            _drawPixel(x + i, y);
    }
    void _drawBMP(float x, float y, float w, float h, const unsigned char *b) override
    {
        u8g2_DrawXBM(&canvas, coord(x), coord(y), coord(w), coord(h), b);
    }
    void _drawBox(float x, float y, float w, float h) override
    {
        if (w > 0 && h > 0)
            u8g2_DrawBox(&canvas, coord(x), coord(y), coord(w), coord(h));
    }
    void _drawFrame(float x, float y, float w, float h) override
    {
        if (w > 0 && h > 0)
            u8g2_DrawFrame(&canvas, coord(x), coord(y), coord(w), coord(h));
    }
    void _drawRBox(float x, float y, float w, float h, float r) override
    {
        if (w > 0 && h > 0)
            u8g2_DrawRBox(&canvas, coord(x), coord(y), coord(w), coord(h), coord(r));
    }
    void _drawRFrame(float x, float y, float w, float h, float r) override
    {
        if (w > 0 && h > 0)
            u8g2_DrawRFrame(&canvas, coord(x), coord(y), coord(w), coord(h), coord(r));
    }
    unsigned long _millis() override
    {
        return HAL_GetTick();
    }
    unsigned long _getTick() override
    {
        return HAL_GetTick();
    }
    void _delay(unsigned long ms) override
    {
        osDelay(ms ? ms : 1);
    }
    void _keyScan() override
    {
    } // 上游双按键扫描禁用；编码器由任务统一读取。
};
void Astra_InjectHAL()
{
    static HALBoard board;
    HAL::inject(&board);
}
