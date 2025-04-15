/*
 * AD840X系列数字电位器驱动库
 * 雪豹  编写   github.com/2827700630
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
 * 1. 选择SPI接口（比如SPI1）
 * 2. 参数设置：
 *    - Mode: Full-Duplex Master  (全双工主机模式，当然不接收数据可以选择Transmit Only Master)
 *    - Data Size: 8 bits         (数据手册Page10 Table6)
 *    - Clock Polarity (CPOL): Low
 *    - Clock Phase (CPHA): 1 Edge
 *    （对应SPI Mode 0，符合Page10 Figure3时序）
 *    - NSS Signal: Disable       (使用软件控制CS引脚)
 *    - Baud Rate: ≤10 MHz        (数据手册Page1 Features)
 *    如何开启SPI的DMA
 *    - 在SPI中找到"DMA Settings"标签并点击，在DMA设置部分，点击"Add"按钮，添加一个传输请求
 *    - 在弹出的配置中选择：
 *    - DMA Request: SPI1_TX (SPI1的DMA请求)
 *    - Direction: Memory To Peripheral (用于发送数据)
 *    - Priority: 优先级，根据需要选择（Medium或High）
 *    - Mode: 选择 Normal (单次传输)
 *    - 其他参数保持默认即可
 * 3. 引脚分配：
 *    - SCK:  指定时钟引脚
 *    - MOSI: 指定数据输出
 *    - MISO: 仅AD8403需要，其他型号可以选择Transmit Only Master来节省引脚
 * [步骤2] GPIO配置,推荐使用STM32CubeMX的右键标签配置GPIO引脚
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

/* 定义未连接引脚的标志值 */
#define PIN_NOT_CONNECTED 0xFFFF

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

/* 设备句柄结构体定义 */
typedef struct 
{
    SPI_HandleTypeDef *hspi;    // SPI句柄
    GPIO_TypeDef *cs_port;      // CS端口 (必须)
    uint16_t cs_pin;            // CS引脚 (必须)
    
    GPIO_TypeDef *shdn_port;    // SHDN端口 (可选)
    uint16_t shdn_pin;          // SHDN引脚 (可选)，PIN_NOT_CONNECTED表示未连接
    
    GPIO_TypeDef *rs_port;      // RS端口 (可选)
    uint16_t rs_pin;            // RS引脚 (可选)，PIN_NOT_CONNECTED表示未连接
    
    uint8_t use_dma;            // 是否使用DMA传输
} AD840X_HandleTypeDef;

    /* 函数声明 */

    /**
     * @brief  初始化AD840X数字电位器
     * @param  hdev: AD840X设备句柄指针
     * @param  hspi: SPI句柄指针
     * @param  cs_port: CS引脚端口
     * @param  cs_pin: CS引脚
     * @note   如果SHDN或RS未连接到STM32，请确保它们连接到高电平(VDD)
     * @retval None
     */
    void AD840X_Init(AD840X_HandleTypeDef *hdev, SPI_HandleTypeDef *hspi, 
                    GPIO_TypeDef *cs_port, uint16_t cs_pin);

    /**
     * @brief  配置设备的SHDN和RS引脚（如果使用）
     * @param  hdev: AD840X设备句柄指针
     * @param  shdn_port: SHDN引脚端口，如不使用则传NULL
     * @param  shdn_pin: SHDN引脚
     * @param  rs_port: RS引脚端口，如不使用则传NULL
     * @param  rs_pin: RS引脚
     * @note   如果不连接到STM32，请确保这些引脚外部连接到高电平(VDD)
     * @retval None
     */
    void AD840X_Config_Pins(AD840X_HandleTypeDef *hdev, 
                           GPIO_TypeDef *shdn_port, uint16_t shdn_pin,
                           GPIO_TypeDef *rs_port, uint16_t rs_pin);

    /**
     * @brief  AD840X写操作函数
     * @param  hdev: AD840X设备句柄指针
     * @param  channel: 通道地址（2位，见表13 Page22）
     * @param  value: 8位电阻值（0-255）
     * @note   - 如果初始化时检测到SPI配置了DMA，将自动使用DMA方式传输
     *         - 数据格式：Page11 Table6（10位：2位地址+8位数据）
     *         - 时序图：Page10 Figure3/Figure4
     * @retval None
     */
    void AD840X_Write(AD840X_HandleTypeDef *hdev, uint8_t channel, uint8_t value);

    /**
     * @brief  通过RS引脚复位所有通道到中间值
     * @param  hdev: AD840X设备句柄指针
     * @note   时序需满足tRS≥50ns（Page10 Table4）
     * @note   如果RS引脚未连接到STM32，此函数将通过SPI写入中间值
     * @ref    Page12 Pin Descriptions, Page20 Programming
     */
    void AD840X_Reset(AD840X_HandleTypeDef *hdev);

    /**
     * @brief  控制SHDN引脚进入/退出低功耗模式
     * @param  hdev: AD840X设备句柄指针
     * @param  state: 0-进入断电模式，1-恢复正常模式
     * @note   仅AD8402/AD8403有效，AD8400需忽略此函数
     * @note   如果SHDN引脚未连接到STM32，此函数将不执行任何操作
     * @ref    Page12 Pin Descriptions, Page20 Theory of Operation
     */
    void AD840X_Shutdown(AD840X_HandleTypeDef *hdev, uint8_t state);

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