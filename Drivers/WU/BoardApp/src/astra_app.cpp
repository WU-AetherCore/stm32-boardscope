/** @file astra_app.cpp
 * @brief V1.2 Astra 数据应用：原生磁贴/列表、编码器、实时数据和业务动作。
 * 所有菜单在启动时创建，动态标题预留空间。任何外设操作均在 UI 任务。
 * 短按进入/执行，长按返回；继电器先进入确认页，默认选中取消。
 */
#include "astra/ui/launcher.h"
extern "C"
{
#include "board.h"
#include "board_memory.h"
#include "astra_port.h"
#include "astra_text.h"
#include "cmsis_os.h"
#include "tim.h"
#include "can.h"
#include "../../wu_can/wu_can.h"
    extern unsigned char buzzer_flag_dat1, cs_bj_flag_bit;
}
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <algorithm>
void Astra_InjectHAL();
extern "C"
{
    volatile uint32_t astra_ui_frame_us, astra_ui_max_us;
}
namespace
{
enum Act
{
    INFO,
    BACK,
    RELAY,
    CONFIRM,
    CANCEL,
    PORT,
    DIRECTION,
    HEX,
    PAUSE,
    OLDER,
    SEND,
    ECHO,
    CLEAR,
    ADDRESS,
    NEXT,
    PREV,
    BRIGHT,
    LIMIT,
    ALARM,
    BUZZER
};
struct Binding
{
    astra::Menu *node;
    Act action;
    unsigned parameter;
};
Binding bindings[128];
unsigned bindingCount;
astra::Launcher launcher;
astra::Tile *home;
astra::List *sensor, *relay, *uart, *flash, *keys, *systemPage, *settings, *memoryPage, *canPage,
    *gpioPage, *confirm;
unsigned port = 0, offset = 0;
bool tx = false, hex = false, paused = false;
uint32_t address = 0, lastData = 0;
unsigned pendingRelay = 0;
bool pendingRelayValue = false;
int edit = 0, editValue = 0;
unsigned char brightness = 80;
unsigned char *relayFlags[] = {&relay1_flag_dat, &relay2_flag_dat, &relay3_flag_dat,
                               &relay4_flag_dat, &relay5_flag_dat};
void add(astra::Menu *parent, const char *label, Act action = INFO, unsigned param = 0)
{
    auto *n = new astra::List(label);
    n->pic.clear();
    n->pic.shrink_to_fit(); // 数据行不需要占用 120 B 图标。
    n->title.reserve(80);
    parent->addItem(n);
    bindings[bindingCount++] = {n, action, param};
}
void row(astra::Menu *page, unsigned index, const char *fmt, ...)
{
    char text[100];
    va_list args;
    va_start(args, fmt);
    vsnprintf(text, sizeof text, fmt, args);
    va_end(args);
    page->childMenu[index]->title = text;
}
astra::List *section(const char *title, unsigned icon)
{
    auto *p = new astra::List(title);
    // 原生 Astra 图标：30×30 XBM，每行 4 字节，绘制简洁的数据面板图案。
    p->pic.assign(120, 0);
    for (unsigned y = 0; y < 30; ++y)
        for (unsigned x = 0; x < 30; ++x)
        {
            bool set =
                (x == 2 || x == 27 || y == 2 || y == 27) && x >= 2 && x <= 27 && y >= 2 && y <= 27;
            if (icon % 3 == 0)
                set |= x > 6 && x < 23 && (y == 9 || y == 15 || y == 21);
            if (icon % 3 == 1)
                set |= (x == 8 && y > 17 && y < 25) || (x == 14 && y > 11 && y < 25) ||
                       (x == 20 && y > 6 && y < 25);
            if (icon % 3 == 2)
                set |= y == (15 + (x % 8 < 4 ? 4 : -4)) && x > 5 && x < 25;
            if (set)
                p->pic[y * 4 + x / 8] |= 1U << (x % 8);
        }
    home->addItem(p);
    return p;
}
void back()
{
    if (edit)
    {
        edit = 0;
        return;
    }
    auto *p = launcher.getCurrentMenu()->parent;
    if (p)
        launcher.jump(p);
}
void updateData(uint32_t now)
{
    if (now - lastData < 250)
        return;
    lastData = now;
    BoardMemory m;
    Board_MemorySnapshot(&m);
    row(sensor, 0, "温度:%u C", Temperature);
    row(sensor, 1, "湿度:%u %%", Humidity);
    row(sensor, 2, "采样:%s", board_dht_ok ? "有效" : "失败");
    row(sensor, 3, "采样龄:%lu ms", board_dht_last ? (unsigned long)(now - board_dht_last) : 0UL);
    row(sensor, 4, "上限:%u C", Temperature_max);
    row(sensor, 5, "报警:%s", T_alarm ? "触发" : "正常");
    row(sensor, 6, "电压预设:11.82 V");
    for (unsigned i = 0; i < 5; ++i)
        row(relay, i, "继电器%u:%s", i + 1, *relayFlags[i] ? "开启" : "关闭");
    row(uart, 0, "选择串口:UART%u", port + 1);
    row(uart, 1, "方向:%s", tx ? "TX" : "RX");
    row(uart, 2, "格式:%s", hex ? "HEX" : "ASCII");
    row(uart, 3, "数据:%s", paused ? "暂停" : "实时");
    row(uart, 4, "历史偏移:%u B", offset);
    if (!paused)
        for (unsigned i = 0; i < 4; ++i)
        {
            char h[22];
            Board_History(port, tx, offset + (3 - i) * (hex ? 6 : 20), hex, h, sizeof h);
            row(uart, 5 + i, "%s", h);
        }
    row(uart, 9, "RX:%lu B", (unsigned long)board_rx[port]);
    row(uart, 10, "TX:%lu B", (unsigned long)board_tx[port]);
    row(uart,11,"TX丢包:%lu",(unsigned long)board_drop[port]);
    row(uart,12,"RX丢包:%lu",(unsigned long)board_rx_drop[port]);
    row(uart,13,"错误次数:%lu",(unsigned long)board_uart_errors[port]);
    row(uart,14,"发送失败:%lu",(unsigned long)board_tx_failed[port]);
    row(uart,15,"排队:%u 包",Board_UartPending(port));
    row(uart,17,"回显:%s",board_uart_echo[port]?"开启":"关闭");
    row(flash, 0, "连接:%s", board_flash_ok ? "正常" : "失败");
    row(flash, 1, "ID:%06lX", (unsigned long)board_flash_id);
    row(flash, 2, "容量:%lu MB", (unsigned long)(board_flash_bytes / 1048576));
    row(flash, 3, "地址:%06lX", (unsigned long)address);
    for (unsigned i = 0; i < 4; ++i)
        row(flash, 4 + i, "%02X %02X %02X %02X", board_flash_data[i * 4],
            board_flash_data[i * 4 + 1], board_flash_data[i * 4 + 2], board_flash_data[i * 4 + 3]);
    row(flash, 8, "状态:%02X", board_flash_status);
    for (unsigned i = 0; i < 3; ++i)
        for (unsigned j = 0; j < 3; ++j)
            row(keys, i * 3 + j, "按键%u%s:%lu", i + 1,
                j == 0   ? "短按"
                : j == 1 ? "长按"
                         : "双击",
                (unsigned long)board_key_events[i][j]);
    row(systemPage, 0, "运行:%lu s", (unsigned long)(now / 1000));
    row(systemPage, 1, "UI帧:%lu", (unsigned long)board_ui_heartbeat);
    row(systemPage, 2, "UI栈:%lu B", (unsigned long)(board_ui_stack_free_words * 4));
    row(systemPage, 3, "显示:%s", board_oled_status ? "异常" : "正常");
    row(canPage, 0, "接收:%lu 帧", (unsigned long)board_can_rx);
    row(canPage, 1, "ID:%08lX", (unsigned long)RxID);
    row(canPage, 2, "长度:%u B", RxLength);
    row(canPage, 3, "错误:%08lX", (unsigned long)HAL_CAN_GetError(&hcan1));
    for (unsigned i = 0; i < 2; ++i)
        row(canPage, 4 + i, "%02X %02X %02X %02X", RxData[i * 4], RxData[i * 4 + 1],
            RxData[i * 4 + 2], RxData[i * 4 + 3]);
    GPIO_TypeDef *gp[] = {GPIOA, GPIOB, GPIOC, GPIOD, GPIOE};
    for (unsigned i = 0; i < 5; ++i)
        row(gpioPage, i, "%c I:%04lX O:%04lX", 'A' + i, (unsigned long)gp[i]->IDR,
            (unsigned long)gp[i]->ODR);
    uint32_t vals[] = {m.flash_used, m.flash_total, m.ram_used,      m.ram_total, m.ccm_used,
                       m.ccm_total,  m.text,        m.data,          m.bss,       m.dec,
                       m.heap_total, m.heap_free,   m.heap_min_free, m.task_count};
    const char *names[] = {"FLASH已用", "FLASH总量", "RAM已用",    "RAM总量", "CCM已用",
                           "CCM总量",   "text",      "data",       "bss",     "dec",
                           "堆总量",    "堆空闲",    "堆最低空闲", "任务数"};
    for (unsigned i = 0; i < 14; ++i)
        row(memoryPage, i, "%s:%lu", names[i], (unsigned long)vals[i]);
    row(memoryPage, 14, "hex:%lX", (unsigned long)m.dec);
    row(settings, 0, "亮度:%u %%", brightness);
    row(settings, 1, "温度上限:%u C", Temperature_max);
    row(settings, 2, "报警:%s", !cs_bj_flag_bit ? "开启" : "关闭");
    row(settings, 3, "蜂鸣器:%s", buzzer_flag_dat1 ? "开启" : "关闭");
    if (launcher.getCurrentMenu()->getType() == "List")
        launcher.getSelector()->setPosition();
}
Binding *selected()
{
    auto *page = launcher.getCurrentMenu();
    auto *n = page->childMenu[page->selectIndex];
    for (unsigned i = 0; i < bindingCount; ++i)
        if (bindings[i].node == n)
            return &bindings[i];
    return nullptr;
}
void rotate(int delta)
{
    if (edit)
    {
        int low = edit == PORT ? 0 : edit == BRIGHT ? 10 : 0;
        int high = edit == PORT      ? 5
                   : edit == BRIGHT  ? 100
                   : edit == ADDRESS ? (int)(board_flash_bytes ? board_flash_bytes - 16 : 0)
                                     : 99;
        editValue = std::max(low, std::min(high, editValue + delta * (edit == ADDRESS  ? 16
                                                                      : edit == BRIGHT ? 5
                                                                                       : 1)));
        return;
    }
    while (delta > 0)
    {
        launcher.getSelector()->goNext();
        --delta;
    }
    while (delta < 0)
    {
        launcher.getSelector()->goPreview();
        ++delta;
    }
}
void click()
{
    if (edit)
    {
        if (edit == PORT)
        {
            port = editValue;
            offset = 0;
        }
        if (edit == BRIGHT)
        {
            brightness = editValue;
            Astra_DisplayContrast(brightness * 255 / 100);
        }
        if (edit == LIMIT)
            Temperature_max = editValue;
        if (edit == ADDRESS)
        {
            address = editValue;
            Board_FlashRead(address);
        }
        edit = 0;
        lastData = 0;
        return;
    }
    auto *b = selected();
    if (!b)
    {
        launcher.open();
        return;
    }
    switch (b->action)
    {
    case INFO:
        back();
        break;
    case BACK:
        back();
        break;
    case RELAY:
        pendingRelay = b->parameter;
        pendingRelayValue = !*relayFlags[pendingRelay];
        confirm->parent = relay;
        confirm->title = "继电器确认";
        row(confirm, 0, "确定:%s继电器%u", pendingRelayValue ? "开启" : "关闭", pendingRelay + 1);
        confirm->selectIndex = 1;
        launcher.jump(confirm);
        break;
    case CONFIRM:
        *relayFlags[pendingRelay] = pendingRelayValue;
        launcher.jump(relay);
        break;
    case CANCEL:
        launcher.jump(relay);
        break;
    case PORT:
        edit = PORT;
        editValue = port;
        break;
    case DIRECTION:
        tx = !tx;
        offset = 0;
        break;
    case HEX:
        hex = !hex;
        break;
    case PAUSE:
        paused = !paused;
        break;
    case OLDER:
        offset = (offset + 20) > 236 ? 0 : offset + 20;
        break;
    case SEND:
    {
        const char *text = "BoardScope V1.2 UART test\r\n";
        Board_Send(port, (const uint8_t *)text, strlen(text));
        break;
    }
    case ECHO:
        board_uart_echo[port] = !board_uart_echo[port];
        break;
    case CLEAR:
        Board_UartClearHistory(port);
        offset = 0;
        break;
    case ADDRESS:
        edit = ADDRESS;
        editValue = address;
        break;
    case NEXT:
        if (board_flash_bytes >= 32 && address <= board_flash_bytes - 32)
            address += 16;
        Board_FlashRead(address);
        break;
    case PREV:
        if (address >= 16)
            address -= 16;
        Board_FlashRead(address);
        break;
    case BRIGHT:
        edit = BRIGHT;
        editValue = brightness;
        break;
    case LIMIT:
        edit = LIMIT;
        editValue = Temperature_max;
        break;
    case ALARM:
        cs_bj_flag_bit = !cs_bj_flag_bit;
        break;
    case BUZZER:
        buzzer_flag_dat1 = !buzzer_flag_dat1;
        break;
    }
    lastData = 0;
}
void init()
{
    Astra_InjectHAL();
    auto &cfg = astra::getUIConfig();
    cfg.mainFont = board_font;
    cfg.listLineHeight = 16;
    cfg.listTextHeight = 9;
    cfg.tileTitleHeight = 8;
    cfg.fadeAnimationSpeed = 15;
    // 软件 I2C 帧率低于上游 SPI，缩短展开动画的收敛时间。
    cfg.listAnimationSpeed = 95;
    cfg.tileAnimationSpeed = 90;
    cfg.cameraAnimationSpeed = 90;
    cfg.selectorYAnimationSpeed = 90;
    cfg.selectorXAnimationSpeed = 90;
    cfg.selectorWidthAnimationSpeed = 90;
    cfg.selectorHeightAnimationSpeed = 90;
    home = new astra::Tile("BoardScope V1.2");
    sensor = section("温湿度", 0);
    relay = section("继电器", 1);
    uart = section("串口监测", 2);
    flash = section("SPI Flash", 0);
    keys = section("按键事件", 1);
    systemPage = section("设备状态", 2);
    memoryPage = section("内存状态", 1);
    settings = section("设置", 0);
    for (unsigned i = 0; i < 7; ++i)
        add(sensor, "读取中");
    add(sensor, "返回", BACK);
    for (unsigned i = 0; i < 5; ++i)
        add(relay, "继电器", RELAY, i);
    add(relay, "返回", BACK);
    add(uart, "串口", PORT);
    add(uart, "方向", DIRECTION);
    add(uart, "格式", HEX);
    add(uart, "暂停", PAUSE);
    add(uart, "浏览历史", OLDER);
    for (unsigned i = 0; i < 11; ++i)
        add(uart, "数据");
    add(uart, "发送测试", SEND);
    add(uart, "回显", ECHO);
    add(uart, "清除历史", CLEAR);
    add(uart, "返回", BACK);
    for (unsigned i = 0; i < 9; ++i)
        add(flash, "Flash", i == 3 ? ADDRESS : INFO);
    add(flash, "下一页", NEXT);
    add(flash, "上一页", PREV);
    add(flash, "返回", BACK);
    for (unsigned i = 0; i < 9; ++i)
        add(keys, "事件");
    add(keys, "返回", BACK);
    for (unsigned i = 0; i < 4; ++i)
        add(systemPage, "系统");
    canPage = new astra::List("CAN数据");
    gpioPage = new astra::List("GPIO状态");
    systemPage->addItem(canPage);
    systemPage->addItem(gpioPage);
    add(systemPage, "返回", BACK);
    for (unsigned i = 0; i < 6; ++i)
        add(canPage, "CAN");
    add(canPage, "返回", BACK);
    for (unsigned i = 0; i < 5; ++i)
        add(gpioPage, "GPIO");
    add(gpioPage, "返回", BACK);
    for (unsigned i = 0; i < 15; ++i)
        add(memoryPage, "内存");
    add(memoryPage, "返回", BACK);
    add(settings, "亮度", BRIGHT);
    add(settings, "上限", LIMIT);
    add(settings, "报警", ALARM);
    add(settings, "蜂鸣器", BUZZER);
    add(settings, "返回", BACK);
    confirm = new astra::List("确认");
    add(confirm, "确定", CONFIRM);
    add(confirm, "取消", CANCEL);
    launcher.init(home);
    Board_FlashRead(0);
    board_ui_status = 0;
    lastData = HAL_GetTick() - 250;
    updateData(HAL_GetTick());
}
void renderEdit()
{
    if (!edit)
        return;
    int kind = edit == PORT ? 1 : edit == BRIGHT ? 2 : edit == LIMIT ? 3 : 4;
    Astra_DrawEditor(kind, edit == PORT ? editValue + 1 : editValue);
}
} // namespace
extern "C" void OLED_WUStartTask04(void const *argument)
{
    (void)argument;
    while (Astra_DisplayInit() != 0)
        osDelay(500);
    board_oled_status = 0;
    init();
    uint16_t previous = __HAL_TIM_GET_COUNTER(&htim3);
    int remainder = 0;
    bool raw = false, down = false, longSent = false;
    uint32_t changed = 0, pressed = 0, lastRecovery = 0;
    TickType_t wake = xTaskGetTickCount();
    for (;;)
    {
        uint32_t now = HAL_GetTick();
        astra::Animation::beginFrame(now);
        uint32_t frameStart = DWT->CYCCNT;
        if (board_oled_status && now - lastRecovery >= 1000)
        {
            lastRecovery = now;
            board_oled_status = Astra_DisplayInit();
            if (!board_oled_status)
                Astra_DisplayContrast(brightness * 255 / 100);
        }
        Board_Service();
        uint16_t count = __HAL_TIM_GET_COUNTER(&htim3);
        remainder += (int16_t)(uint16_t)(count - previous);
        previous = count;
        int delta = remainder / 2;
        remainder %= 2;
        if (board_debug_turn)
        {
            delta += board_debug_turn;
            board_debug_turn = 0;
        }
        rotate(std::max(-30, std::min(30, delta)));
        bool sample = HAL_GPIO_ReadPin(WU_KEY3_CODE_GPIO_Port, WU_KEY3_CODE_Pin) == GPIO_PIN_RESET;
        if (raw != sample)
        {
            raw = sample;
            changed = now;
        }
        if (now - changed >= 25 && down != sample)
        {
            down = sample;
            if (down)
            {
                pressed = now;
                longSent = false;
            }
            else if (!longSent)
                click();
        }
        if (down && !longSent && now - pressed >= 800)
        {
            back();
            longSent = true;
        }
        if (board_debug_ok)
        {
            board_debug_ok = 0;
            click();
        }
        updateData(now);
        // 菜单和编辑弹窗先合成，再一次性提交，避免两次传输造成闪烁。
        launcher.update(false);
        renderEdit();
        HAL::canvasUpdate();
        astra::Menu *pages[] = {home,       sensor,     relay,    uart,    flash,    keys,
                                systemPage, memoryPage, settings, canPage, gpioPage, confirm};
        for (unsigned i = 0; i < 12; ++i)
            if (launcher.getCurrentMenu() == pages[i])
                astra_page = i + 1;
        astra_selection = launcher.getCurrentMenu()->selectIndex;
        ++board_ui_heartbeat;
        board_ui_stack_free_words = uxTaskGetStackHighWaterMark(nullptr);
        astra_ui_frame_us = (DWT->CYCCNT - frameStart) / (SystemCoreClock / 1000000U);
        if (astra_ui_frame_us > astra_ui_max_us)
            astra_ui_max_us = astra_ui_frame_us;
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(16));
    }
}
