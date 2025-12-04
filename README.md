<p align="center">
  <img src="https://img.shields.io/badge/平台-GD32F103-blue?style=for-the-badge&logo=arm" alt="Platform"/>
  <img src="https://img.shields.io/badge/React-18.0+-61DAFB?style=for-the-badge&logo=react" alt="React"/>
  <img src="https://img.shields.io/badge/许可证-MIT-green?style=for-the-badge" alt="License"/>
  <img src="https://img.shields.io/badge/版本-5.6-orange?style=for-the-badge" alt="Version"/>
</p>

<h1 align="center">
  <br>
  <img src="docs/assets/logo.png" alt="Frequency Analyzer Logo" width="200">
  <br>
  🎛️ 频率响应分析仪
  <br>
</h1>

<h4 align="center">基于 GD32F103 的专业伯德图测量系统</h4>

<p align="center">
  <a href="#-核心功能">核心功能</a> •
  <a href="#-演示效果">演示</a> •
  <a href="#-快速开始">快速开始</a> •
  <a href="#-硬件连接">硬件</a> •
  <a href="#-技术文档">文档</a> •
  <a href="#-许可证">许可证</a>
</p>

<p align="center">
  <img src="docs/assets/demo.gif" alt="演示动画" width="700">
</p>

---

## 🎯 项目简介

**Frequency Analyzer** 是一个嵌入式频率响应分析系统，可实时测量并可视化伯德图。基于 GD32F103 ARM Cortex-M3 微控制器，通过 DDS 算法生成高精度正弦波，采用双通道 ADC 同步采样，计算 10Hz–1kHz 频率范围内的幅频和相频特性。

### 为什么选择这个项目？

| 传统方案 | 本项目 |
|---------|--------|
| 昂贵的实验室设备（数千元） | 低成本 DIY 方案（~￥150） |
| 手动调节频率 | 自动100点扫频 |
| 示波器 + 函数发生器 | 一体化集成系统 |
| 无法导出数据 | 支持 CSV/JSON 导出 |

---

## ✨ 核心功能

<table>
<tr>
<td width="50%">

### 🔬 信号生成
- **DDS算法**：32位相位累加器
- **频率范围**：10Hz – 1000Hz（步进10Hz）
- **频率精度**：< 0.01%
- **信号输出**：DAC5311 纯净正弦波

</td>
<td width="50%">

### 📊 信号分析
- **双通道ADC**：同步采样
- **测量算法**：DFT 相位检测
- **相位精度**：±1°
- **自动Unwrap**：连续相位曲线

</td>
</tr>
<tr>
<td width="50%">

### 🖥️ 现代化界面
- **React 18**：快速响应式UI
- **实时图表**：Chart.js 可视化
- **Web Serial**：免驱动连接
- **主题切换**：深色/浅色模式

</td>
<td width="50%">

### 📈 专业可视化
- **幅频曲线**：H(ω) = K₁/K（dB显示）
- **相频曲线**：θ(ω) 自动Unwrapping
- **波形显示**：输入/输出叠加对比
- **数据导出**：CSV、JSON 格式

</td>
</tr>
</table>

---

## 🎬 演示效果

<p align="center">
  <img src="docs/assets/screenshot-bode.png" alt="伯德图" width="45%">
  <img src="docs/assets/screenshot-waveform.png" alt="波形图" width="45%">
</p>

### 实测数据示例

测量 RC 低通滤波器（R=10kΩ, C=100nF, fc=159Hz）：

| 频率 | 幅频增益 | 相位 | 理论值 |
|------|---------|------|--------|
| 10 Hz | 0.98 | -4° | ~1.0, 0° |
| 159 Hz | 0.71 | -45° | 0.707, -45° |
| 1000 Hz | 0.16 | -81° | ~0, -90° |

---

## 🚀 快速开始

### 环境要求

