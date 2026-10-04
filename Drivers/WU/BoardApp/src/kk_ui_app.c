/** @file kk_ui_app.c
 * @brief UI 应用层：页面路由、绑定数据、事件提交和自定义页面绘制。
 * 所有 KK_UI 调用只由 OLED/UI 任务执行，其他任务不得直接操作页面。
 * 信息值每 250ms 刷新；固定数组存储文本，避免频繁动态分配。
 */
#include "kk_ui_app.h"
#include "board.h"
#include "board_memory.h"
#include "kk_oled.h"
#include "kk_ui_draw.h"
#include "kk_home_icons.h"
#include "../../wu_can/wu_can.h"
#include "can.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
extern const uint8_t board_font[], board_small_font[];
extern unsigned char cs_bj_flag_bit, buzzer_flag_dat1;
extern volatile uint32_t kk_ui_heartbeat, kk_ui_stack_free_words;
enum
{
    HOME = 1,
    SENSOR,
    RELAY,
    SERIAL,
    PAGE_FLASH,
    KEYS,
    SYSTEM,
    SETTINGS,
    UART_VIEW,
    FLASH_VIEW,
    IO,
    CAN,
    UART_STATS,
    FLASH_INFO,
    MEMORY
};
enum
{
    EV_RELAY = 100,
    EV_BRIGHT = 110,
    EV_LIMIT,
    EV_ALARM,
    EV_BUZZ,
    EV_HEX,
    EV_SEND,
    EV_FLASH,
    EV_ECHO,
    EV_CLEAR
};
static int32_t brightness = 80, limit = 45, port = 1, address;
static bool relays[5], alarm_enable = false, buzzer, hex_view, echo_setting;
static bool frozen, view_tx;
static unsigned offset;
static uint32_t updated;
static char values[40][40], keytext[9][24], memory_text[26][32];
static unsigned char *relay_ptr[] = {&relay1_flag_dat, &relay2_flag_dat, &relay3_flag_dat,
                                     &relay4_flag_dat, &relay5_flag_dat};
static const KK_UI_HomeItem home_items[] = {
    {"温湿度", home_icons[1], SENSOR},   {"继电器", home_icons[0], RELAY},
    {"串口监测", home_icons[2], SERIAL}, {"SPI Flash", home_icons[3], PAGE_FLASH},
    {"按键事件", home_icons[0], KEYS},   {"设备状态", home_icons[3], SYSTEM},
    {"设置", home_icons[0], SETTINGS},   {"内存状态", home_icons[1], MEMORY}};
static const KK_UI_HomePage homes[] = {{home_items, 8}};
static const KK_UI_MenuItem relay_items[] = {
    {"继电器 1", KK_UI_MENU_BOOL, 0}, {"继电器 2", KK_UI_MENU_BOOL, 1},
    {"继电器 3", KK_UI_MENU_BOOL, 2}, {"继电器 4", KK_UI_MENU_BOOL, 3},
    {"继电器 5", KK_UI_MENU_BOOL, 4}, {"输出状态", KK_UI_MENU_PAGE, IO}};
static const KK_UI_MenuItem serial_items[] = {
    {"选择串口", KK_UI_MENU_INT, 2},           {"收发数据", KK_UI_MENU_PAGE, UART_VIEW},
    {"HEX 显示", KK_UI_MENU_BOOL, 7},          {"发送测试", KK_UI_MENU_ACTION, EV_SEND},
    {"通信统计", KK_UI_MENU_PAGE, UART_STATS}, {"接收回显", KK_UI_MENU_BOOL, 8},
    {"清除历史", KK_UI_MENU_ACTION, EV_CLEAR}};
static const KK_UI_MenuItem flash_items[] = {{"浏览地址", KK_UI_MENU_INT, 3},
                                             {"Flash 数据", KK_UI_MENU_PAGE, FLASH_VIEW},
                                             {"重新检测", KK_UI_MENU_ACTION, EV_FLASH},
                                             {"芯片信息", KK_UI_MENU_PAGE, FLASH_INFO}};
