/** @file board_telemetry.h
 * @brief 独立通信任务，每秒通过 UART1 输出一个完整状态快照。
 * JSON 行协议：s 为快照序号，t 为记录类型，v 的字段顺序参见协议文档。
 */
#ifndef BOARD_TELEMETRY_H
#define BOARD_TELEMETRY_H
void Board_CommTask(void const *argument);
#endif
