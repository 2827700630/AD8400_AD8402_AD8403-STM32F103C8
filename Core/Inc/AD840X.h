/*
 * AD840X 数字电位器控制程序（STM32 HAL库）
 * 数据手册参考：AD8400_8402_8403.pdf Rev.E
 * 硬件连接：SPI + GPIO控制CS/SHDN/RS
 */

#ifndef __AD840X_H
#define __AD840X_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "main.h"
#include "spi.h"

/* AD840X CS引脚定义 请在CubeMX中设置AD840X-CS引脚*/

/* AD840X通道定义 */
#define AD840X_CHANNEL_A 0b00000000 // 通道A
#define AD840X_CHANNEL_B 0b00000001 // 通道B
#define AD840X_CHANNEL_C 0b00000010 // 通道C，只有AD8403有通道C
#define AD840X_CHANNEL_D 0b00000011 // 通道D，只有AD8403有通道D

    /* 函数声明 */
    void AD840X_Init(void);
    void AD840X_Write(uint8_t channel, uint8_t value);

#ifdef __cplusplus
}
#endif
#endif /* __AD840X_H */