/* STM32F407 / SSD1306 128x64, PB6 SCL / PB7 SDA, software I2C.
 * Address and initialization derive from the project's original U8g2 driver.
 * KK_OLED graphics and its buffer/commit contract are unchanged.
 */
#include "kk_oled_driver.h"
#include "kk_oled_internal.h"
#include "main.h"

#define OLED_ADDRESS_WRITE 0x78U
static bool driver_busy;
volatile uint32_t kk_oled_transfer_count;
volatile uint32_t kk_oled_error_count;

static void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = (SystemCoreClock / 1000000U) * us;
    while ((uint32_t)(DWT->CYCCNT - start) < cycles) { __NOP(); }
}
static void sda(bool high)
{
    OLED_SDA_GPIO_Port->BSRR = high ? OLED_SDA_Pin : ((uint32_t)OLED_SDA_Pin << 16U);
}
static void scl_low(void) { OLED_SCL_GPIO_Port->BSRR = (uint32_t)OLED_SCL_Pin << 16U; }
static bool scl_high(void)
{
    uint32_t start = DWT->CYCCNT;
    OLED_SCL_GPIO_Port->BSRR = OLED_SCL_Pin;
    while (!(OLED_SCL_GPIO_Port->IDR & OLED_SCL_Pin)) {
        if ((uint32_t)(DWT->CYCCNT - start) > SystemCoreClock / 1000U) return false;
    }
    delay_us(2U);
    return true;
}
static void stop(void)
{
    scl_low(); sda(false); delay_us(2U);
    (void)scl_high(); sda(true); delay_us(2U);
}
static bool send_byte(uint8_t value)
{
    for (uint8_t bit = 0U; bit < 8U; ++bit) {
        scl_low(); sda((value & 0x80U) != 0U); delay_us(2U);
        if (!scl_high()) return false;
        value <<= 1U;
    }
    scl_low(); sda(true); delay_us(2U);
    if (!scl_high()) return false;
    bool ack = (OLED_SDA_GPIO_Port->IDR & OLED_SDA_Pin) == 0U;
    scl_low();
    return ack;
}
static OLED_Status send(uint8_t control, const uint8_t *data, uint16_t size)
{
    sda(true); delay_us(2U);
    if (!scl_high() || !(OLED_SDA_GPIO_Port->IDR & OLED_SDA_Pin)) goto error;
    sda(false); delay_us(2U); scl_low();
    if (!send_byte(OLED_ADDRESS_WRITE) || !send_byte(control)) goto error;
    for (uint16_t i = 0U; i < size; ++i) {
        if (!send_byte(data[i])) goto error;
    }
    stop();
    ++kk_oled_transfer_count;
    return OLED_OK;
error:
    stop();
    ++kk_oled_error_count;
    return OLED_ERROR;
}
static OLED_Status set_page(uint8_t page, uint8_t column)
{
    const uint8_t command[] = { (uint8_t)(0xB0U | page),
        (uint8_t)(column & 15U), (uint8_t)(0x10U | (column >> 4U)) };
    return send(0x00U, command, sizeof(command));
}
OLED_Status OLED_DriverInit(void)
{
    if (driver_busy) return OLED_BUSY;
    driver_busy = true;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /* Release both lines before switching to open drain. External pull-ups on
       the OLED module are retained; internal pull-ups assist the idle state. */
    sda(true);
    OLED_SCL_GPIO_Port->BSRR = OLED_SCL_Pin;
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = OLED_SCL_Pin | OLED_SDA_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_Delay(20U);
    /* Recover an interrupted transaction, without disabling RTOS interrupts. */
    for (uint8_t i = 0U; i < 9U; ++i) {
        scl_low(); delay_us(2U); (void)scl_high();
    }
    stop();
    /* Same supply/timing configuration as original SSD1306 setup. Page mode
       replaces horizontal mode. C0 reproduces its U8G2_MIRROR_VERTICAL view. */
    static const uint8_t init[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x02, 0xA1, 0xC0, 0xDA, 0x12,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0x2E, 0xA4, 0xA6
    };
    static const uint8_t zeros[OLED_PHYSICAL_WIDTH] = {0};
    OLED_Status status = send(0x00U, init, sizeof(init));
    for (uint8_t page = 0U; status == OLED_OK && page < OLED_PHYSICAL_PAGES; ++page) {
        status = set_page(page, 0U);
        if (status == OLED_OK) status = send(0x40U, zeros, sizeof(zeros));
    }
    const uint8_t on = 0xAFU;
    if (status == OLED_OK) status = send(0x00U, &on, 1U);
    driver_busy = false;
    return status;
}
OLED_Status OLED_DriverWriteBlocking(void)
{
    if (driver_busy) return OLED_BUSY;
    driver_busy = true;
    OLED_Status status = OLED_OK;
    const uint8_t *buffer = OLED_InternalGetTransferBuffer();
    for (uint8_t page = 0U; page < OLED_PHYSICAL_PAGES; ++page) {
        uint8_t first = OLED_InternalGetTransferMinX(page);
        uint8_t last = OLED_InternalGetTransferMaxX(page);
        if (first >= OLED_PHYSICAL_WIDTH) continue;
        if (last < first || last >= OLED_PHYSICAL_WIDTH) { status = OLED_ERROR; break; }
        status = set_page(page, first);
        if (status == OLED_OK) status = send(0x40U,
            buffer + (uint16_t)page * OLED_PHYSICAL_WIDTH + first,
            (uint16_t)last - first + 1U);
        if (status != OLED_OK) break;
    }
    driver_busy = false;
    return status;
}
OLED_Status OLED_DriverWriteIT(void) { return OLED_UNSUPPORTED; }
OLED_Status OLED_DriverWriteDMA(void) { return OLED_UNSUPPORTED; }
bool OLED_DriverIsBusy(void) { return driver_busy; }
OLED_Status OLED_DriverSetContrast(uint8_t value)
{
    if (driver_busy) return OLED_BUSY;
    const uint8_t command[] = {0x81U, value};
    return send(0x00U, command, sizeof(command));
}
OLED_Status OLED_DriverSetPowerSave(bool enable)
{
    if (driver_busy) return OLED_BUSY;
    const uint8_t command = enable ? 0xAEU : 0xAFU;
    return send(0x00U, &command, 1U);
}
void OLED_DriverHandleMemTxComplete(void) { /* No asynchronous software I2C. */ }
void OLED_DriverHandleError(void) { /* Synchronous errors return to the core. */ }
