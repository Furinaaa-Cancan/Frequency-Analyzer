# 频率响应分析仪 - React版本

基于React + Vite的现代化Bode图分析工具，支持GD32F103 ADC双通道采样数据实时可视化。

## 🚀 快速开始

### 安装依赖

```bash
npm install
```

### 启动开发服务器

```bash
npm run dev
```

应用将自动在浏览器中打开 `http://localhost:3000`

### 构建生产版本

```bash
npm run build
```

构建文件将生成在 `dist` 目录。

### 预览生产版本

```bash
npm run preview
```

## ✨ 功能特性

- ✅ **串口通信**: 基于Web Serial API的串口连接
- ✅ **实时绘图**: 使用Chart.js绘制幅频和相频特性曲线
- ✅ **数据分析**: 自动计算H(ω)和θ(ω)
- ✅ **数据导出**: 支持CSV和PNG图片导出
- ✅ **Apple风格UI**: 精美的macOS风格界面设计
- ✅ **响应式布局**: 适配各种屏幕尺寸

## 🎨 技术栈

- **React 18**: 现代化UI框架
- **Vite**: 极速开发构建工具
- **Chart.js**: 专业图表库
- **Web Serial API**: 浏览器串口通信

## 📊 数据格式

串口接收数据格式：
```
FREQ_RESP:频率,输入幅度K,输出幅度K1,幅频响应H,相位θ
示例: FREQ_RESP:100,1024.50,850.25,0.8300,-35.54
```

## 🌐 浏览器兼容性

需要支持Web Serial API的浏览器：
- Chrome 89+
- Edge 89+
- Opera 75+

## 📁 项目结构

```
react-bode-analyzer/
├── src/
│   ├── components/         # React组件
│   │   ├── TopBar.jsx
│   │   ├── Header.jsx
│   │   ├── ControlPanel.jsx
│   │   ├── StatusBar.jsx
│   │   ├── Charts.jsx
│   │   ├── DataTable.jsx
│   │   └── ...
│   ├── hooks/             # 自定义Hooks
│   │   ├── useSerialPort.js
│   │   ├── useDataPoints.js
│   │   └── useLogs.js
│   ├── styles/            # 样式文件
│   ├── App.jsx            # 主应用组件
│   ├── main.jsx           # 入口文件
│   └── index.css          # 全局样式
├── index.html
├── vite.config.js
└── package.json
```

## 🔧 开发指南

### 自定义配置

修改 `vite.config.js` 可以自定义开发服务器端口等配置。

### 添加新组件

在 `src/components/` 目录下创建新的 `.jsx` 文件，并在相应位置导入使用。

### 修改样式

每个组件都有对应的CSS文件在 `src/styles/` 目录下。

## 📝 使用说明

1. 点击"连接串口"按钮选择串口设备
2. 选择正确的波特率（默认115200）
3. 点击"开始扫描"进行10-1000Hz频率扫描
4. 或使用"单频测试"测试特定频率
5. 实时查看Bode图和数据表格
6. 导出CSV数据或保存图片

## 🎯 课题要求

本项目实现了课题2的所有要求：
- ✅ ADC双通道同步采样
- ✅ 自动测量输入/输出信号幅度
- ✅ 自动计算幅频响应 H(ω) = K₁/K
- ✅ 自动测量相位差 θ(ω)
- ✅ 实时绘制Bode图
- ✅ 全自动测试，无需手动输入

## 📄 License

MIT


