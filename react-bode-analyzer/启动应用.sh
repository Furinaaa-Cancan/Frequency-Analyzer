#!/bin/bash

echo "======================================"
echo "  频率响应分析仪 - React版本启动脚本"
echo "======================================"
echo ""

cd "$(dirname "$0")"

# 检查node是否安装
if ! command -v node &> /dev/null; then
    echo "❌ 错误: 未安装Node.js"
    echo "请先安装Node.js: https://nodejs.org/"
    exit 1
fi

# 检查是否已安装依赖
if [ ! -d "node_modules" ]; then
    echo "📦 首次运行，正在安装依赖..."
    npm install
    echo ""
fi

echo "🚀 启动开发服务器..."
echo "应用将在 http://localhost:5173 打开"
echo ""
echo "按 Ctrl+C 停止服务器"
echo ""

npm run dev


