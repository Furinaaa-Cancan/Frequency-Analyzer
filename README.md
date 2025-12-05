<p align="center">
  <img src="https://img.shields.io/badge/Platform-GD32F103_ARM_Cortex--M3-blue?style=for-the-badge&logo=arm" alt="Platform"/>
  <img src="https://img.shields.io/badge/React-18.0+-61DAFB?style=for-the-badge&logo=react" alt="React"/>
  <img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" alt="License"/>
  <img src="https://img.shields.io/badge/Version-5.6-orange?style=for-the-badge" alt="Version"/>
  <img src="https://img.shields.io/badge/Build-Passing-brightgreen?style=for-the-badge" alt="Build"/>
</p>

<h1 align="center">
  <br>
  <img src="docs/assets/logo.png" alt="Frequency Analyzer Logo" width="200">
  <br>
  Frequency Response Analyzer
  <br>
  <sub>频率响应分析仪</sub>
</h1>

<h4 align="center">基于 GD32F103 ARM Cortex-M3 的专业级伯德图测量与生物电信号分析系统</h4>

<p align="center">
  <a href="#-系统概述">系统概述</a> •
  <a href="#-技术架构">技术架构</a> •
  <a href="#-硬件设计">硬件设计</a> •
  <a href="#-软件架构">软件架构</a> •
  <a href="#-快速开始">快速开始</a> •
  <a href="#-api参考">API参考</a> •
  <a href="#-许可证">许可证</a>
</p>

<p align="center">
  <img src="docs/assets/demo.gif" alt="系统演示" width="700">
</p>

---

## 📋 目录