static const KK_UI_MenuItem settings_items[] = {{"屏幕亮度", KK_UI_MENU_INT, 0},
                                                {"温度上限", KK_UI_MENU_INT, 1},
                                                {"报警使能", KK_UI_MENU_BOOL, 5},
                                                {"蜂鸣器", KK_UI_MENU_BOOL, 6}};
static const KK_UI_MenuItem system_items[] = {{"运行状态", KK_UI_MENU_PAGE, IO},
                                              {"CAN 总线", KK_UI_MENU_PAGE, CAN},
                                              {"按键事件", KK_UI_MENU_PAGE, KEYS},
                                              {"设置", KK_UI_MENU_PAGE, SETTINGS},
                                              {"内存状态", KK_UI_MENU_PAGE, MEMORY}};
static const KK_UI_MenuPage menus[] = {{"继电器控制", relay_items, NULL, 6},
                                       {"串口监测", serial_items, NULL, 7},
                                       {"SPI Flash", flash_items, NULL, 4},
                                       {"设置", settings_items, NULL, 4},
                                       {"设备状态", system_items, NULL, 5}};
static const KK_UI_InfoRow sensor_rows[] = {{"温度", values[0]},      {"湿度", values[1]},
                                            {"传感器", values[2]},    {"数据更新时间", values[3]},
                                            {"温度上限", values[4]},  {"报警", values[5]},
                                            {"电压(预设)", values[6]}};
static const KK_UI_InfoRow key_rows[] = {
    {"KEY0 短按", keytext[0]}, {"KEY0 长按", keytext[1]}, {"KEY0 双击", keytext[2]},
    {"KEY1 短按", keytext[3]}, {"KEY1 长按", keytext[4]}, {"KEY1 双击", keytext[5]},
    {"KEY2 短按", keytext[6]}, {"KEY2 长按", keytext[7]}, {"KEY2 双击", keytext[8]}};
static const KK_UI_InfoRow io_rows[] = {
    {"固件版本", "V1.1"},    {"运行时间", values[7]},  {"继电器", values[8]},
    {"蜂鸣器", values[9]},   {"空闲堆", values[10]},   {"UI 栈余量", values[11]},
    {"UI 心跳", values[12]}, {"UI 操作", "旋转/按下"}, {"设置保存", "仅本次运行"}};
static const KK_UI_InfoRow can_rows[] = {{"接收帧数", values[13]}, {"接收 ID", values[14]},
                                         {"数据长度", values[15]}, {"数据 0-3", values[16]},
                                         {"数据 4-7", values[17]}, {"错误码", values[18]}};
static const KK_UI_InfoRow uart_rows[] = {
    {"当前串口", values[19]},     {"接收字节", values[20]},    {"完成发送字节", values[21]},
    {"队列丢包", values[22]},     {"串口错误", values[23]},    {"波特率", "115200 8N1"},
    {"历史长度", "RX/TX 各256B"}, {"待发送包", values[28]},    {"接收丢包", values[29]},
    {"发送失败", values[30]},     {"RX 字节每秒", values[31]}, {"TX 字节每秒", values[32]},
    {"回显", values[33]}};
static const KK_UI_InfoRow flash_rows[] = {{"JEDEC ID", values[24]},     {"容量", values[25]},
                                           {"通信状态", values[26]},     {"状态寄存器", values[27]},
                                           {"SPI 接口", "SPI2 PB12-15"}, {"操作模式", "只读浏览"}};
/* 链接占用与实时内存分开标记；百分比采用整数计算，Release 无需浮点 printf。 */
static const KK_UI_InfoRow memory_rows[] = {
    {"闪存已用", memory_text[0]},       {"闪存容量", memory_text[1]},
    {"闪存占比", memory_text[2]},       {"RAM 静态", memory_text[3]},
    {"RAM 容量", memory_text[4]},       {"RAM 占比", memory_text[5]},
    {"CCM 已用", memory_text[6]},       {"CCM 容量", memory_text[7]},
    {"CCM 占比", memory_text[8]},       {"text", memory_text[9]},
    {"data", memory_text[10]},          {"bss", memory_text[11]},
    {"dec", memory_text[12]},           {"hex", memory_text[13]},
    {"RTOS 堆总量", memory_text[14]},   {"堆当前空闲", memory_text[15]},
    {"堆历史最小", memory_text[16]},    {"堆当前已用", memory_text[17]},
    {"UI 栈最小空闲", memory_text[18]}, {"任务数", memory_text[19]},
    {"默认任务栈", memory_text[20]},    {"LED 任务栈", memory_text[21]},
    {"按键任务栈", memory_text[22]},    {"业务任务栈", memory_text[23]},
    {"构建配置", memory_text[24]},      {"刷新周期", "250 ms"}};
