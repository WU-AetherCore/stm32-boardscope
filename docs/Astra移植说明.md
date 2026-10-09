# V1.2 Astra 移植说明

移植日期：2026-10-08。来源：[AstraThreshold/oled-ui-astra](https://github.com/AstraThreshold/oled-ui-astra)，固定提交 `88716a62f84c41838458fd159841fe67e6a86f87`。

V1.2 从本地 STM32-BoardScope（V1.1.1）复制创建。V1.0、V1.1 和开源目录未修改。
屏蔽的是旧显示软件，继续使用原有 128×64 SSD1306 OLED 硬件。旧 KK_UI、KK_OLED、kk_ui_app.c、kk_ui_port.c 不参与编译；不启动上游 F103 演示程序。

## 分层

| 文件 / 目录 | 职责 |
|---|---|
| Middlewares/Third_Party/Astra | 固定版本 Astra 原生磁贴、列表、选择框、摄像机、动画及 U8g2 图形 |
| astra_hal.cpp | 实现 HAL 虚接口，将 Astra 绘图转换为 U8g2；C++ 容器分配接入 FreeRTOS 堆 |
| astra_display.c | PB6/PB7 硬件 I2C1，400 kHz，中断发送，0x78 写地址，变化区间发送及故障重试 |
| astra_app.cpp | 板级菜单、实时数据、编码器去抖和动作提交；显示任务唯一入口 |
| board.c / board_uart.c / board_memory.c | 保留板级数据与业务接口 |
| board_telemetry.c | 独立通信任务，每秒状态上报；ui 字段布局兼容电脑窗口 |

## 上游必要修改

1. 默认字体替换为已使用的文泉驿 13px GB2312a。F103 SPI 屏驱动替换为本板硬件 I2C1，不改 SPI2 Flash 接线。
2. getUIConfig/getSystemConfig 使用 inline，避免不同翻译单元拿到独立配置。
3. Item 使用共享配置引用；删除默认图标重复存储，数据行释放未使用图标。初版板上发现初始化堆不足，已据实际分配修复。
4. Animation 的 1024 B 缓冲长度使用 uint16_t，避免上游 uint8_t 溢出。
5. Launcher 停用双按键扫描，增加当前页查询与板级跳转入口。菜单、弹窗先合成再一次提交，避免编辑时闪烁。
6. 摄像机在类型检查为 List 后使用 static_cast，编译关闭 RTTI/异常；所有 UI 实例在 HAL 注入后创建。
7. 以实际帧间隔计算动画收敛速度，硬件 I2C1 后台发送，静止画面只发送变化区间；OLED 断线后每秒尝试重新初始化。

## 输入和业务

旋转编码器选择，短按进入或执行，长按 800 ms 返回或取消编辑；去抖 25 ms，每档两个 TIM3 计数。
信息行短按返回。菜单末尾也有返回项目。普通 PD5/PD6/PD7 三按键不参与 UI，保留短按、长按、双击回调。

继电器必须进入确认列表，默认选中取消。旋转到确定后按下才提交，提交明确目标值；确认期间其他业务改变标志不会使动作反转。
亮度、温度上限、串口选择、Flash 地址：短按进入编辑，旋转调整，短按保存，长按取消。设置暂不掉电保存。
串口可选择 UART1–6、RX/TX、ASCII/HEX、暂停、历史、发送测试、回显、清除；历史依旧仅保留 256 B。
Flash 以 16 B 为步长，只读，不擦除或写入。

## 内存与构建

C11 / C++17，Cortex-M4F 硬浮点。FreeRTOS 堆 65536 B，OLED 任务栈 1024 个字（4096 B），通信任务 512 个字（2048 B）。
C++ new/delete 接入 pvPortMalloc/vPortFree；分配失败会将 UI 状态置为 2，并让通信任务继续报告，而非静默调用空指针。
所有菜单启动时构建，动态标题预留 80 B；实时标题每 250 ms 更新。

```powershell
cmake --preset Debug
cmake --build --preset Debug --parallel
cmake --preset Release
cmake --build --preset Release --parallel
python tools/verify_memory.py
```

需要 ARM GNU 工具链含 g++、CMake 和 Ninja。普通 build 不烧录。

## 许可证

Astra 原仓库采用 GPL-3.0，本 V1.2 的组合工程按 GPL-3.0 处理，根 LICENSE 保留原全文。
原 BoardScope 自有代码 MIT 保留于 LICENSES/BoardScope-MIT.txt；各第三方仍保留原许可。
V1.1 保留在 Git 历史与 v1.1.1 分支；V1.2 组合工程在同一仓库中按 GPL-3.0 发布。
