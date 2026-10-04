/*
 * W25Q.h
 *
 *  Created on: Dec 27, 2024
 *      Author: 17076
 */

#ifndef WU_W25Q_W25Q_H_
#define WU_W25Q_W25Q_H_

#include "main.h"
#include "spi.h"


/* W25Q Flash 命令定义 */
#define W25Q_WRITE_ENABLE        0x06  // 写使能命令，允许写入或擦除操作
#define W25Q_WRITE_DISABLE       0x04  // 写禁用命令，禁止写入或擦除操作
#define W25Q_READ_STATUS_REG_1   0x05  // 读取状态寄存器 1 命令，获取 Flash 的当前状态
#define W25Q_READ_DATA           0x03  // 读取数据命令，从指定地址读取数据
#define W25Q_PAGE_PROGRAM        0x02  // 页编程命令，向指定地址写入数据（页大小为 256 字节）
#define W25Q_SECTOR_ERASE        0x20  // 扇区擦除命令，擦除指定扇区（扇区大小为 4KB）
#define W25Q_CHIP_ERASE          0xC7  // 全片擦除命令，擦除整个 Flash
#define W25Q_READ_JEDEC_ID       0x9F  // 读取 JEDEC ID 命令，获取 Flash 的制造商 ID、存储器类型和容量信息


void W25Q_CS_LOW(void);//拉低 W25Q Flash 的片选信号 (CS)
void W25Q_CS_HIGH(void);//拉高 W25Q Flash 的片选信号 (CS)
uint8_t W25Q_ReadStatusReg1(void);//读取 W25Q Flash 的状态寄存器 1
void W25Q_WriteEnable(void);//使能 W25Q Flash 的写操作
void W25Q_WriteDisable(void);//禁用 W25Q Flash 的写操作
void W25Q_ReadData(uint32_t address, uint8_t *data, uint16_t length);//从 W25Q Flash 读取数据
void W25Q_PageProgram(uint32_t address, uint8_t *data, uint16_t length);//向 W25Q Flash 写入数据
void W25Q_SectorErase(uint32_t address);//擦除 W25Q Flash 的一个扇区
void W25Q_ChipErase(void);//擦除整个 W25Q Flash
uint32_t W25Q_ReadJEDECID(void);//读取 W25Q Flash 的 JEDEC ID



#endif /* WU_W25Q_W25Q_H_ */
