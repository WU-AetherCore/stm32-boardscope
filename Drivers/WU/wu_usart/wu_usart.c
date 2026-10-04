/** @file wu_usart.c
 * @brief 保留旧 Seria1/2/3_Printf 接口，内部统一使用复制式发送队列。
 * 栈缓冲只用于格式化；Board_Send 在返回前复制数据，DMA 不引用临时栈。
 * 单条输出最多 127 字节，超长内容截断；队列满在通信统计中计为丢包。
 */
#include "wu_usart.h"
#include "board.h"
#include <stdarg.h>
#include <stdio.h>
static void print(unsigned p, char *f, va_list args)
{
    char b[128];
    int n = vsnprintf(b, sizeof b, f, args);
    if (n > 0)
        Board_Send(p, (uint8_t *)b, n >= (int)sizeof b ? sizeof b - 1 : n);
}
void Seria1_Printf(char *f, ...)
{
    va_list a;
    va_start(a, f);
    print(0, f, a);
    va_end(a);
}
void Seria2_Printf(char *f, ...)
{
    va_list a;
    va_start(a, f);
    print(1, f, a);
    va_end(a);
}
void Seria3_Printf(char *f, ...)
{
    va_list a;
    va_start(a, f);
    print(2, f, a);
    va_end(a);
}