- **硬件**：GD32F103 开发板、DAC5311 模块
- **软件**：[Keil MDK](https://www.keil.com/mdk5/) v5.x、[Node.js](https://nodejs.org/) v16+
- **浏览器**：Chrome / Edge（支持 Web Serial API）

### 获取代码

```bash
# 克隆仓库
git clone https://github.com/leeyoung7017/Frequency-Analyzer.git
cd Frequency-Analyzer
```

### 烧录固件（3分钟）

```bash
# 打开 Keil 工程
firmware/Tamlate.uvprojx

# 编译并烧录
Build (F7) → Download (F8) → Reset
```

### 启动界面（1分钟）

```bash
cd react-bode-analyzer
npm install
npm run dev

# 浏览器打开
# http://localhost:5173
```

### 开始测量

```
1. 点击"连接串口"按钮
2. 接线：DAC输出 → [被测电路] → ADC输入
3. 发送命令：SWEEP
4. 等待伯德图绘制完成（约2分钟）
```

---

## 🔧 硬件连接

### 系统架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                        GD32F103RCT6                              │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐  │
│  │   DDS    │───▶│   SPI    │───▶│ DAC5311  │───▶│ 运放滤波 │──┼──▶ 输出
│  │  算法    │    │  (PA5/7) │    │   (U3)   │    │          │  │   (PA6)
│  └──────────┘    └──────────┘    └──────────┘    └──────────┘  │
│                                                                  │
│                            ▼ [被测电路 DUT]                      │
│                                                                  │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐  │
│  │  UART    │◀───│   DFT    │◀───│   ADC    │◀───│   输入   │◀─┼── 输入
│  │ (PA9/10) │    │  算法    │    │  0 + 1   │    │  (PB0)   │  │   (PB0)
│  └──────────┘    └──────────┘    └──────────┘    └──────────┘  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
                     ┌─────────────────┐
                     │  React Web 界面 │
                     │   (Chart.js)    │
                     └─────────────────┘
```

### 引脚定义

| 功能 | 引脚 | 说明 |
|------|------|------|
| `SPI_SCK` | PA5 | DAC5311 时钟 |
| `SPI_MOSI` | PA7 | DAC5311 数据 |
| `SPI_CS` | PA4 | DAC5311 片选 |
| `ADC_IN` | PA6 | 输入参考信号 (K) |
| `ADC_OUT` | PB0 | 输出测量信号 (K₁) |
| `UART_TX` | PA9 | 串口发送 |
| `UART_RX` | PA10 | 串口接收 |

### 接线示意图

```
              ┌─────────────────────────────────────┐
              │           GD32F103 开发板            │
              │                                      │
   DAC5311 ◄──┤ PA4/5/7 (SPI)                       │
              │                                      │
              │ PA6 ─────┐                          │
              │          │     ┌──────────┐         │
              │          └────▶│ 被测电路  │────────▶│ PB0
              │                │(RC滤波器) │         │
              │                └──────────┘         │
              │                                      │
    USB ◄─────┤ PA9/10 (UART)                       │
              └─────────────────────────────────────┘
```

### 典型被测电路（RC低通滤波器）

```
PA6 ──[10kΩ]──┬──▶ PB0
              │
           [100nF]
              │
             GND

截止频率: fc = 1/(2πRC) = 159 Hz
```

---

## 📡 串口命令

| 命令 | 功能 | 示例 |
|------|------|------|
| `HELP` | 显示帮助信息 | `HELP` |
| `DEBUG` | 系统诊断 | `DEBUG` |
| `FREQ:xxx` | 设置频率（10-1000Hz） | `FREQ:100` |
| `MEASURE` | 单次测量 | `MEASURE` |
| `SWEEP` | 完整扫频测量 | `SWEEP` |
| `STATUS` | 查询系统状态 | `STATUS` |
| `CALIBRATION` | 运行校准 | `CALIBRATION` |

### 数据协议

```
// 频率响应数据格式
FREQ_RESP:<频率>,<输入幅度>,<输出幅度>,<增益>,<相位>
示例: FREQ_RESP:100,1.234,1.156,0.938,-5.23

// 波形数据格式
WAVEFORM:<频率>,<采样率>,<数据...>
示例: WAVEFORM:100,10000,2048,2145,2234,...
```

---

## 🧮 核心算法

### DDS 信号生成

```c
// 32位相位累加器实现精确频率合成
phase_accumulator += phase_increment;
sample = sine_table[phase_accumulator >> 24];
DAC5311_Write(sample);

// 相位增量计算公式
// phase_increment = (freq × 2^32) / sample_rate
```

### DFT 相位测量

```c
// 单频点DFT精确提取相位
float omega = 2.0f * PI * freq / sample_rate;
for (int i = 0; i < N; i++) {
    sin_sum += signal[i] * sinf(omega * i);
    cos_sum += signal[i] * cosf(omega * i);
}
phase = atan2f(sin_sum, cos_sum) * 180.0f / PI;
```

### 相位 Unwrapping

```c
// 自动修正 ±180° 相位跳变
float phase_diff = current_phase - previous_phase;
if (phase_diff > 180.0f)
    phase_offset -= 360.0f;
else if (phase_diff < -180.0f)
    phase_offset += 360.0f;
unwrapped_phase = current_phase + phase_offset;
```

---

## 📁 项目结构

```
Frequency-Analyzer/
├── 📂 firmware/                    # GD32 嵌入式固件
│   ├── 📂 BSP/                     # 板级支持包
│   │   ├── ADC/                    # 双通道ADC驱动
│   │   ├── DAC5311/                # DAC SPI驱动
│   │   ├── DDS/                    # DDS信号生成
│   │   ├── DMA/                    # DMA配置
│   │   ├── TIMER/                  # 定时器配置
│   │   └── USART/                  # 串口通信
│   ├── 📂 LIB/                     # GD32 标准库
│   ├── 📂 USER/                    # 主程序
│   │   ├── main.c                  # 入口文件
│   │   ├── measurement.c           # 测量逻辑
│   │   └── signal_processing.c     # DSP算法
│   └── Tamlate.uvprojx             # Keil工程文件
│
├── 📂 react-bode-analyzer/         # React Web 界面
│   ├── 📂 src/
│   │   ├── 📂 components/          # React 组件
│   │   │   ├── BodePlot.jsx        # 伯德图组件
│   │   │   ├── WaveformChart.jsx   # 波形显示
│   │   │   ├── SerialTerminal.jsx  # 串口终端
│   │   │   └── ControlPanel.jsx    # 控制面板
│   │   └── 📂 hooks/               # 自定义 Hooks
│   │       └── useSerial.js        # Web Serial Hook
│   └── package.json
│
├── 📂 docs/                        # 项目文档
│   ├── 📂 architecture/            # 系统架构文档
│   ├── 📂 guides/                  # 使用指南
│   └── 📂 assets/                  # 图片资源
│
└── README.md                       # 本文件
```

---

## 📊 技术规格

| 参数 | 数值 |
|------|------|
| **MCU** | GD32F103RCT6 @ 72MHz (ARM Cortex-M3) |
| **DAC** | DAC5311（8位，SPI接口） |
| **ADC分辨率** | 12位 |
| **DDS更新率** | 50 kHz |
| **ADC采样率** | 自适应（信号频率 × 10） |
| **频率范围** | 10 Hz – 1000 Hz |
| **频率点数** | 100 点（步进 10 Hz） |
| **每点采样数** | 512 |
| **相位精度** | ±1° |
| **幅度精度** | ±0.1 dB |
| **完整扫频时间** | 约 2 分钟 |
| **串口波特率** | 115200 |

---

## 🛠️ 开发指南

### 编译固件

1. 使用 Keil MDK 打开 `firmware/Tamlate.uvprojx`
2. 选择目标芯片：`GD32F103RCT6`
3. 编译：`F7`
4. 烧录：`F8`

### 构建 Web 界面

```bash
cd react-bode-analyzer

# 开发模式
npm run dev

# 生产构建
npm run build

# 预览生产版本
npm run preview
```

---

## 🤝 参与贡献

欢迎贡献代码！请先阅读 [贡献指南](CONTRIBUTING.md)。

1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/新功能`)
3. 提交更改 (`git commit -m '添加新功能'`)
4. 推送分支 (`git push origin feature/新功能`)
5. 发起 Pull Request

---

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件。

---

## 🙏 致谢

- [GD32 社区](https://www.gd32mcu.com/) - 优秀的技术文档
- [React](https://react.dev/) - 现代化 UI 框架
- [Chart.js](https://www.chartjs.org/) - 强大的图表库
- [Web Serial API](https://developer.chrome.com/docs/capabilities/serial) - 浏览器串口通信

---

## 📬 联系方式

**作者**：leeyoung7017  
**邮箱**：leeyoung7017@163.com  
**实验室**：卓越楼 1302 实验室

---

<p align="center">
  <sub>用 ❤️ 为嵌入式爱好者打造</sub>
</p>

<p align="center">
  <a href="#top">⬆️ 返回顶部</a>
</p>