static const KK_UI_InfoPage infos[] = {{"温湿度", sensor_rows, 7},   {"按键事件", key_rows, 9},
                                       {"运行状态", io_rows, 9},     {"CAN 总线", can_rows, 6},
                                       {"通信统计", uart_rows, 13},  {"芯片信息", flash_rows, 6},
                                       {"内存状态", memory_rows, 26}};
static const KK_UI_IntBinding ints[] = {{"屏幕亮度", &brightness, 10, 100, 5, "%", EV_BRIGHT},
                                        {"温度上限", &limit, 0, 80, 1, "C", EV_LIMIT},
                                        {"选择串口", &port, 1, 6, 1, "", 0},
                                        {"Flash 地址", &address, 0, 0xFFFFF0, 16, "", EV_FLASH}};
static const KK_UI_BoolBinding bools[] = {
    {"继电器 1", &relays[0], EV_RELAY},     {"继电器 2", &relays[1], EV_RELAY + 1},
    {"继电器 3", &relays[2], EV_RELAY + 2}, {"继电器 4", &relays[3], EV_RELAY + 3},
    {"继电器 5", &relays[4], EV_RELAY + 4}, {"报警使能", &alarm_enable, EV_ALARM},
    {"蜂鸣器", &buzzer, EV_BUZZ},           {"HEX 显示", &hex_view, EV_HEX},
    {"接收回显", &echo_setting, EV_ECHO}};
static const KK_UI_PageRoute routes[] = {
    {KK_UI_PAGE_HOME, 0},   {KK_UI_PAGE_INFO, 0},   {KK_UI_PAGE_MENU, 0}, {KK_UI_PAGE_MENU, 1},
    {KK_UI_PAGE_MENU, 2},   {KK_UI_PAGE_INFO, 1},   {KK_UI_PAGE_MENU, 4}, {KK_UI_PAGE_MENU, 3},
    {KK_UI_PAGE_CUSTOM, 0}, {KK_UI_PAGE_CUSTOM, 1}, {KK_UI_PAGE_INFO, 2}, {KK_UI_PAGE_INFO, 3},
    {KK_UI_PAGE_INFO, 4},   {KK_UI_PAGE_INFO, 5},   {KK_UI_PAGE_INFO, 6}};
static const KK_UI_App app = {.root_page = HOME,
                              .routes = routes,
                              .route_count = 15,
                              .home_pages = homes,
                              .home_page_count = 1,
                              .menu_pages = menus,
                              .menu_page_count = 5,
                              .info_pages = infos,
                              .info_page_count = 7,
                              .custom_page_count = 2,
                              .int_bindings = ints,
                              .int_binding_count = 4,
                              .bool_bindings = bools,
                              .bool_binding_count = 9,
                              .fonts = {board_font, board_font, board_font},
                              .texts = {"返回", "取消", "确定", "开", "关", "提示"}};
