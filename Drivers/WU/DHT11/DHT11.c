#include "DHT11.h"
#include "FreeRTOS.h"
#include "task.h"

void delay_us(uint32_t us)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000U);
    while ((uint32_t)(DWT->CYCCNT - start) < cycles) { }
}

void DHT11_IO_IN(void)//温湿度模块输入函数
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pin = IO_DHT11;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIO_DHT11, &GPIO_InitStruct);
}

void DHT11_IO_OUT(void)//温湿度模块输出函数
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Pin = IO_DHT11;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIO_DHT11, &GPIO_InitStruct);
};

//复位DHT11
void DHT11_Rst(void)
{
    DHT11_IO_OUT(); //SET OUTPUT
    DHT11_DQ_Low; //DQ=0
    HAL_Delay(20);    //拉低至少18ms
    DHT11_DQ_High; //DQ=1
    delay_us(30);     //主机拉高20~40us
}

//等待DHT11的回应
//返回1:未检测到DHT11的存在
//返回0:存在
u8 DHT11_Check(void)
{
    u8 retry = 0; //定义临时变量
    DHT11_IO_IN();//SET INPUT
    while ((HAL_GPIO_ReadPin(GPIO_DHT11, IO_DHT11) == 1) && retry < 100) //DHT11会拉低40~80us
    {
        retry++;
        delay_us(1);
    };
    if(retry >= 100)return 1;
    else retry = 0;
    while ((HAL_GPIO_ReadPin(GPIO_DHT11, IO_DHT11) == 0) && retry < 100) //DHT11拉低后会再次拉高40~80us
    {
        retry++;
        delay_us(1);
    };
    if(retry >= 100)return 1;
    return 0;
}
//从DHT11读取一个位
//返回值：1/0
u8 DHT11_Read_Bit(void)
{
    u8 retry = 0;
    while((HAL_GPIO_ReadPin(GPIO_DHT11, IO_DHT11) == 1) && retry < 100) //等待变为低电平
    {
        retry++;
        delay_us(1);
    }
    retry = 0;
    while((HAL_GPIO_ReadPin(GPIO_DHT11, IO_DHT11) == 0) && retry < 100) //等待变高电平
    {
        retry++;
        delay_us(1);
    }
    delay_us(40);//等待40us
    if(HAL_GPIO_ReadPin(GPIO_DHT11, IO_DHT11) == 1)
        return 1;
    else
        return 0;
}
//从DHT11读取一个字节
//返回值：读到的数据
u8 DHT11_Read_Byte(void)
{
    u8 i, dat;
    dat = 0;
    for (i = 0; i < 8; i++)
    {
        dat <<= 1;
        dat |= DHT11_Read_Bit();
    }
    return dat;
}

//从DHT11读取一次数据
//temp:温度值(范围:0~50°)
//humi:湿度值(范围:20%~90%)
//返回值：0,正常;1,读取失败
u8 DHT11_Read_Data(u8 *temp, u8 *humi)
{
    u8 buf[5], result=1;
    DHT11_Rst();
    /* Capture the 40-bit pulse train without RTOS/IRQ timing jitter. */
    taskENTER_CRITICAL();
    if (DHT11_Check() == 0) {
        for (u8 n=0;n<5;n++) buf[n]=DHT11_Read_Byte();
        if ((uint8_t)(buf[0]+buf[1]+buf[2]+buf[3]) == buf[4] && buf[0]<=100 && buf[2]<=80) {
            *humi=buf[0]; *temp=buf[2]; result=0;
        }
    }
    taskEXIT_CRITICAL();
    return result;
}
//初始化DHT11的IO口 DQ 同时检测DHT11的存在
//返回1:不存在
//返回0:存在
void DHT11_Init(void)
{
    DHT11_Rst();  //复位DHT11
    DHT11_Check();//等待DHT11的回应
}
