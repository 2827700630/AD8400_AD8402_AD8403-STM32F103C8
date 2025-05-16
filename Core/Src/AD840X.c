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

    /* 默认情况下，假设SHDN和RS引脚未连接到STM32 */
    hdev->shdn_port = NULL;
    hdev->shdn_pin = PIN_NOT_CONNECTED;
    hdev->rs_port = NULL;
    hdev->rs_pin = PIN_NOT_CONNECTED;

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
}

/**
 * @brief  配置设备的SHDN和RS引脚（如果使用）
 * @param  hdev: AD840X设备句柄指针
 * @param  shdn_port: SHDN引脚端口，如不使用则传NULL
 * @param  shdn_pin: SHDN引脚
 * @param  rs_port: RS引脚端口，如不使用则传NULL
 * @param  rs_pin: RS引脚
 * @note   如果不连接到STM32，请确保这些引脚连接到高电平(VDD)
 * @retval None
 */
void AD840X_Config_Pins(AD840X_HandleTypeDef *hdev, 
                       GPIO_TypeDef *shdn_port, uint16_t shdn_pin,
                       GPIO_TypeDef *rs_port, uint16_t rs_pin)
{
    /* 配置SHDN引脚 */
    if (shdn_port == NULL)
    {
        hdev->shdn_port = NULL;
        hdev->shdn_pin = PIN_NOT_CONNECTED;
        /* 在配置函数中不输出警告，只在实际使用功能时输出 */
    }
    else
    {
        hdev->shdn_port = shdn_port;
        hdev->shdn_pin = shdn_pin;
        
        /* 默认设置为高电平（正常工作模式） */
        HAL_GPIO_WritePin(hdev->shdn_port, hdev->shdn_pin, GPIO_PIN_SET);
    }

    /* 配置RS引脚 */
    if (rs_port == NULL)
    {
        hdev->rs_port = NULL;
        hdev->rs_pin = PIN_NOT_CONNECTED;
        /* 在配置函数中不输出警告，只在实际使用功能时输出 */
    }
    else
    {
        hdev->rs_port = rs_port;
        hdev->rs_pin = rs_pin;
        
        /* 默认设置为高电平（正常工作模式） */
        HAL_GPIO_WritePin(hdev->rs_port, hdev->rs_pin, GPIO_PIN_SET);
    }
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
 * @brief  通过RS引脚复位所有通道到中间值
 * @param  hdev: AD840X设备句柄指针
 * @note   时序需满足tRS≥50ns（Page10 Table4）
 * @note   如果RS引脚未连接到STM32，将通过SPI写入中间值实现复位
 * @ref    Page12 Pin Descriptions, Page20 Programming
 */
void AD840X_Reset(AD840X_HandleTypeDef *hdev)
{
    if (hdev->rs_port == NULL || hdev->rs_pin == PIN_NOT_CONNECTED)
    {
        /* 引脚未连接时发出警告 */
        #warning "RS pin not connected to STM32. Using SPI commands to reset to mid-scale. Make sure RS pin is pulled up to VDD externally."//RS引脚未连接STM32,请连接高电平。这里使用SPI设置为中值的方式复位
        //SHDN和RS引脚不连接单片机时，请连接高电平
        
        /* 如果未连接RS引脚，则通过SPI写入中间值（128）到所有通道 */
        AD840X_Write(hdev, AD840X_CHANNEL_1, 128);
        AD840X_Write(hdev, AD840X_CHANNEL_2, 128);
        AD840X_Write(hdev, AD840X_CHANNEL_3, 128);
        AD840X_Write(hdev, AD840X_CHANNEL_4, 128);
    }
    else
    {
        /* RS低脉冲触发复位 */
        HAL_GPIO_WritePin(hdev->rs_port, hdev->rs_pin, GPIO_PIN_RESET);
        // 短延时，确保至少50ns
        for(volatile uint8_t i = 0; i < 5; i++); 
        HAL_GPIO_WritePin(hdev->rs_port, hdev->rs_pin, GPIO_PIN_SET);
    }
}

/**
 * @brief  控制SHDN引脚进入/退出低功耗模式
 * @param  hdev: AD840X设备句柄指针
 * @param  state: 0-进入断电模式，1-恢复正常模式
 * @note   仅AD8402/AD8403有效，AD8400需忽略此函数
 * @note   如果SHDN引脚未连接到STM32，此函数不会执行任何操作
 * @ref    Page12 Pin Descriptions, Page20 Theory of Operation
 */
void AD840X_Shutdown(AD840X_HandleTypeDef *hdev, uint8_t state)
{
    if (hdev->shdn_port == NULL || hdev->shdn_pin == PIN_NOT_CONNECTED)
    {
        /* 引脚未连接时发出警告 */
        #warning "SHDN pin not connected to STM32. Cannot control shutdown mode. Make sure SHDN pin is pulled up to VDD externally for normal operation."//SHDN引脚不连接单片机时,这个函数无效
        //SHDN和RS引脚不连接单片机时，请连接高电平
        return;
    }

    /* SHDN低电平有效 */
    HAL_GPIO_WritePin(hdev->shdn_port, hdev->shdn_pin,
                     state ? GPIO_PIN_SET : GPIO_PIN_RESET);

    /* 退出断电模式后需等待稳定（参考Page4 Table1的ts参数）*/
    if (state)
    {
        HAL_Delay(1); // 至少等待2μs（根据ts=2μs@10kΩ）
    }
}

/**
 * @brief  计算8位控制值（0-255）对应的比例
 * @param  ratio: 所需比例（0.0~1.0对应0%~100%）
 * @retval 8位控制值（0x00~0xFF）
 * @note   自动钳制超限值，0.5对应中值0x80
 */
uint8_t AD840X_CalculateRatio(float ratio)
{
    /* 参数保护 */
    if(ratio < 0.0f) ratio = 0.0f;
    if(ratio > 1.0f) ratio = 1.0f;
    
    /* 计算并四舍五入 */
    return (uint8_t)(ratio * 255.0f + 0.5f);
}

/**
 * @brief  基于分压比例设置数字电位器
 * @param  hdev: AD840X设备句柄指针
 * @param  channel: 通道地址（AD840X_CHANNEL_x）
 * @param  ratio: 分压比例（0.0~1.0）
 * @retval 实际设置的分压比例值
 */
float AD840X_WriteRatio(AD840X_HandleTypeDef *hdev, uint8_t channel, float ratio)
{
    uint8_t value = AD840X_CalculateRatio(ratio);
    AD840X_Write(hdev, channel, value);
    return value / 255.0f; // 返回实际设置的比例值
}

/**
 * @brief  基于实际电阻值设置数字电位器
 * @param  hdev: AD840X设备句柄指针
 * @param  channel: 通道地址（AD840X_CHANNEL_x）
 * @param  resistance: 目标电阻值（欧姆）
 * @param  full_scale: 设备满量程电阻值（欧姆）
 * @retval 实际设置的电阻值（欧姆）
 */
float AD840X_WriteResistance(AD840X_HandleTypeDef *hdev, uint8_t channel, 
                          float resistance, float full_scale)
{
    float ratio;
    uint8_t value;
    
    /* 限制电阻值在有效范围内 */
    if (resistance <= 0.0f) {
        ratio = 0.0f;
    } else if (resistance >= full_scale) {
        ratio = 1.0f;
    } else {
        ratio = resistance / full_scale;
    }
    
    /* 设置电阻器 */
    value = AD840X_CalculateRatio(ratio);
    AD840X_Write(hdev, channel, value);
    
    /* 返回实际设置的电阻值 */
    return (value / 255.0f) * full_scale;
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