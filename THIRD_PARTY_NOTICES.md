# 第三方来源与许可证

根目录 MIT 仅覆盖 WU-AetherCore 自有业务代码、监控器和说明，不重新许可第三方组件。

| 组件 | 来源 | 许可位置 |
|---|---|---|
| KK_UI / 参考应用与图标 | https://gitee.com/keysking/kk_ui | Middlewares/Third_Party/KK_UI/LICENSE.txt；Drivers/WU/BoardApp/LICENSE.txt |
| KK_OLED | KK_UI 项目附带的 KK_OLED | Middlewares/Third_Party/KK_OLED/LICENSE.txt |
| STM32 HAL / ST 生成文件 | STMicroelectronics | Drivers/STM32F4xx_HAL_Driver/LICENSE.txt（BSD-3-Clause） |
| CMSIS | ARM | Drivers/CMSIS/LICENSE.txt（Apache-2.0） |
| FreeRTOS | FreeRTOS / Amazon | Middlewares/Third_Party/FreeRTOS/Source/LICENSE（MIT） |
| U8g2 字库容器 | https://github.com/olikraus/u8g2 | LICENSES/U8g2-LICENSE.txt |
| 文泉驿点阵宋体 | WenQuanYi Project Board of Trustees and Qianqian Fang, Copyright 2004–2010 | GPL v2，带字体嵌入例外；LICENSES/GPL-2.0.txt |
| u8g2_wqy 转换工程 | https://github.com/larryli/u8g2_wqy | LICENSES/u8g2-wqy-LICENSE.txt |

board_font.c 是原 U8g2 的 u8g2_font_wqy13_t_gb2312a 数组（119848 B）改名，字模内容未改。
board_small_font.c 来自 u8g2_font_5x7_tr，原声明为 Public domain font. Share and enjoy.
文泉驿来源与许可声明参见 [U8g2 官方说明](https://github.com/olikraus/u8g2/wiki/fntgrpwqy)。字体不会因为使用 MIT 的 UI 而转为 MIT。
开源副本移除未使用的在线生成 Demo 字库和旧 OLED 源码，原本地工程保留。
