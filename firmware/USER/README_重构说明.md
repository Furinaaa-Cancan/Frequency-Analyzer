# 固件重构说明

## 📁 新的文件结构

```
firmware/USER/
├── main.c                   # 主程序（仅包含main函数和初始化）
├── signal_processing.c/h    # 信号处理算法模块
├── measurement.c/h          # 测量功能模块（扫频、校准）
├── adc_handler.c/h          # ADC数据处理模块
└── README_重构说明.md       # 本文件
```

## 🎯 模块划分

### 1. signal_processing（信号处理模块）✅ 已完成

**功能**：
- `CalculatePeakToPeak()` - 峰峰值计算
- `CalculateDCOffset()` - 直流偏移计算
- `CalculateAmplitude_DFT()` - RMS幅度计算
- `EstimatePhaseShift_Int()` - 相位差计算
- `CalculateDistortion()` - 失真度计算（THD）

**依赖**：
- `gd32f10x.h`
- 数学库（`math.h`）

### 2. adc_handler（ADC处理模块）⏳ 待完成

**功能**：
- `ExtractADCData()` - 从DMA缓冲区提取双通道ADC数据
- `ProcessADCData()` - 处理ADC数据并计算频率响应

**依赖**：
- signal_processing模块
- DDS模块（`DDS_GetFrequency()`）
- 全局ADC缓冲区

### 3. measurement（测量模块）⏳ 待完成

**功能**：
- `AutoSweep()` - 自动扫频（10Hz-1000Hz）
- `AutoCalibration()` - 自动校准

**依赖**：
- signal_processing模块
- adc_handler模块
- DDS模块
- 校准数据结构

### 4. main.c（主程序）⏳ 待简化

**保留内容**：
- `main()` 函数
- 初始化代码
- 串口命令解析
- 全局变量定义
- `delay_ms()` / `delay_us()` 工具函数

**移除内容**：
- 所有信号处理函数 → signal_processing.c
- 所有ADC处理函数 → adc_handler.c
- 所有测量函数 → measurement.c

## 📋 重构步骤

- [x] 1. 创建 signal_processing.c/h ✅
- [x] 2. 创建 adc_handler.c/h ✅
- [x] 3. 创建 measurement.c/h ✅
- [ ] 4. 简化 main.c（参考：如何简化main.c.md）
- [ ] 5. 更新 Keil工程文件，添加新文件
- [ ] 6. 编译测试

## 🔧 注意事项

1. **全局变量**：
   - `g_calibration` - 校准数据（在measurement.c中定义）
   - `adc_buffer` - ADC DMA缓冲区（在main.c中定义）
   - `g_signal_type` - 信号类型（在main.c中定义）

2. **外部依赖**：
   - DDS模块函数需要正确引用
   - 确保所有头文件路径正确
   - 数学库需要在Keil中正确配置

3. **编译顺序**：
   - signal_processing（无依赖，最先编译）
   - adc_handler（依赖signal_processing）
   - measurement（依赖以上两个）
   - main（依赖所有模块）

## 💡 优势

- **可维护性**：模块化设计，职责清晰
- **可测试性**：每个模块可独立测试
- **可扩展性**：新功能可以独立添加
- **代码复用**：信号处理函数可在其他项目中复用
