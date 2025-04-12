/*
 * AD840X系列数字电位器驱动库
 * 雪豹  编写
 * 型号支持：AD8400（1通道）、AD8402（2通道）、AD8403（4通道）
 * 数据手册：AD8400_8402_8403.pdf Rev.E
 */

/*
 * ====================== 主要特性 ======================
 * 参考数据手册 Page1 Features / Page3 General Description
 * ------------------------------------------------------
 * 1. 通道配置：
 *     AD8400：只有通道1
 *     AD8402：有通道1和通道2
 *     AD8403：有通道1、2、3和4
 *
 * 2. 关键参数：
 *    - 256位分辨率（8位控制）
 *    - 可选标称电阻：1kΩ/10kΩ/50kΩ/100kΩ（型号由元件后缀指定）
 *    - 电阻温度系数：500ppm/°C（电阻模式），15ppm/°C（分压器模式）
 *    - 工作电压：2.7V-5.5V单电源
 *    - SPI接口：10MHz最大时钟速率（Page1 Features）
 *    - 低功耗模式：<5μA（SHDN控制，Page20）
 *
 * 3. 特殊功能：
 *    - 硬件复位（RS引脚强制中值，Page20）
 *    - 级联支持（仅AD8403，SDO引脚实现，Page22）
 */
/*
 * ===================== CubeMX 配置指南 =====================
 * 以下配置基于STM32 HAL库
 * 数据手册参考：Page10 Timing Diagrams, Page12-13 Pin Descriptions
 * [步骤1] SPI外设配置
 * ---------------------------------------------------------
 * 1. 选择SPI接口（如SPI1）
 * 2. 参数设置：
 *    - Mode: Full-Duplex Master  (全双工主机模式，当然不接收数据可以选择Transmit Only Master)
 *    - Data Size: 8 bits         (数据手册Page10 Table6)
 *    - Clock Polarity (CPOL): Low
 *    - Clock Phase (CPHA): 1 Edge
 *    （对应SPI Mode 0，符合Page10 Figure3时序）
 *    - NSS Signal: Disable       (使用软件控制CS引脚)
 *    - Baud Rate: ≤10 MHz        (数据手册Page1 Features)
 * 3. 引脚分配：
 *    - SCK:  指定时钟引脚
 *    - MOSI: 指定数据输出
 *    - MISO: 仅AD8403需要，其他型号可以选择Transmit Only Master来节省引脚
 * [步骤2] GPIO配置
 * ----------------------------------------------------------
 * 1. CS引脚：
 *    - 引脚上左键模式: Output Push-Pull
 *    - 右键Enter User Label标注AD840X_CS
 *
 * 2. SHDN引脚（仅AD8402/AD8403）：
 * 不接单片机可以接高电平
 *    - 引脚上左键模式: Output Push-Pull
 *    - 右键Enter User Label标注AD840X_SHDN
 *
 * 3. RS引脚（仅AD8402/AD8403）：
 * 不接单片机可以接高电平
 *    - 引脚上左键模式: Output Push-Pull
 *    - 右键Enter User Label标注AD840X_RS
 */
/*
 * ====================== 接线方式 ======================
 * 参考数据手册 Page12-13 Pin Configurations
 * ------------------------------------------------------
 * 引脚名称      连接说明                 注意事项
 * -------------------------------------------------------
 * 1. SPI接口：
 *    CS        -> AD840X_CS     低电平有效，需10ns以上脉冲（Page10 Table4）
 *    CLK       -> SPI_SCK      支持Mode0/3（CPOL=0/CPHA=0，Page10 Figure3）
 *    SDI       -> SPI_MOSI     数据输入，MSB优先
 *    SDO       -> SPI_MISO     仅AD8403需要，需4.7kΩ上拉（Page22 Figure50）,不接收数据可以不接
 *
 * 2. 控制引脚：
 *    SHDN      -> AD840X_SHDN  低电平有效（仅AD8402/AD8403有，Page12），这个引脚不能浮空
 *    RS        -> AD840X_RS    低电平复位到中值（Page12），这个引脚也不能浮空
 *
 * 3. 电源引脚：
 *    VDD       -> 2.7-5.5V     需加0.1μF去耦电容（Page24 Applications）
 *    DGND      -> 数字地        必须与AGND单点共地（Page12 Note1）
 *    AGNDx     -> 模拟地        每个通道独立接地（Page13）
 *
 * 4. 模拟端子：
 *    Ax        -> 电阻高端      可接VDD或信号输入
 *    Wx        -> 滑动端        输出端，建议接高阻抗负载
 *    Bx        -> 电阻低端      可接GND或信号输入
 *
 * 典型接线示例（AD8403）：
 *    A1 --[信号输入]  W1 --[输出]  B1 -- GND
 *    A2 -- VDD       W2 --[负载]  B2 -- GND
 */

/*
 * ====================== 注意事项 ======================
 * 1. 未使用通道处理：
 *    - 将Ax/Bx接地，Wx悬空（Page24 Applications）
 *
 * 2. 热插拔保护：
 *    - 所有引脚内置ESD保护（Page11 ESD Caution）
 *    - 避免超过绝对最大额定值（Page11 Table5）
 *
 * 3. 噪声抑制：
 *    - AGND与DGND单点连接（Page12 Note1）
 *    - 高频应用时并联10pF电容（Page24 Figure41）
 */

#ifndef __AD840X_H
#define __AD840X_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "main.h"
#include "spi.h"

/* AD840X通道定义
 * 对应数据手册Page22 Table13：
 * AD8400：只有通道1
 * AD8402：有通道1和通道2
 * AD8403：有通道1、2、3和4
 */
#define AD840X_CHANNEL_1 0b00000000 // 通道1
#define AD840X_CHANNEL_2 0b00000001 // 通道2
#define AD840X_CHANNEL_3 0b00000010 // 通道3，只有AD8403有通道3
#define AD840X_CHANNEL_4 0b00000011 // 通道4，只有AD8403有通道4

    /* 函数声明 */

    void AD840X_Init(SPI_HandleTypeDef *hspi);
    void AD840X_Write(uint8_t channel, uint8_t value);

    /**
     * @brief  使用DMA方式写入AD840X
     * @param  channel: 通道地址（2位）
     * @param  value: 8位电阻值（0-255）
     * @note   使用DMA方式时，需要在STM32CubeMX中配置SPI的DMA传输
     * @retval None
     */
    void AD840X_Write_DMA(uint8_t channel, uint8_t value);

    /**
     * @brief  通过RS引脚复位所有通道到中间值
     * @param  None
     * @note   时序需满足tRS≥50ns（Page10 Table4）
     * @ref    Page12 Pin Descriptions, Page20 Programming
     */
    void AD840X_Reset(void);

    /**
     * @brief  控制SHDN引脚进入/退出低功耗模式
     * @param  state: 0-进入断电模式，1-恢复正常模式
     * @note   仅AD8402/AD8403有效，AD8400需忽略此函数
     * @ref    Page12 Pin Descriptions, Page20 Theory of Operation
     */
    void AD840X_Shutdown(uint8_t state);

#ifdef __cplusplus
}
#endif
#endif /* __AD840X_H */
       /*
        *             /\_____/\
        *            /  o   o  \
        *           ( ==  ^  == )
        *            )         (
        *           (           )
        *          ( (  )   (  ) )
        *         (__(__)___(__)__)
        *
        *            雪豹  编写
        */