/** @file board_memory.c
 * @brief 符号取值与运行时快照。Flash/静态 RAM 在同一固件运行期间不改变。
 * 堆空闲量与历史最小空闲量由 FreeRTOS heap_4 提供，不扫描未分配物理内存。
 */
#include "board_memory.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stddef.h>
extern char __board_flash_used, __board_ram_used, __board_ccm_used;
extern char __board_flash_total, __board_ram_total, __board_ccm_total;
extern char __board_text, __board_data, __board_bss;
#define SYMBOL_VALUE(name) ((uint32_t)(uintptr_t)&name)
void Board_MemorySnapshot(BoardMemory *s)
{
    if (s == NULL)
        return;
    s->flash_used = SYMBOL_VALUE(__board_flash_used);
    s->flash_total = SYMBOL_VALUE(__board_flash_total);
    s->ram_used = SYMBOL_VALUE(__board_ram_used);
    s->ram_total = SYMBOL_VALUE(__board_ram_total);
    s->ccm_used = SYMBOL_VALUE(__board_ccm_used);
    s->ccm_total = SYMBOL_VALUE(__board_ccm_total);
    s->text = SYMBOL_VALUE(__board_text);
    s->data = SYMBOL_VALUE(__board_data);
    s->bss = SYMBOL_VALUE(__board_bss);
    s->dec = s->text + s->data + s->bss;
    s->heap_total = configTOTAL_HEAP_SIZE;
    s->heap_free = xPortGetFreeHeapSize();
    s->heap_min_free = xPortGetMinimumEverFreeHeapSize();
    s->task_count = uxTaskGetNumberOfTasks();
}
