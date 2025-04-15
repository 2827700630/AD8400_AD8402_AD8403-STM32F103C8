/*
 * AD840X系列数字电位器驱动库
 * 雪豹  编写
 */
#include "AD840X.h"

/* 用于DMA传输的全局变量 */
static uint8_t tx_data_dma[2];
static AD840X_HandleTypeDef *current_dma_device = NULL;

/**
 * @brief  初始化AD840X数字电位器
 * @param  hdev: AD840X设备句柄指针
 * @param  hspi: SPI句柄指针
 * @param  cs_port: CS引脚端口
 * @param  cs_pin: CS引脚
 * @note   为每个AD840X设备调用一次
 * @retval None
 */
void AD840X_Init(AD840X_HandleTypeDef *hdev, SPI_HandleTypeDef *hspi, 
                GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    /* 初始化设备句柄 */
    hdev->hspi = hspi;
    hdev->cs_port = cs_port;
    hdev->cs_pin = cs_pin;

    /* 检查SPI是否配置了DMA */
    if (hspi->hdmatx != NULL)
    {
        hdev->use_dma = 1; // SPI已配置DMA
    }
    else
    {
        hdev->use_dma = 0; // SPI未配置DMA
    }

    /* 初始化时将CS引脚拉高 */
    HAL_GPIO_WritePin(hdev->cs_port, hdev->cs_pin, GPIO_PIN_SET);

#ifdef AD840X_SHDN_GPIO_Port
    /* 默认使用默认的引脚定义，如果需要自定义，可以后续调用AD840X_Config_Pins */
    hdev->shdn_port = AD840X_SHDN_GPIO_Port;
    hdev->shdn_pin = AD840X_SHDN_Pin;
#endif

#ifdef AD840X_RS_GPIO_Port
    hdev->rs_port = AD840X_RS_GPIO_Port;
    hdev->rs_pin = AD840X_RS_Pin;
#endif

    /* 初始化所有通道为中间值（128） */
    AD840X_Reset(hdev);
}

/**
 * @brief  配置设备的SHDN和RS引脚（如果使用）
 * @param  hdev: AD840X设备句柄指针
 * @param  shdn_port: SHDN引脚端口
 * @param  shdn_pin: SHDN引脚
 * @param  rs_port: RS引脚端口
 * @param  rs_pin: RS引脚
 * @retval None
 */
void AD840X_Config_Pins(AD840X_HandleTypeDef *hdev, 
                       GPIO_TypeDef *shdn_port, uint16_t shdn_pin,
                       GPIO_TypeDef *rs_port, uint16_t rs_pin)
{
#ifdef AD840X_SHDN_GPIO_Port
    hdev->shdn_port = shdn_port;
    hdev->shdn_pin = shdn_pin;
#endif

#ifdef AD840X_RS_GPIO_Port
    hdev->rs_port = rs_port;
    hdev->rs_pin = rs_pin;
#endif
}

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
void AD840X_Write(AD840X_HandleTypeDef *hdev, uint8_t channel, uint8_t value)
{
    static uint8_t tx_data[2];
    
    /* 数据包构造（Table6 Page11）*/
    tx_data[0] = channel; // 地址位在Bit9-Bit8（两位）
    tx_data[1] = value;   // 数据位在Bit7-Bit0
    
    /* CS拉低（满足tCSS >10ns，Page10 Table4）*/
    HAL_GPIO_WritePin(hdev->cs_port, hdev->cs_pin, GPIO_PIN_RESET);
    
    /* 根据初始化时检测到的DMA状态选择传输方式 */
    if (hdev->use_dma)
    {
        /* 保存当前正在使用DMA的设备 */
        current_dma_device = hdev;
        
        /* 使用DMA全局变量，确保传输过程中数据有效 */
        tx_data_dma[0] = tx_data[0];
        tx_data_dma[1] = tx_data[1];
        
        /* 使用DMA方式传输数据 */
        HAL_SPI_Transmit_DMA(hdev->hspi, tx_data_dma, 2);
        
        /* 注意：CS引脚需要在传输完成回调函数中拉高 */
        /* 这里不能直接拉高CS，因为DMA传输是异步的 */
    }
    else
    {
        /* 使用阻塞方式传输数据 */
        HAL_SPI_Transmit(hdev->hspi, tx_data, 2, HAL_MAX_DELAY);
        
        /* CS拉高（满足tCSW >10ns，Page10 Table4）*/
        HAL_GPIO_WritePin(hdev->cs_port, hdev->cs_pin, GPIO_PIN_SET);
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
    /* 检查是否有正在使用DMA的设备，且SPI实例匹配 */
    if (current_dma_device != NULL && hspi == current_dma_device->hspi)
    {
        /* 传输完成后，拉高CS引脚，锁存数据 */
        HAL_GPIO_WritePin(current_dma_device->cs_port, current_dma_device->cs_pin, GPIO_PIN_SET);
    }
}

/**
 * @brief  通过RS引脚复位所有通道到中间值
 * @param  hdev: AD840X设备句柄指针
 * @note   时序需满足tRS≥50ns（Page10 Table4）
 * @ref    Page12 Pin Descriptions, Page20 Programming
 */
void AD840X_Reset(AD840X_HandleTypeDef *hdev)
{
#ifdef AD840X_RS_GPIO_Port
    /* RS低脉冲触发复位 */
    HAL_GPIO_WritePin(hdev->rs_port, hdev->rs_pin, GPIO_PIN_RESET);
    // HAL_Delay(1); // 实际应用中可用更精准的延时（如1μs）
    HAL_GPIO_WritePin(hdev->rs_port, hdev->rs_pin, GPIO_PIN_SET);
#else
    /* 如果没有RS引脚，则通过SPI写入中间值（128）到所有通道 */
    AD840X_Write(hdev, AD840X_CHANNEL_1, 128);
    AD840X_Write(hdev, AD840X_CHANNEL_2, 128);
    AD840X_Write(hdev, AD840X_CHANNEL_3, 128);
    AD840X_Write(hdev, AD840X_CHANNEL_4, 128);
#endif
}

/**
 * @brief  控制SHDN引脚进入/退出低功耗模式
 * @param  hdev: AD840X设备句柄指针
 * @param  state: 0-进入断电模式，1-恢复正常模式
 * @note   仅AD8402/AD8403有效，AD8400需忽略此函数
 * @ref    Page12 Pin Descriptions, Page20 Theory of Operation
 */
void AD840X_Shutdown(AD840X_HandleTypeDef *hdev, uint8_t state)
{
#ifdef AD840X_SHDN_GPIO_Port
    /* SHDN低电平有效 */
    HAL_GPIO_WritePin(hdev->shdn_port, hdev->shdn_pin,
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);

    /* 退出断电模式后需等待稳定（参考Page4 Table1的ts参数）*/
    if (state)
    {
        HAL_Delay(1); // 至少等待2μs（根据ts=2μs@10kΩ）
    }
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