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

    // // 初始化4个通道为中间值（128）
    // AD840X_Write(AD840X_CHANNEL_A, 128);
    // AD840X_Write(AD840X_CHANNEL_B, 128);
    // AD840X_Write(AD840X_CHANNEL_C, 128);
    // AD840X_Write(AD840X_CHANNEL_D, 128);
}

/*
 * AD840X写操作函数
 * 参数：channel - 通道地址（2位，见表13 Page22）
 *       value   - 8位电阻值（0-255）
 * 数据手册参考：
 *   - 数据格式：Page11 Table6（10位：2位地址+8位数据）
 *   - 时序图：Page10 Figure3/Figure4
 */
void AD840X_Write(uint8_t channel, uint8_t value)
{
    uint8_t tx_data[2];

    /* 数据包构造（Table6 Page11）*/
    tx_data[0] = channel; // 地址位在Bit9-Bit8（两位）
    tx_data[1] = value;   // 数据位在Bit7-Bit0

    /* CS拉低（满足tCSS >10ns，Page10 Table4）*/
    HAL_GPIO_WritePin(AD840X_CS_GPIO_Port, AD840X_CS_Pin, GPIO_PIN_RESET);
    /* SPI传输（CPOL=0/CPHA=0，依据Page10 Figure3时序）*/
    HAL_SPI_Transmit(&hspi1, tx_data, 2, HAL_MAX_DELAY);
    /* CS拉高（满足tCSW >10ns，Page10 Table4）*/
    HAL_GPIO_WritePin(AD840X_CS_GPIO_Port, AD840X_CS_Pin, GPIO_PIN_SET);
}