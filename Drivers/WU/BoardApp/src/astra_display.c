/** @file astra_display.c
 * @brief SSD1306 / PB6-PB7 / I2C1 400kHz 中断发送，不阻塞 UI。
 * 六路 UART 占用 DMA1 的所有流，本驱动使用事件/错误 IRQ，不抢占 UART DMA。
 * 画布提交复制到持久快照；传输忙时保留最新画布供下一帧提交，绝不改写在途数据。
 */
#include "astra_port.h"
#include "main.h"
#include <stdbool.h>
#include <string.h>
#define OLED_ADDRESS 0x78U
static I2C_HandleTypeDef oled_i2c;
static uint8_t staged[1024], committed[1024], packet[129];
static volatile bool busy, cache_valid, contrast_pending;
static volatile uint8_t contrast_value = 204;
static volatile int result = 1;
static unsigned page, first, last;
static uint8_t phase; /* 0=对比度，1=定位命令，2=页数据。 */
static uint32_t started;
volatile uint32_t astra_display_transfers, astra_display_errors;
volatile uint32_t astra_display_bytes, astra_display_skipped;
volatile uint32_t astra_display_frame_us, astra_display_max_us;
static uint32_t start_cycles;
static bool frame_changed;
static void fail(void)
{
    ++astra_display_errors;
    cache_valid = false;
    result = 1;
    busy = false;
}
static bool transmit(unsigned count)
{
    if (HAL_I2C_Master_Transmit_IT(&oled_i2c, OLED_ADDRESS, packet, count) != HAL_OK)
    {
        fail();
        return false;
    }
    astra_display_bytes += count;
    return true;
}
static void next_page(void)
{
    while (page < 8)
    {
        unsigned base = page * 128;
        first = 0;
        last = 127;
        if (cache_valid)
        {
            while (first < 128 && staged[base + first] == committed[base + first])
                ++first;
            if (first == 128)
            {
                ++page;
                continue;
            }
            while (last > first && staged[base + last] == committed[base + last])
                --last;
        }
        packet[0] = 0;
        packet[1] = 0xB0 | page;
        packet[2] = first & 15;
        packet[3] = 0x10 | (first >> 4);
        phase = 1;
        frame_changed = true;
        transmit(4);
        return;
    }
    cache_valid = true;
    result = 0;
    // 只统计实际发送完成的变化帧；静止画布不算屏幕刷新次数。
    if (frame_changed)
    {
        astra_display_frame_us = (DWT->CYCCNT - start_cycles) / (SystemCoreClock / 1000000U);
        if (astra_display_frame_us > astra_display_max_us)
            astra_display_max_us = astra_display_frame_us;
        ++astra_display_transfers;
    }
    busy = false;
}
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *h)
{
    if (h != &oled_i2c)
        return;
    if (phase == 0)
    {
        result = 0;
        busy = false;
        return;
    }
    if (phase == 1)
    {
        packet[0] = 0x40;
        memcpy(packet + 1, staged + page * 128 + first, last - first + 1);
        phase = 2;
        transmit(last - first + 2);
    }
    else
    {
        memcpy(committed + page * 128 + first, staged + page * 128 + first, last - first + 1);
        ++page;
        next_page();
    }
}
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *h)
{
    if (h == &oled_i2c)
        fail();
}
void I2C1_EV_IRQHandler(void)
{
    HAL_I2C_EV_IRQHandler(&oled_i2c);
}
void I2C1_ER_IRQHandler(void)
{
    HAL_I2C_ER_IRQHandler(&oled_i2c);
}
int Astra_DisplayInit(void)
{
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
    busy = false;
    cache_valid = false;
    result = 1;
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_I2C1_FORCE_RESET();
    __HAL_RCC_I2C1_RELEASE_RESET();
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = OLED_SCL_Pin | OLED_SDA_Pin;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &gpio);
    memset(&oled_i2c, 0, sizeof oled_i2c);
    oled_i2c.Instance = I2C1;
    oled_i2c.Init.ClockSpeed = 400000;
    oled_i2c.Init.DutyCycle = I2C_DUTYCYCLE_2;
    oled_i2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    oled_i2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    oled_i2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    oled_i2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&oled_i2c) != HAL_OK)
        return 1;
    uint8_t init[] = {0,    0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0,    0x40,
                      0x8D, 0x14, 0x20, 2,    0xA1, 0xC0, 0xDA, 0x12, 0x81,
                      204,  0xD9, 0xF1, 0xDB, 0x40, 0x2E, 0xA4, 0xA6, 0xAF};
    if (HAL_I2C_Master_Transmit(&oled_i2c, OLED_ADDRESS, init, sizeof init, 30) != HAL_OK)
        return 1;
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 6, 0);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
    result = 0;
    return 0;
}
int Astra_DisplayWrite(const uint8_t *buffer)
{
    if (busy)
    {
        ++astra_display_skipped;
        if (HAL_GetTick() - started > 100)
        {
            HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
            HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
            fail();
        }
        return result;
    }
    if (result)
        return result;
    busy = true;
    started = HAL_GetTick();
    start_cycles = DWT->CYCCNT;
    if (contrast_pending)
    {
        uint32_t mask = __get_PRIMASK();
        __disable_irq();
        packet[0] = 0;
        packet[1] = 0x81;
        packet[2] = contrast_value;
        contrast_pending = false;
        __set_PRIMASK(mask);
        phase = 0;
        transmit(3);
        return result;
    }
    memcpy(staged, buffer, 1024);
    frame_changed = false;
    page = 0;
    next_page();
    return result;
}
int Astra_DisplayContrast(uint8_t value)
{
    contrast_value = value;
    contrast_pending = true;
    return 0;
}
