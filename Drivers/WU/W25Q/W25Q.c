/*
 * W25Q.c
 *
 *  Created on: Dec 27, 2024
 *      Author: 17076
 */
#include "W25Q.h"

/**
 * @brief 拉低 W25Q Flash 的片选信号 (CS)
 * @note 在 SPI 通信开始时调用此函数，以选中 W25Q Flash。
 */
void W25Q_CS_LOW(void) {
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
}

/**
 * @brief 拉高 W25Q Flash 的片选信号 (CS)
 * @note 在 SPI 通信结束时调用此函数，以取消选中 W25Q Flash。
 */
void W25Q_CS_HIGH(void) {
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
}

/**
 * @brief 读取 W25Q Flash 的状态寄存器 1
 * @return 状态寄存器的值
 * @note 状态寄存器 1 包含 Flash 的当前状态信息，例如 BUSY 位和写保护状态。
 */
uint8_t W25Q_ReadStatusReg1(void) {
	uint8_t cmd = W25Q_READ_STATUS_REG_1; // 读取状态寄存器 1 的命令
	uint8_t status; // 用于存储读取的状态寄存器值
	W25Q_CS_LOW(); // 拉低 CS，选中 Flash
	HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY); // 发送读取状态寄存器命令
	HAL_SPI_Receive(&hspi2, &status, 1, HAL_MAX_DELAY); // 接收状态寄存器值
	W25Q_CS_HIGH(); // 拉高 CS，取消选中 Flash
	return status; // 返回状态寄存器值
}

/**
 * @brief 使能 W25Q Flash 的写操作
 * @note 在写入数据或擦除操作之前，必须调用此函数使能写操作。
 */
void W25Q_WriteEnable(void) {
	uint8_t cmd = W25Q_WRITE_ENABLE; // 写使能命令
	W25Q_CS_LOW(); // 拉低 CS，选中 Flash
	HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY); // 发送写使能命令
	W25Q_CS_HIGH(); // 拉高 CS，取消选中 Flash
}

/**
 * @brief 禁用 W25Q Flash 的写操作
 * @note 在写入数据或擦除操作完成后，可以调用此函数禁用写操作。
 */
void W25Q_WriteDisable(void) {
	uint8_t cmd = W25Q_WRITE_DISABLE; // 写禁用命令
	W25Q_CS_LOW(); // 拉低 CS，选中 Flash
	HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY); // 发送写禁用命令
	W25Q_CS_HIGH(); // 拉高 CS，取消选中 Flash
}

/**
 * @brief 从 W25Q Flash 读取数据
 * @param address 要读取数据的起始地址
 * @param data 存储读取数据的缓冲区
 * @param length 要读取的数据长度
 * @note 从指定地址开始读取指定长度的数据，并存储到提供的缓冲区中。
 */
void W25Q_ReadData(uint32_t address, uint8_t *data, uint16_t length) {
	uint8_t cmd[4];
	cmd[0] = W25Q_READ_DATA; // 读取数据命令
	cmd[1] = (address >> 16) & 0xFF; // 地址的高字节
	cmd[2] = (address >> 8) & 0xFF; // 地址的中字节
	cmd[3] = address & 0xFF; // 地址的低字节
	W25Q_CS_LOW(); // 拉低 CS，选中 Flash
	HAL_SPI_Transmit(&hspi2, cmd, 4, HAL_MAX_DELAY); // 发送读取数据命令和地址
	HAL_SPI_Receive(&hspi2, data, length, HAL_MAX_DELAY); // 接收数据
	W25Q_CS_HIGH(); // 拉高 CS，取消选中 Flash
}

/**
 * @brief 向 W25Q Flash 写入数据
 * @param address 要写入数据的起始地址
 * @param data 要写入的数据缓冲区
 * @param length 要写入的数据长度
 * @note 从指定地址开始写入指定长度的数据。写入操作必须在页边界内（256 字节）。
 */
void W25Q_PageProgram(uint32_t address, uint8_t *data, uint16_t length) {
	uint8_t cmd[4];
	cmd[0] = W25Q_PAGE_PROGRAM; // 页编程命令
	cmd[1] = (address >> 16) & 0xFF; // 地址的高字节
	cmd[2] = (address >> 8) & 0xFF; // 地址的中字节
	cmd[3] = address & 0xFF; // 地址的低字节
	W25Q_WriteEnable(); // 使能写操作
	W25Q_CS_LOW(); // 拉低 CS，选中 Flash
	HAL_SPI_Transmit(&hspi2, cmd, 4, HAL_MAX_DELAY); // 发送页编程命令和地址
	HAL_SPI_Transmit(&hspi2, data, length, HAL_MAX_DELAY); // 发送数据
	W25Q_CS_HIGH(); // 拉高 CS，取消选中 Flash
	while (W25Q_ReadStatusReg1() & 0x01); // 等待写入操作完成
}

/**
 * @brief 擦除 W25Q Flash 的一个扇区
 * @param address 要擦除的扇区起始地址
 * @note 扇区大小为 4KB。擦除操作会将整个扇区的数据置为 0xFF。
 */
void W25Q_SectorErase(uint32_t address) {
	uint8_t cmd[4];
	cmd[0] = W25Q_SECTOR_ERASE; // 扇区擦除命令
	cmd[1] = (address >> 16) & 0xFF; // 地址的高字节
	cmd[2] = (address >> 8) & 0xFF; // 地址的中字节
	cmd[3] = address & 0xFF; // 地址的低字节
	W25Q_WriteEnable(); // 使能写操作
	W25Q_CS_LOW(); // 拉低 CS，选中 Flash
	HAL_SPI_Transmit(&hspi2, cmd, 4, HAL_MAX_DELAY); // 发送扇区擦除命令和地址
	W25Q_CS_HIGH(); // 拉高 CS，取消选中 Flash
	while (W25Q_ReadStatusReg1() & 0x01); // 等待擦除操作完成
}

/**
 * @brief 擦除整个 W25Q Flash
 * @note 全片擦除操作会将整个 Flash 的数据置为 0xFF。此操作可能需要几秒钟。
 */
void W25Q_ChipErase(void) {
	uint8_t cmd = W25Q_CHIP_ERASE; // 全片擦除命令
	W25Q_WriteEnable(); // 使能写操作
	W25Q_CS_LOW(); // 拉低 CS，选中 Flash
	HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY); // 发送全片擦除命令
	W25Q_CS_HIGH(); // 拉高 CS，取消选中 Flash
	while (W25Q_ReadStatusReg1() & 0x01); // 等待擦除操作完成
}

/**
 * @brief 读取 W25Q Flash 的 JEDEC ID
 * @return JEDEC ID（3 字节）
 * @note JEDEC ID 包含制造商 ID、存储器类型和容量信息。
 */
uint32_t W25Q_ReadJEDECID(void) {
	uint8_t cmd = W25Q_READ_JEDEC_ID; // 读取 JEDEC ID 命令
	uint8_t id[3]; // 用于存储 JEDEC ID
	W25Q_CS_LOW(); // 拉低 CS，选中 Flash
	HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY); // 发送读取 JEDEC ID 命令
	HAL_SPI_Receive(&hspi2, id, 3, HAL_MAX_DELAY); // 接收 JEDEC ID
	W25Q_CS_HIGH(); // 拉高 CS，取消选中 Flash
	return (id[0] << 16) | (id[1] << 8) | id[2]; // 返回 JEDEC ID
}
