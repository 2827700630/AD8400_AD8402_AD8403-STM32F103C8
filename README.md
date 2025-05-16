# AD8400_AD8402_AD8403-STM32F103C8
 使用STM32F103C8控制数字电位器AD840X，使用CubeIDE编写，也可以使用VSCode打开

# AD840X数字电位器驱动库使用说明
多芯片连接因为我没时间接线所以没测试

## 下一步优化计划

- [ ] 将HAL库代码独立拆分为模块，方便移植到其他微控制器（如ZYNQ、MSP系列、CH系列RISC-V微控制器）
- [ ] 开发完整的硬件测试套件和示例
- [ ] 增加STM32F407的示例代码
- [ ] 必要时为MSPM0G3507微控制器实现示例程序
- [ ] 误差如何快速校准

## 基本信息

- **项目名称**: AD8400/AD8402/AD8403 数字电位器驱动库
- **例程使用芯片**: STM32F103C8（可以方便移植到其他STM32芯片，见AD840X.h）
- **开发环境**: STM32CubeIDE / VSCode
- **作者**: 雪豹
- **版本**: 2.0

## 更新记录

- **2025.4.15 v2.0**:
  - 新增多设备支持，可同时控制多个AD840X器件
  - 添加引脚未连接检测和编译警告
  - 重构代码，使用结构体管理设备状态
  - 优化条件逻辑，使代码更易读

- **2025.4.7 v1.0**:
  - 首次发布，支持单设备控制

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

2. **特殊功能**:
   - 多设备支持，可同时控制多个AD840X器件
   - 支持硬件复位(通过RS引脚)
   - 支持低功耗模式控制(通过SHDN引脚)

3. **优化特性**:
   - 代码注释丰富，符合doxygen标准
   - 引脚配置说明详细
   - 时序控制符合数据手册要求
   - 提供编译期警告，确保引脚正确连接

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


#### GPIO配置
1. **CS引脚** (必须):
   - 模式: Output Push-Pull
   - 用户标签: AD840X_CS1, AD840X_CS2, ... (多设备时需定义多个)

2. **SHDN引脚** (可选，仅AD8402/AD8403有此引脚):
   - 模式: Output Push-Pull
   - 用户标签: AD840X_SHDN1, AD840X_SHDN2, ...
   - 注意: 未连接到STM32时，必须外部上拉至VDD

3. **RS引脚** (可选，仅AD8402/AD8403有此引脚):
   - 模式: Output Push-Pull
   - 用户标签: AD840X_RS1, AD840X_RS2, ...
   - 注意: 未连接到STM32时，必须外部上拉至VDD

### 硬件连接

```
STM32 <---> AD840X
--------------------------------
SPI_SCK  --> CLK
SPI_MOSI --> SDI
SPI_MISO <-- SDO (仅AD8403需要，需4.7kΩ上拉)
GPIO     --> CS  (每个设备需要一个独立的CS引脚)
GPIO     --> SHDN (可选，未连接需上拉)
GPIO     --> RS (可选，未连接需上拉)
```

## 软件使用方法

### 1. 初始化

在主程序中包含头文件:
```c
#include "AD840X.h" // 引入AD840X驱动库
```

定义设备句柄:
```c
// 定义多个设备句柄，连接多少个设备就定义多少个
AD840X_HandleTypeDef hAD840X_1;  // 第一个设备
AD840X_HandleTypeDef hAD840X_2;  // 第二个设备
AD840X_HandleTypeDef hAD840X_3;  // 第三个设备
```

初始化函数:
```c
// 初始化第一个设备，这里的AD840X_CS1需要手动在cubeMX中设置
AD840X_Init(&hAD840X_1, &hspi1, AD840X_CS1_GPIO_Port, AD840X_CS1_Pin);
AD840X_Config_Pins(&hAD840X_1, AD840X_SHDN1_GPIO_Port, AD840X_SHDN1_Pin,
                  AD840X_RS1_GPIO_Port, AD840X_RS1_Pin);

// 初始化第二个设备
AD840X_Init(&hAD840X_2, &hspi1, AD840X_CS2_GPIO_Port, AD840X_CS2_Pin);
AD840X_Config_Pins(&hAD840X_2, AD840X_SHDN2_GPIO_Port, AD840X_SHDN2_Pin,
                     AD840X_RS2_GPIO_Port, AD840X_RS2_Pin);

// 初始化不带SHDN和RS的设备（这些引脚必须外部上拉至VDD）
AD840X_Init(&hAD840X_3, &hspi1, AD840X_CS3_GPIO_Port, AD840X_CS3_Pin);
```

### 2. 基本控制

设置通道阻值:
```c
// 设置第一个设备的通道1阻值为128 (50%)
AD840X_Write(&hAD840X_1, AD840X_CHANNEL_1, 128);

// 设置第二个设备的通道2阻值为255 (最大阻值100%)
AD840X_Write(&hAD840X_2, AD840X_CHANNEL_2, 255);
```

### 3. 特殊功能

#### 硬件复位
将所有通道重置为中间值(128):
```c
// 使用RS引脚重置第一个设备
AD840X_Reset(&hAD840X_1);

// 使用SPI命令重置未连接RS引脚的设备
AD840X_Reset(&hAD840X_3);  // 自动使用SPI命令写入中间值
```

#### 低功耗控制
```c
// 第一个设备进入低功耗模式
AD840X_Shutdown(&hAD840X_1, 0);

// 第一个设备退出低功耗模式
AD840X_Shutdown(&hAD840X_1, 1);

// 未连接SHDN引脚的设备将无法控制低功耗模式
AD840X_Shutdown(&hAD840X_3, 0); // 无效，会产生编译警告
```


## 注意事项

1. **硬件连接**:
   - 每个设备需要单独的CS引脚
   - SHDN和RS引脚不能悬空，未连接STM32时必须接高电平
   - 未连接引脚的情况会在编译时发出警告
   - 确保AGND与DGND单点连接

2. **未使用通道**:
   - 将Ax/Bx接地，Wx悬空

3. **防干扰措施**:
   - VDD处加0.1μF去耦电容
   - 高频应用时并联10pF电容

## 故障排除

1. **数值不准确**:
   - 检查SPI时序配置
   - 确认CS引脚正确控制

2. **通道无响应**:
   - 检查SHDN引脚状态
   - 验证相关通道在芯片中是否可用

3. **编译警告**:
   - 如果看到关于SHDN或RS引脚的警告，请确保这些引脚已连接到高电平
   - 只有在使用相关功能时才会显示警告，如不需要低功耗控制功能则可忽略SHDN相关警告