- [系统概述](#-系统概述)
- [技术架构](#-技术架构)
- [硬件设计](#-硬件设计)
- [软件架构](#-软件架构)
- [信号处理算法](#-信号处理算法)
- [双工作模式](#-双工作模式)
- [快速开始](#-快速开始)
- [API参考](#-api参考)
- [技术规格](#-技术规格)
- [项目结构](#-项目结构)
- [许可证](#-许可证)

---

## 🎯 系统概述

**Frequency Response Analyzer** 是一款高精度嵌入式测量仪器，集成了**频率响应分析**与**生物电信号采集**两大核心功能。系统基于兆易创新 GD32F103RCT6 微控制器（ARM Cortex-M3 @ 72MHz），采用直接数字频率合成（DDS）技术生成高纯度测试信号，通过双通道同步ADC采样实现精确的幅频/相频特性测量。

### 核心特性

| 功能模块 | 技术指标 | 说明 |
|---------|---------|------|
| **DDS信号发生器** | 10Hz–1kHz, ±0.01% | 32位相位累加器，256点正弦查找表 |
| **双通道ADC** | 12-bit, 同步采样 | ADC0(PA6)输入参考，ADC1(PB1)输出测量 |
| **频率响应测量** | 相位精度±1°, 幅度±0.1dB | DFT单频点提取，自动相位展开 |
| **ECG信号采集** | MIT-BIH标准数据 | 360点/心跳，2500Hz采样率 |
| **Web界面** | React 18 + Chart.js | Web Serial API，实时可视化 |

### 应用场景

- **教学实验**：模拟电路频率特性分析
- **滤波器设计**：RC/RLC网络传递函数测量
- **放大器测试**：增益-带宽特性验证
- **生物医学**：心电信号演示与分析

---

## 🏗️ 技术架构

### 系统架构

系统由三个子系统组成：

**1. 信号生成子系统**
> DDS算法(TIMER2 50kHz) → SPI0(PA5/PA7/PA4) → DAC5311(8-bit) → 三级运放滤波网络

**2. 信号采集子系统**
> TIMER3触发 → ADC0+ADC1双通道并行采样 → DMA循环传输(512点) → DFT/RMS数据处理

**3. 通信子系统**
> USART0(PA9/PA10, 115200bps) → 命令解析 → USB-UART → React Web界面

### 信号流

**正弦波模式（Bode图测量）**：
> DDS相位累加器 → 256点正弦表 → DAC5311 → 运放滤波 → **被测电路(DUT)** → PA6(输入K) / PB1(输出K₁) → ADC同步采样 → DMA → DFT提取 → **H(ω)=K₁/K, θ(ω)**

**ECG模式（心电信号）**：
> MIT-BIH波形表(360点) → 相位累加器(50Hz) → DAC5311 → 运放链路 → PA6采集(2500Hz) → DMA缓冲 → WAVEFORM数据流 → **前端显示**

---

## 🔧 硬件设计

### 模拟前端电路

**信号链路**：
> DAC5311(VOUT) → U1A(缓冲器) → C1(1.5μF) → U1B(放大器) → C2(1.5μF) → U1C(输出级)

**采集点**：
- **PA6**：U1A输出端，采集输入参考信号 K
- **PB1**：U1C输出端，采集输出测量信号 K₁

**滤波器特性**：
- 耦合电容 C1/C2 = 1.5μF，构成高通滤波器
- 截止频率 fc ≈ 1/(2πRC)
- 适用频率范围：50Hz - 1kHz

### 引脚分配表

| 功能组 | 引脚 | 方向 | 说明 |
|--------|------|------|------|
| **SPI0 (DAC5311)** | PA5 | OUT | SCK - SPI时钟 |
|  | PA7 | OUT | MOSI - SPI数据 |
|  | PA4 | OUT | CS - 片选（低有效） |
| **ADC双通道** | PA6 | IN | ADC0_CH6 - 输入参考信号 K |
|  | PB1 | IN | ADC1_CH9 - 输出测量信号 K₁ |
| **USART0** | PA9 | OUT | TX - 串口发送 |
|  | PA10 | IN | RX - 串口接收 |
| **LED指示** | PB11-15 | OUT | 5路LED状态指示 |

### 定时器资源

| 定时器 | 频率 | 功能 |
|--------|------|------|
| TIMER2 | 50kHz | DDS波形生成中断，DAC更新 |
| TIMER3 | 可变 | ADC触发源，采样率控制 |
| TIMER1 | 20kHz | LED PWM亮度控制 |

---

## 🔀 双工作模式

### 模式1：正弦波模式（Bode图测量）

```c
// 信号类型切换
g_signal_type = SIGNAL_TYPE_SINE;

// DDS频率设置（10-1000Hz）
DDS_SetFrequency(100);  // 100Hz正弦波

// ADC采样率自适应
TIMER3_SetSampleRate(freq * 10);  // 10倍过采样
```

**数据流协议**：
```
WAVEFORM:<频率>,<采样率>,<PA6值>|<PB1值>
示例: WAVEFORM:100,1000,2048|1856
```

### 模式2：ECG模式（心电信号）

```c
// 切换到ECG模式
g_signal_type = SIGNAL_TYPE_ECG;

// MIT-BIH 100号记录数据
// 来源: PhysioNet (physionet.org/content/mitdb/1.0.0)
// 患者: 69岁男性，正常窦性心律
// MLII导联，360点/心跳，原始11位ADC归一化到8位

const uint8_t ecg_wave_table[360] = {
    78, 78, 78, 78, 78, 78, 78, 78, 82, 80, 78, 77, ...  // P波
    40, 37, 29, 24, 24, 34, 49, 66, 90,120,161,200, ...  // QRS复合波
    225,235,223,184,128, 75, 43, 32, 34, 42, 49, 49, ...  // S波
    // ... T波, U波
};
```

**ECG参数配置**：

| 参数 | 值 | 说明 |
|------|-----|------|
| 播放频率 | 50Hz | 相位累加器控制 |
| ADC采样率 | 2500Hz | TIMER3触发 |
| 数据流率 | 2500Hz | stream_divisor=20 |
| 心率 | ~50 BPM | 实际MIT-BIH记录 |

**数据流协议**：
```
WAVEFORM:1,2500,<PA6值>|<PB1值>
示例: WAVEFORM:1,2500,288|12
      WAVEFORM:1,2500,175|0
      WAVEFORM:1,2500,241|5
```

---

## 🧮 信号处理算法

### DDS (Direct Digital Synthesis)

```c
// 32位相位累加器
static uint32_t phase_accumulator = 0;
static uint32_t phase_increment = 0;

// 频率设置: phase_increment = freq × 2³² / sample_rate
// 对于50kHz采样率: phase_increment = freq × 85899
void DDS_SetFrequency(uint32_t freq_hz) {
    phase_increment = freq_hz * 85899UL;
}

// 获取样本（50kHz中断调用）
uint8_t DDS_GetSample(void) {
    uint8_t index = (phase_accumulator >> 24) & 0xFF;  // 高8位作为索引
    uint8_t sample = sine_table[index];                 // 查表
    phase_accumulator += phase_increment;               // 相位累加
    return sample;
}
```

### DFT单频点提取

```c
// 提取特定频率的幅度和相位
void DFT_SingleFrequency(int16_t *signal, uint32_t N, float freq, 
                         float sample_rate, float *amplitude, float *phase) {
    float omega = 2.0f * PI * freq / sample_rate;
    float sin_sum = 0, cos_sum = 0;
    
    for (uint32_t i = 0; i < N; i++) {
        sin_sum += signal[i] * sinf(omega * i);
        cos_sum += signal[i] * cosf(omega * i);
    }
    
    *amplitude = sqrtf(sin_sum * sin_sum + cos_sum * cos_sum) * 2.0f / N;
    *phase = atan2f(sin_sum, cos_sum) * 180.0f / PI;
}
```

### 相位展开 (Unwrapping)

```c
// 消除±180°相位跳变，获得连续相位曲线
float unwrap_phase(float current_phase, float *phase_offset, float *prev_phase) {
    float diff = current_phase - *prev_phase;
    
    if (diff > 180.0f)
        *phase_offset -= 360.0f;
    else if (diff < -180.0f)
        *phase_offset += 360.0f;
    
    *prev_phase = current_phase;
    return current_phase + *phase_offset;
}
```

---

## 🚀 快速开始

### 环境要求

| 类别 | 要求 | 说明 |
|------|------|------|
| **MCU** | GD32F103RCT6 | 或兼容的STM32F103 |
| **DAC** | DAC5311 | TI 8位SPI DAC |
| **编译器** | Keil MDK v5.x | ARM Compiler 5/6 |
| **Node.js** | v16+ | npm包管理 |
| **浏览器** | Chrome/Edge | Web Serial API支持 |

### 获取代码

```bash
git clone https://github.com/Furinaaa-Cancan/Frequency-Analyzer.git
cd Frequency-Analyzer
```

### 固件编译与烧录

```bash
# 1. 使用Keil打开工程
firmware/Tamlate.uvprojx

# 2. 编译
Project → Build Target (F7)

# 3. 烧录（需要调试器，如ST-Link/J-Link）
Flash → Download (F8)

# 4. 复位运行
```

### 启动Web界面

```bash
cd react-bode-analyzer

# 安装依赖
npm install

# 开发模式启动
npm run dev

# 浏览器访问
# http://localhost:5173
```

### 快速测试

```bash
# 串口调试工具（115200bps）发送命令：

HELP                    # 查看帮助
TYPE:SINE               # 正弦波模式
FREQ:100                # 设置100Hz
START                   # 开始数据流
MEASURE                 # 单点测量
SWEEP                   # 完整扫频

TYPE:ECG                # 切换ECG模式
```

---

## 📡 API参考

### 串口命令列表

| 命令 | 参数 | 功能 | 响应 |
|------|------|------|------|
| `HELP` | - | 显示帮助信息 | 命令列表 |
| `DEBUG` | - | 系统诊断信息 | 硬件状态 |
| `STATUS` | - | 查询当前状态 | 频率/模式 |
| `TYPE:SINE` | - | 切换正弦波模式 | `OK:TYPE:SINE` |
| `TYPE:ECG` | - | 切换ECG模式 | `OK:TYPE:ECG` |
| `FREQ:xxx` | 10-1000 | 设置频率(Hz) | `OK:FREQ:xxxHz` |
| `START` | - | 启动数据流 | `OK:STREAM_STARTED` |
| `STOP` | - | 停止数据流 | `OK:STREAM_STOPPED` |
| `MEASURE` | - | 单点频率测量 | 幅度/相位数据 |
| `SWEEP` | - | 完整扫频(10-1000Hz) | 100点数据 |
| `SWEEP:xxx` | 最大频率 | 自定义扫频范围 | N点数据 |
| `CALIBRATE` | - | 系统校准 | 校准结果 |

### LED控制命令

| 命令 | 参数 | 功能 |
|------|------|------|
| `LED:x` | 0-6 | LED模式(0=关,1=流水,2=呼吸,3=闪烁,4=指定,5=旋转,6=警报) |
| `LED_BRIGHT:x` | 0-100 | 亮度百分比 |
| `LED_FREQ:x` | 1-30000 | 动画间隔(ms) |
| `LED_MASK:x` | 0-31 | LED掩码(bit0-4对应5个LED) |

### 数据协议格式

**波形数据流**：
```
WAVEFORM:<freq>,<sample_rate>,<ch0>|<ch1>\r\n

字段说明：
- freq: 信号频率（正弦波为实际频率，ECG为1）
- sample_rate: 数据流率(Hz)
- ch0: PA6 ADC值（12-bit，0-4095）
- ch1: PB1 ADC值（12-bit，0-4095）

示例：
WAVEFORM:100,500,2048|1856      # 正弦波100Hz
WAVEFORM:1,2500,288|12          # ECG模式
```

**频率响应数据**：
```
FREQ_RESP:<freq>,<input_amp>,<output_amp>,<gain>,<phase>\r\n

字段说明：
- freq: 测试频率(Hz)
- input_amp: 输入信号RMS幅度
- output_amp: 输出信号RMS幅度
- gain: 增益 H(ω) = output_amp / input_amp
- phase: 相位差θ(ω)（度）

示例：
FREQ_RESP:100,1.234,1.156,0.938,-5.23
```

---

## 📁 项目结构

```
Frequency-Analyzer/
├── 📂 firmware/                      # GD32 嵌入式固件 (C语言)
│   ├── 📂 BSP/                       # 板级支持包 (Board Support Package)
│   │   ├── ADC/                      # 双通道ADC驱动 (PA6+PB1同步采样)
│   │   ├── DAC5311/                  # TI DAC5311 SPI驱动
│   │   ├── DDS/                      # DDS信号生成 (32位相位累加器)
│   │   ├── DMA/                      # DMA循环传输配置 (512点缓冲)
│   │   ├── LED/                      # LED指示灯驱动 (PWM调光)
│   │   ├── SINE/                     # 256点正弦查找表
│   │   ├── TIMER/                    # 定时器配置 (TIMER2/3)
│   │   └── USART/                    # 串口通信与命令解析
│   ├── 📂 LIB/                       # GD32F10x 标准外设库
│   │   ├── CMSIS/                    # ARM CMSIS核心
│   │   ├── inc/                      # 库头文件
│   │   └── src/                      # 库源文件
│   ├── 📂 USER/                      # 用户应用层
│   │   ├── main.c                    # 系统初始化与主循环
│   │   ├── main.h                    # 全局类型定义 (SignalType_t)
│   │   ├── measurement.c             # 频率响应测量算法
│   │   ├── adc_handler.c             # ADC数据处理
│   │   └── signal_processing.c       # DFT/RMS信号处理
│   ├── .clang-format                 # 代码格式化配置
│   └── Tamlate.uvprojx               # Keil MDK工程文件
│
├── 📂 react-bode-analyzer/           # React 18 Web应用
│   ├── 📂 src/
│   │   ├── 📂 components/            # React组件
│   │   │   ├── BodePlot.jsx          # 伯德图 (幅频+相频)
│   │   │   ├── WaveformDisplay.jsx   # 实时波形显示
│   │   │   ├── WaveformCapture.jsx   # 欠采样波形捕获
│   │   │   ├── SerialTerminal.jsx    # 串口调试终端
│   │   │   └── FrequencyControl.jsx  # 频率控制面板
│   │   ├── 📂 hooks/                 # 自定义Hooks
│   │   │   ├── useSerial.js          # Web Serial API封装
│   │   │   └── useDataPoints.js      # 数据解析与状态管理
│   │   ├── 📂 styles/                # CSS样式文件
│   │   ├── App.jsx                   # 应用主组件
│   │   └── main.jsx                  # 入口文件
│   ├── package.json                  # 依赖配置
│   └── vite.config.js                # Vite构建配置
│
├── 📂 docs/                          # 项目文档
│   ├── 📂 architecture/              # 系统架构设计文档
│   ├── 📂 development/               # 开发技术说明
│   └── 📂 guides/                    # 用户操作指南
│
├── .gitignore                        # Git忽略规则
├── CONTRIBUTING.md                   # 贡献指南
├── LICENSE                           # MIT许可证
└── README.md                         # 本文档
```

---

## 📊 技术规格

### 硬件参数

| 参数 | 规格 | 说明 |
|------|------|------|
| **MCU** | GD32F103RCT6 | ARM Cortex-M3 @ 72MHz, 256KB Flash, 48KB SRAM |
| **DAC** | TI DAC5311 | 8-bit, SPI接口, 0-3.3V输出 |
| **ADC** | 内置12-bit | 双通道同步采样, 规则并行模式 |
| **运放** | 三级放大链路 | 带通滤波, 1.5μF耦合电容 |

### 信号参数

| 参数 | 正弦波模式 | ECG模式 |
|------|-----------|---------|
| **信号源** | DDS合成 | MIT-BIH数据库 |
| **频率范围** | 10Hz - 1kHz | 50Hz播放 |
| **波形点数** | 256点/周期 | 360点/心跳 |
| **DDS更新率** | 50kHz | 50kHz |
| **ADC采样率** | freq×10 (自适应) | 2500Hz |
| **数据流率** | ~500Hz | 2500Hz |

### 测量精度

| 参数 | 典型值 | 条件 |
|------|--------|------|
| **频率精度** | ±0.01% | DDS相位累加器32位 |
| **相位精度** | ±1° | DFT单频点提取 |
| **幅度精度** | ±0.1dB | RMS能量法 |
| **THD** | <1% | DAC+滤波器 |

### 系统参数

| 参数 | 数值 |
|------|------|
| **串口波特率** | 115200 bps |
| **DMA缓冲区** | 512点 × 32bit |
| **扫频点数** | 100点 (10Hz步进) |
| **完整扫频时间** | ~2分钟 |

---

## 🛠️ 开发指南

### 固件开发

```bash
# 开发环境
- Keil MDK-ARM v5.x
- ARM Compiler 5 或 6
- ST-Link / J-Link 调试器

# 编译选项
- 优化等级: -O2
- C标准: C99
- 警告等级: 全部警告
```

### Web前端开发

```bash
cd react-bode-analyzer

# 开发模式 (热重载)
npm run dev

# 生产构建
npm run build

# 本地预览
npm run preview

# 代码检查
npm run lint
```

### 代码规范

- **固件**: 遵循 `.clang-format` 配置
- **前端**: ESLint + Prettier
- **命名**: 驼峰命名法 (JS), 下划线命名法 (C)
- **注释**: Doxygen格式 (C), JSDoc格式 (JS)

---

## 🤝 参与贡献

欢迎贡献代码！请参阅 [CONTRIBUTING.md](CONTRIBUTING.md)

```bash
# 1. Fork仓库
# 2. 创建功能分支
git checkout -b feature/your-feature

# 3. 提交更改
git commit -m "feat: 添加新功能"

# 4. 推送分支
git push origin feature/your-feature

# 5. 发起Pull Request
```

### Commit规范

- `feat`: 新功能
- `fix`: 修复Bug
- `docs`: 文档更新
- `refactor`: 代码重构
- `test`: 测试相关
- `chore`: 构建/工具

---

## 📄 许可证

本项目采用 **MIT License** 开源许可证。

```
MIT License

Copyright (c) 2025 Frequency Analyzer Project

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software...
```

详见 [LICENSE](LICENSE) 文件。

---

## 🙏 致谢

| 项目/组织 | 贡献 |
|----------|------|
| [GigaDevice](https://www.gd32mcu.com/) | GD32F103 MCU及技术文档 |
| [PhysioNet](https://physionet.org/) | MIT-BIH心电数据库 |
| [React](https://react.dev/) | 前端UI框架 |
| [Chart.js](https://www.chartjs.org/) | 图表可视化库 |
| [Vite](https://vitejs.dev/) | 前端构建工具 |
| [Web Serial API](https://developer.chrome.com/docs/capabilities/serial) | 浏览器串口通信 |

---

<p align="center">
  <img src="https://img.shields.io/badge/Made_with-❤️-red?style=flat-square" alt="Made with Love"/>
  <img src="https://img.shields.io/badge/For-Embedded_Engineers-blue?style=flat-square" alt="For Engineers"/>
</p>

<p align="center">
  <sub>🔬 专业级嵌入式测量仪器 | 开源 · 免费 · 可扩展</sub>
</p>

<p align="center">
  <a href="#-目录">⬆️ 返回顶部</a>
</p>
