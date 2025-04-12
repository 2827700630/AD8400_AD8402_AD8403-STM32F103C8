# AD8400_AD8402_AD8403-STM32F103C8
 使用STM32F103C8控制数字电位器AD840X，使用CubeIDE编写，也可以使用VSCode打开

# AD840X数字电位器驱动库使用说明

## 基本信息

- **项目名称**: AD8400/AD8402/AD8403 数字电位器驱动库
- **适用芯片**: STM32F103C8
- **开发环境**: STM32CubeIDE / VSCode
- **作者**: 雪豹

## 硬件支持

驱动库支持以下数字电位器型号：
- **AD8400**: 单通道数字电位器
- **AD8402**: 双通道数字电位器
- **AD8403**: 四通道数字电位器

## 功能特点

1. **基本功能**:
   - 支持精确控制电位器阻值(8位分辨率, 256档位)
   - 支持多通道独立控制
   - 支持阻塞式SPI传输
   - 支持DMA传输方式

2. **特殊功能**:
   - 自动检测DMA状态并选择最佳传输方式
   - 支持硬件复位(通过RS引脚)
   - 支持低功耗模式控制(通过SHDN引脚)

3. **优化特性**:
   - 代码注释丰富，符合doxygen标准
   - 引脚配置说明详细
   - 时序控制符合数据手册要求

## 硬件配置

### CubeMX配置步骤

#### SPI配置
1. 选择SPI接口(如SPI1)
2. 参数设置:
   - Mode: Full-Duplex Master 或 Transmit Only Master
   - Data Size: 8 bits
   - Clock Polarity (CPOL): Low
   - Clock Phase (CPHA): 1 Edge
   - NSS Signal: Disable (软件控制CS引脚)
   - Baud Rate: ≤10 MHz

3. DMA配置(可选):
   - 如需DMA传输，为SPI添加TX DMA

#### GPIO配置
1. **CS引脚**:
   - 模式: Output Push-Pull
   - 用户标签: AD840X_CS

2. **SHDN引脚** (仅AD8402/AD8403):
   - 模式: Output Push-Pull
   - 用户标签: AD840X_SHDN

3. **RS引脚** (仅AD8402/AD8403):
   - 模式: Output Push-Pull
   - 用户标签: AD840X_RS

### 硬件连接

```
STM32 <---> AD840X
--------------------------------
SPI_SCK  --> CLK
SPI_MOSI --> SDI
SPI_MISO <-- SDO (仅AD8403需要，需4.7kΩ上拉)
GPIO     --> CS (AD840X_CS)
GPIO     --> SHDN (AD840X_SHDN, 可选)
GPIO     --> RS (AD840X_RS, 可选)
```

## 软件使用方法

### 1. 初始化

在主程序中包含头文件:
```c
#include "AD840X.h"
```

初始化函数:
```c
// 在main函数中调用初始化函数
AD840X_Init(&hspi1); // 参数为SPI句柄
```

### 2. 基本控制

设置通道阻值:
```c
// 设置通道1阻值为128 (50%)
AD840X_Write(AD840X_CHANNEL_1, 128);

// 设置通道2阻值为255 (最大阻值100%)
AD840X_Write(AD840X_CHANNEL_2, 255);

// 设置通道3阻值为0 (最小阻值0%)
AD840X_Write(AD840X_CHANNEL_3, 0);
```

### 3. 特殊功能

#### 硬件复位
将所有通道重置为中间值(128):
```c
AD840X_Reset();
```

#### 低功耗控制
```c
// 进入低功耗模式
AD840X_Shutdown(0);

// 退出低功耗模式
AD840X_Shutdown(1);
```

### 4. DMA传输

自动DMA模式:
```c
// 初始化时自动检测DMA配置
// 使用标准函数即可
AD840X_Write(AD840X_CHANNEL_1, 128); // 自动选择传输方式
```

手动DMA传输:
```c
// 如需显式指定DMA传输模式
AD840X_Write_DMA(AD840X_CHANNEL_1, 128);
```

## 注意事项

1. **硬件连接**:
   - SHDN和RS引脚不能悬空，未使用时需接高电平
   - 确保AGND与DGND单点连接

2. **使用DMA时**:
   - CS引脚在DMA传输完成回调中自动控制
   - 请确保正确配置CubeMX中的DMA设置

3. **未使用通道**:
   - 将Ax/Bx接地，Wx悬空

4. **防干扰措施**:
   - VDD处加0.1μF去耦电容
   - 高频应用时并联10pF电容

## 故障排除

1. **数值不准确**:
   - 检查SPI时序配置
   - 确认CS引脚正确控制

2. **通道无响应**:
   - 检查SHDN引脚状态
   - 验证相关通道在芯片中是否可用

3. **DMA传输问题**:
   - 确认正确配置了DMA通道
   - 检查TX完成回调是否正常工作
