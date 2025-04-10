/*
 * AD840X系列数字电位器驱动库
 * 雪豹  编写
 */
#include "AD840X.h"

/**
 * @brief  初始化AD840X数字电位器,并初始化所有通道为中间值
 * @param  None
 * @retval None
 */
void AD840X_Init(void)
{
    // 初始化时将CS引脚拉高
    HAL_GPIO_WritePin(AD840X_CS_GPIO_Port, AD840X_CS_Pin, GPIO_PIN_SET);
}

/*
 * AD840X写操作函数
 * 参数：channel - 通道地址（2位，见表13 Page22）
 *       value   - 8位电阻值（0-255）
 * 数据手册参考：
 *   - 数据格式：Page11 Table6（10位：2位地址+8位数据）
 *   - 时序图：Page10 Figure3/Figure4
 *   - 地址解码：Page22 Table13
 *   - 电气特性：Page4-9 Tables1-4
 */
void AD840X_Write(uint8_t channel, uint8_t value)
{
    uint8_t tx_data[2];

    /* 数据包构造（Table6 Page11）
     * Bit9-8: 通道地址 (A1A0)
     * Bit7-0: 电阻值 (D7-D0)
     * 注意：AD8400固定使用00地址（单通道）*/
    tx_data[0] = channel;
    tx_data[1] = value;

    /* CS拉低（满足tCSS >10ns，Page10 Table4）
     * 注意：实际使用时需考虑MCU的GPIO速度*/
    HAL_GPIO_WritePin(AD840X_CS_GPIO_Port, AD840X_CS_Pin, GPIO_PIN_RESET);

    /* SPI传输（在cubeMX中SPI设置CPOL=low，CPHA=1 edge，依据Page10 Figure3时序）
     * 建议时钟频率不超过10MHz（Page1 Features）*/
    HAL_SPI_Transmit(&hspi1, tx_data, 2, HAL_MAX_DELAY);

    /* CS拉高（满足tCSW >10ns，Page10 Table4）
     * 上升沿锁存数据（Page21 Digital Interfacing部分）*/
    HAL_GPIO_WritePin(AD840X_CS_GPIO_Port, AD840X_CS_Pin, GPIO_PIN_SET);
}

/**
 * @brief  控制SHDN引脚进入/退出低功耗模式
 * @param  state: 0-进入断电模式，1-恢复正常模式
 * @note   仅AD8402/AD8403有效，AD8400需忽略此函数
 * @ref    Page12 Pin Descriptions, Page20 Theory of Operation
 */
void AD840X_Shutdown(uint8_t state)
{
    /* SHDN低电平有效，硬件需连接至GPIO */
    HAL_GPIO_WritePin(AD840X_SHDN_GPIO_Port, AD840X_SHDN_Pin,
                      state ? GPIO_PIN_RESET : GPIO_PIN_SET);

    /* 退出断电模式后需等待稳定（参考Page4 Table1的ts参数）*/
    if (!state)
    {
        HAL_Delay(1); // 至少等待2μs（根据ts=2μs@10kΩ）
    }
}

/**
 * @brief  通过RS引脚复位所有通道到中间值
 * @param  None
 * @note   时序需满足tRS≥50ns（Page10 Table4）
 * @ref    Page12 Pin Descriptions, Page20 Programming
 */
void AD840X_Reset(void)
{
    /* RS低脉冲触发复位 */
    HAL_GPIO_WritePin(AD840X_RS_GPIO_Port, AD840X_RS_Pin, GPIO_PIN_RESET);
    HAL_Delay(1); // 实际应用中可用更精准的延时（如1μs）
    HAL_GPIO_WritePin(AD840X_RS_GPIO_Port, AD840X_RS_Pin, GPIO_PIN_SET);
}