const KK_UI_App *KK_UI_AppGet(void)
{
    return &app;
}
void KK_UI_AppReset(uint32_t now)
{
    updated = now - 500;
    Board_FlashRead(0);
}
/* 仅在确认框提交后才修改硬件状态，编辑草稿不直接驱动继电器。 */
void KK_UI_AppProcessEvents(void)
{
    KK_UI_EventId event;
    while (KK_UI_PollEvent(&event))
    {
        if (event >= EV_RELAY && event < EV_RELAY + 5)
            *relay_ptr[event - EV_RELAY] = relays[event - EV_RELAY];
        if (event == EV_LIMIT)
            Temperature_max = limit;
        if (event == EV_ALARM)
            cs_bj_flag_bit = alarm_enable ? 0 : 1;
        if (event == EV_BUZZ)
            buzzer_flag_dat1 = buzzer;
        if (event == EV_FLASH)
            Board_FlashRead(address);
        if (event == EV_ECHO)
            board_uart_echo[port - 1] = echo_setting;
        if (event == EV_CLEAR)
        {
            Board_UartClearHistory(port - 1);
            offset = 0;
            frozen = false;
            KK_UI_ShowToast("历史已清除", 1000);
        }
        if (event == EV_SEND)
        {
            const char *s = "V1.1 UART test\r\n";
            KK_UI_ShowToast(Board_Send(port - 1, (const uint8_t *)s, strlen(s)) ? "已加入发送队列"
                                                                                : "发送队列已满",
                            1000);
        }
        if (event == EV_BRIGHT && KK_UI_IsDisplayIdle())
            OLED_SetContrast(brightness * 255 / 100);
    }
    static int applied = -1;
    if (applied != brightness && KK_UI_IsDisplayIdle() &&
        OLED_SetContrast(brightness * 255 / 100) == OLED_OK)
        applied = brightness;
}
/* 各任务句柄由 FreeRTOS 创建，栈查询返回的是历史最小剩余量，并非瞬时 SP。 */
extern void *defaultTaskHandle, *WU_Task02Handle, *WU_Task03Handle, *WU_Task05Handle;
static void memory_update(void)
{
    BoardMemory m;
    Board_MemorySnapshot(&m);
    uint32_t used[] = {m.flash_used, m.ram_used, m.ccm_used};
    uint32_t total[] = {m.flash_total, m.ram_total, m.ccm_total};
    for (unsigned i = 0; i < 3; i++)
    {
        unsigned base = i * 3;
        uint32_t percentage =
            total[i] ? (uint32_t)(((uint64_t)used[i] * 10000 + total[i] / 2) / total[i]) : 0;
        snprintf(memory_text[base], 32, "%lu B", (unsigned long)used[i]);
        snprintf(memory_text[base + 1], 32, "%lu KB", (unsigned long)(total[i] / 1024));
        snprintf(memory_text[base + 2], 32, "%lu.%02lu %%", (unsigned long)(percentage / 100),
                 (unsigned long)(percentage % 100));
    }
    uint32_t bytes[] = {m.text, m.data, m.bss, m.dec};
    for (unsigned i = 0; i < 4; i++)
        snprintf(memory_text[9 + i], 32, "%lu", (unsigned long)bytes[i]);
    snprintf(memory_text[13], 32, "%lX", (unsigned long)m.dec);
    uint32_t runtime[] = {m.heap_total, m.heap_free, m.heap_min_free, m.heap_total - m.heap_free,
                          kk_ui_stack_free_words * sizeof(StackType_t)};
    for (unsigned i = 0; i < 5; i++)
        snprintf(memory_text[14 + i], 32, "%lu B", (unsigned long)runtime[i]);
    snprintf(memory_text[19], 32, "%lu", (unsigned long)m.task_count);
    void *tasks[] = {defaultTaskHandle, WU_Task02Handle, WU_Task03Handle, WU_Task05Handle};
    for (unsigned i = 0; i < 4; i++)
    {
        if (tasks[i])
            snprintf(memory_text[20 + i], 32, "%lu B",
                     (unsigned long)(uxTaskGetStackHighWaterMark(tasks[i]) * sizeof(StackType_t)));
        else
            strcpy(memory_text[20 + i], "未创建");
    }
#ifdef DEBUG
    strcpy(memory_text[24], "Debug");
#else
    strcpy(memory_text[24], "Release");
#endif
}
void KK_UI_AppUpdate(uint32_t now)
{
    echo_setting = board_uart_echo[port - 1];
    for (unsigned i = 0; i < 5; i++)
        relays[i] = *relay_ptr[i] != 0;
    if (now - updated < 250)
        return;
    updated = now;
    memory_update();
#define F(i, ...) snprintf(values[i], sizeof values[i], __VA_ARGS__)
    F(0, "%u C", Temperature);
    F(1, "%u %%", Humidity);
    F(2, "%s", board_dht_ok ? "正常" : "读取失败");
    F(3, "%lu ms", (unsigned long)(now - board_dht_last));
    F(4, "%u C", Temperature_max);
    F(5, "%s", T_alarm ? "开启" : "关闭");
    F(6, "%lu.%02lu V", (unsigned long)(Voltage * 100) / 100, (unsigned long)(Voltage * 100) % 100);
    F(7, "%lu s", (unsigned long)(now / 1000));
    F(8, "%u%u%u%u%u", *relay_ptr[0], *relay_ptr[1], *relay_ptr[2], *relay_ptr[3], *relay_ptr[4]);
    F(9, "%s", buzzer_flag_dat1 ? "开" : "关");
    F(10, "%u B", (unsigned)xPortGetFreeHeapSize());
    F(11, "%lu B", (unsigned long)kk_ui_stack_free_words * 4);
    F(12, "%lu", (unsigned long)kk_ui_heartbeat);
    F(13, "%lu", (unsigned long)board_can_rx);
    F(14, "%08lX", (unsigned long)RxID);
    F(15, "%u", RxLength);
    F(16, "%02X %02X %02X %02X", RxData[0], RxData[1], RxData[2], RxData[3]);
    F(17, "%02X %02X %02X %02X", RxData[4], RxData[5], RxData[6], RxData[7]);
    F(18, "%08lX", (unsigned long)HAL_CAN_GetError(&hcan1));
    F(19, "UART%ld", (long)port);
    F(20, "%lu", (unsigned long)board_rx[port - 1]);
    F(21, "%lu", (unsigned long)board_tx[port - 1]);
    F(22, "%lu", (unsigned long)board_drop[port - 1]);
    F(23, "%lu", (unsigned long)board_uart_errors[port - 1]);
    F(24, "%06lX", (unsigned long)board_flash_id);
    F(25, "%lu KB", (unsigned long)(board_flash_bytes / 1024));
    F(26, "%s", board_flash_ok ? "正常" : "检测失败");
    F(27, "%02X", board_flash_status);
    F(28, "%u", Board_UartPending(port - 1));
    F(29, "%lu", (unsigned long)board_rx_drop[port - 1]);
    F(30, "%lu", (unsigned long)board_tx_failed[port - 1]);
    static uint32_t rate_time, previous_rx[6], previous_tx[6], rx_rate[6], tx_rate[6];
    if (now - rate_time >= 1000)
    {
        uint32_t elapsed = now - rate_time;
        for (unsigned n = 0; n < 6; n++)
        {
            rx_rate[n] = (uint32_t)((uint64_t)(board_rx[n] - previous_rx[n]) * 1000 / elapsed);
            tx_rate[n] = (uint32_t)((uint64_t)(board_tx[n] - previous_tx[n]) * 1000 / elapsed);
            previous_rx[n] = board_rx[n];
            previous_tx[n] = board_tx[n];
        }
        rate_time = now;
    }
    F(31, "%lu B/s", (unsigned long)rx_rate[port - 1]);
    F(32, "%lu B/s", (unsigned long)tx_rate[port - 1]);
    F(33, "%s", board_uart_echo[port - 1] ? "开" : "关");
    for (unsigned k = 0; k < 3; k++)
        for (unsigned e = 0; e < 3; e++)
            snprintf(keytext[k * 3 + e], 24, "%lu", (unsigned long)board_key_events[k][e]);
    KK_UI_Invalidate();
}
/* 自定义串口页底部为动作选择器；旋转选择，按下执行，不占用普通按键。 */
static unsigned selected_action;
static char frozen_lines[2][32];
void KK_UI_CustomOnEnter(KK_UI_PageId page)
{
    selected_action = 0;
    offset = 0;
    frozen = false;
    view_tx = false;
    if (page == FLASH_VIEW)
        Board_FlashRead(address);
}
void KK_UI_CustomOnLeave(KK_UI_PageId page)
{
    (void)page;
}
void KK_UI_CustomOnInput(KK_UI_PageId page, KK_UI_InputEvent event)
{
    if (event.action != KK_UI_INPUT_OK)
    {
        int d = event.action == KK_UI_INPUT_DOWN ? event.steps : -(int)event.steps;
        selected_action = (selected_action + d + 5 * 100) % 5;
        KK_UI_Invalidate();
        return;
    }
    if (selected_action == 4)
    {
        KK_UI_CustomRequestClose();
        return;
    }
    if (page == UART_VIEW)
    {
        if (selected_action == 0)
        {
            port = port % 6 + 1;
            offset = 0;
            frozen = false;
        }
        if (selected_action == 1)
        {
            view_tx = !view_tx;
            offset = 0;
            frozen = false;
        }
        if (selected_action == 2)
        {
            frozen = !frozen;
            if (frozen)
                for (unsigned i = 0; i < 2; i++)
                    Board_History(port - 1, view_tx, offset + i * (hex_view ? 5 : 16), hex_view,
                                  frozen_lines[i], 32);
        }
        if (selected_action == 3)
        {
            offset = (offset + (hex_view ? 10 : 32)) % 256;
            frozen = false;
        }
    }
    else
    {
        if (selected_action == 0)
            Board_FlashRead(address);
        if (selected_action == 1 && address >= 16)
            address -= 16;
        if (selected_action == 2 && board_flash_bytes && address + 32 <= board_flash_bytes &&
            address + 16 <= 0xFFFFF0)
            address += 16;
        if (selected_action == 3)
            address = 0;
        Board_FlashRead(address);
    }
    KK_UI_Invalidate();
}
bool KK_UI_CustomOnTick(KK_UI_PageId page, uint32_t now)
{
    (void)page;
    /* 不再每 5ms 请求重绘；动态数据每 250ms 更新，降低软件 I2C 与 CPU 开销。 */
    static uint32_t last;
    if (frozen || now - last < 250)
        return false;
    last = now;
    return true;
}
static void draw(int16_t x, int y, const char *s)
{
    OLED_DrawUTF8(x, y, s);
}
void KK_UI_CustomOnDraw(KK_UI_PageId page, int16_t x, int16_t clip, uint16_t width)
{
    char b[64];
    OLED_SetClipWindow(clip, 0, width, 64);
    OLED_SetFont(board_font);
    OLED_DrawHLine(x, 15, 128);
    if (page == UART_VIEW)
    {
        unsigned p = port - 1;
        snprintf(b, sizeof b, "UART%ld %s %s", (long)port, view_tx ? "TX" : "RX",
                 hex_view ? "HEX" : "TXT");
        draw(x + 2, 0, b);
        snprintf(b, sizeof b, "R%lu T%lu", (unsigned long)board_rx[p], (unsigned long)board_tx[p]);
        draw(x + 2, 17, b);
        for (unsigned i = 0; i < 2; i++)
        {
            if (frozen)
                strcpy(b, frozen_lines[i]);
            else
                Board_History(p, view_tx, offset + i * (hex_view ? 5 : 16), hex_view, b, 32);
            draw(x + 2, 30 + i * 12, b);
        }
        const char *names[] = {"切换串口", "切换 RX/TX", "暂停/继续", "浏览更早数据", "返回"};
        draw(x + 2, 51, names[selected_action]);
    }
    else
    {
        snprintf(b, sizeof b, "Flash %06lX", (unsigned long)board_flash_id);
        draw(x + 2, 0, b);
        if (!board_flash_ok)
        {
            draw(x + 2, 18, "未识别/通信失败");
        }
        else
        {
            OLED_SetFont(board_small_font);
            for (unsigned row = 0; row < 4; row++)
            {
                snprintf(b, sizeof b, "%06lX  %02X %02X %02X %02X",
                         (unsigned long)(address + row * 4), board_flash_data[row * 4],
                         board_flash_data[row * 4 + 1], board_flash_data[row * 4 + 2],
                         board_flash_data[row * 4 + 3]);
                draw(x + 2, 18 + row * 8, b);
            }
            OLED_SetFont(board_font);
        }
        const char *names[] = {"重新读取", "上一页", "下一页", "回到地址零", "返回"};
        draw(x + 2, 51, names[selected_action]);
    }
    OLED_DrawRFrame(x, 50, 128, 14, 3);
    OLED_ResetClipWindow();
}
