/*
 * AD840X系列数字电位器驱动库
 * 雪豹  编写
 */
#include "AD840X.h"

/* 私有变量 */
static SPI_HandleTypeDef *ad840x_spi; // 存储SPI句柄
static uint8_t ad840x_use_dma = 0;    // DMA使用标志

/**
 * @brief  初始化AD840X数字电位器
 * @param  hspi: SPI句柄指针
 * @note   括号里输入使用的SPI通道，如果使用的是SPI1，就填写&hspi1
 * @retval None
 */
void AD840X_Init(SPI_HandleTypeDef *hspi)
{
    // 保存SPI句柄
    ad840x_spi = hspi;

    // 检查SPI是否配置了DMA
    if (hspi->hdmatx != NULL)
    {
        ad840x_use_dma = 1; // SPI已配置DMA
    }
    else
    {
        ad840x_use_dma = 0; // SPI未配置DMA
    }

    // 初始化时将CS引脚拉高
    HAL_GPIO_WritePin(AD840X_CS_GPIO_Port, AD840X_CS_Pin, GPIO_PIN_SET);
}

/**
 * @brief  AD840X写操作函数
 * @param  channel: 通道地址（2位，见表13 Page22）
 * @param  value: 8位电阻值（0-255）
 * @note   - 如果初始化时检测到SPI配置了DMA，将自动使用DMA方式传输
 *         - 数据手册参考：
 *         - 数据格式：Page11 Table6（10位：2位地址+8位数据）
 *         - 时序图：Page10 Figure3/Figure4
 *         - 地址解码：Page22 Table13
 *         - 电气特性：Page4-9 Tables1-4
 * @retval None
 */
void AD840X_Write(uint8_t channel, uint8_t value)
{
    static uint8_t tx_data[2];

    /* 数据包构造（Table6 Page11）*/
    tx_data[0] = channel; // 地址位在Bit9-Bit8，这里为了方便传输把地址填写为8位
    tx_data[1] = value;   // 数据位在Bit7-Bit0

    /* CS拉低（满足tCSS >10ns，Page10 Table4）*/
    HAL_GPIO_WritePin(AD840X_CS_GPIO_Port, AD840X_CS_Pin, GPIO_PIN_RESET);

    /* 根据初始化时检测到的DMA状态选择传输方式 */
    if (ad840x_use_dma == 1) // 使用DMA方式
    {
        /* 使用DMA方式传输数据（CS将在DMA完成回调中拉高） */
        HAL_SPI_Transmit_DMA(ad840x_spi, tx_data, 2);
        /* 注意：CS引脚将在传输完成回调函数中拉高 */
    }
    else
    {
        /* 使用阻塞方式传输数据 */
        HAL_SPI_Transmit(ad840x_spi, tx_data, 2, HAL_MAX_DELAY);

        /* CS拉高（满足tCSW >10ns，Page10 Table4）*/
        HAL_GPIO_WritePin(AD840X_CS_GPIO_Port, AD840X_CS_Pin, GPIO_PIN_SET);
    }
}

/**
 * @brief  SPI DMA传输完成回调函数
 * @note   此函数会被HAL库自动调用，不需要用户手动调用
 * @param  hspi: SPI句柄指针
 * @retval None
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    /* 检查是否为AD840X使用的SPI实例 */
    if (hspi == ad840x_spi)
    {
        /* 传输完成后，拉高CS引脚，锁存数据 */
        HAL_GPIO_WritePin(AD840X_CS_GPIO_Port, AD840X_CS_Pin, GPIO_PIN_SET);
    }
}

/**
 * @brief  控制SHDN引脚进入/退出低功耗模式
 * @param  state: 0-进入断电模式，1-恢复正常模式
 * @note   仅AD8402/AD8403有效，AD8400需忽略此函数
 * @ref    Page12 Pin Descriptions, Page20 Theory of Operation
 */
void AD840X_Shutdown(uint8_t state)
{
#ifdef AD840X_SHDN_GPIO_Port // 如果定义了SHDN引脚
    /* SHDN低电平有效 */
    HAL_GPIO_WritePin(AD840X_SHDN_GPIO_Port, AD840X_SHDN_Pin,
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);

    /* 退出断电模式后需等待稳定（参考Page4 Table1的ts参数）*/
    if (state)
    {
        HAL_Delay(1); // 至少等待2μs（根据ts=2μs@10kΩ）
    }
#endif
}

/**
 * @brief  通过RS引脚复位所有通道到中间值
 * @param  None
 * @note   时序需满足tRS≥50ns（Page10 Table4）
 * @ref    Page12 Pin Descriptions, Page20 Programming
 */
void AD840X_Reset(void)
{
#ifdef AD840X_RS_GPIO_Port // 如果定义了RS引脚
    /* RS低脉冲触发复位 */
    HAL_GPIO_WritePin(AD840X_RS_GPIO_Port, AD840X_RS_Pin, GPIO_PIN_RESET);
    // HAL_Delay(1); // 实际应用中可用更精准的延时（如1μs）
    HAL_GPIO_WritePin(AD840X_RS_GPIO_Port, AD840X_RS_Pin, GPIO_PIN_SET);
#endif
}
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