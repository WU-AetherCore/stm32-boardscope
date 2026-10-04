/** @file board_memory.h
 * @brief 编译期占用与 FreeRTOS 实时内存监测，单位均为字节。
 * 静态 RAM 包含 FreeRTOS 堆数组；不能再把运行时堆已用量加到静态 RAM 上。
 */
#ifndef BOARD_MEMORY_H
#define BOARD_MEMORY_H
#include <stdint.h>
typedef struct
{
    uint32_t flash_used, flash_total, ram_used, ram_total, ccm_used, ccm_total;
    uint32_t text, data, bss, dec;
    uint32_t heap_total, heap_free, heap_min_free, task_count;
} BoardMemory;
void Board_MemorySnapshot(BoardMemory *snapshot);
#endif
