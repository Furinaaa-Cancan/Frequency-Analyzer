import { useEffect, useRef, useState, useMemo } from 'react'
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
} from 'chart.js'
import zoomPlugin from 'chartjs-plugin-zoom'
import { Line } from 'react-chartjs-2'
import CountUp from './CountUp'
import '../styles/WaveformDisplay.css'

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  zoomPlugin
)

function WaveformDisplay({ waveformData, allWaveforms, signalType = 'sine' }) {
  const chartRef = useRef(null)
  // 根据信号类型设置平滑度：ECG需要更高的平滑度
  const [smoothLevel, setSmoothLevel] = useState(signalType === 'ecg' ? 0.8 : 0.5)
  
  // 频率选择状态 - 简化逻辑
  const [isManualSelection, setIsManualSelection] = useState(false)  // 是否手动选择了频率
  const [manualFreq, setManualFreq] = useState(null)  // 手动选择的频率
  
  // 监听信号类型变化，调整平滑度
  useEffect(() => {
    setSmoothLevel(signalType === 'ecg' ? 0.9 : 0.5)  // ECG使用更高的平滑度
  }, [signalType])
  
  // 获取所有频率列表（排序）
  const availableFreqs = useMemo(() => {
    if (!allWaveforms || allWaveforms.size === 0) return []
    return Array.from(allWaveforms.keys()).sort((a, b) => a - b)
  }, [allWaveforms])
  
  // 决定显示哪个频率的波形
  const displayWaveform = useMemo(() => {
    // 如果用户手动选择了频率，显示选择的频率
    if (isManualSelection && manualFreq && allWaveforms && allWaveforms.has(manualFreq)) {
      return allWaveforms.get(manualFreq)
    }
    // 否则显示最新的波形数据
    return waveformData
  }, [isManualSelection, manualFreq, allWaveforms, waveformData])
  
  // 计算当前滑动条位置
  const currentFreq = displayWaveform?.freq || 0
  const sliderValue = useMemo(() => {
    return availableFreqs.indexOf(currentFreq)
  }, [availableFreqs, currentFreq])
  
  // 处理滑动条变化
  const handleSliderChange = (e) => {
    const value = parseInt(e.target.value)
    if (availableFreqs.length > 0 && value >= 0 && value < availableFreqs.length) {
      setIsManualSelection(true)
      setManualFreq(availableFreqs[value])
    }
  }
  
  // 重置为自动跟随最新数据
  const handleResetToLive = () => {
    setIsManualSelection(false)
    setManualFreq(null)
  }
  
  // 根据信号类型设置标签
  const signalLabels = {
    sine: {
      title: '实时信号波形',
      input: '输入信号 K sin(ωt) - PA6',
      output: '输出信号 K₁ sin(ωt + θ) - PB1',
      inputLabel: '输入信号 (PA6)',
      outputLabel: '输出信号 (PB1)'
    },
    ecg: {
      title: '心电信号波形',
      input: '输入心电信号 - PA6',
      output: '输出心电信号 - PB1',
      inputLabel: '输入ECG (PA6)',
      outputLabel: '输出ECG (PB1)'
    }
  }
  
  const labels = signalLabels[signalType] || signalLabels.sine

  // 如果没有波形数据，显示提示
  if (!displayWaveform || !displayWaveform.input || displayWaveform.input.length === 0) {
    return (
      <div className="waveform-container">
        <div className="waveform-placeholder">
          <h3>{labels.title}</h3>
          <p>等待信号数据...</p>
          <p className="hint">
            {signalType === 'ecg' 
              ? '切换到心电ECG信号模式后，这里将显示输入和输出的心电信号实时波形' 
              : '开始扫描或单频测试后，这里将显示输入和输出信号的实时波形'}
          </p>
        </div>
      </div>
    )
  }

  const { input, output, timeStamps, freq, sampleRate } = displayWaveform

  // 安全检查：防止除零错误
  if (!sampleRate || sampleRate <= 0) {
    return (
      <div className="waveform-display">
        <div className="section-header">
          <h3>实时波形显示</h3>
          <span className="section-subtitle">双通道ADC采样波形（输入/输出）</span>
        </div>
        <div className="waveform-placeholder">
          <p>采样率数据异常</p>
        </div>
      </div>
    )
  }

  // 如果没有时间戳数据，使用旧的索引方式
  const timeData = timeStamps && timeStamps.length === input.length
    ? timeStamps
    : input.map((_, index) => (index / sampleRate) * 1000)
  
  // 计算时间范围
  const minTime = timeData.length > 0 ? timeData[0] : 0
  const maxTime = timeData.length > 0 ? timeData[timeData.length - 1] : 0
  const totalTimeMs = maxTime - minTime

  // 生成数据点对（时间-ADC值）
  const inputPoints = input.map((value, index) => ({
    x: timeData[index],
    y: value
  }))
  
  const outputPoints = output.map((value, index) => ({
    x: timeData[index],
    y: value
  }))
  
  // 计算实际显示的周期数
  const cyclesToShow = (freq * totalTimeMs / 1000).toFixed(1)
  
  // 显示窗口：ECG模式显示1500ms（约2个心跳），正弦波显示10ms
  const displayWindowMs = signalType === 'ecg' ? 1500 : 10
  
  // 使用滚动窗口：显示最近的数据
  const xAxisMax = maxTime > displayWindowMs ? maxTime : displayWindowMs
  const xAxisMin = maxTime > displayWindowMs ? maxTime - displayWindowMs : 0

  // 输入信号数据（使用散点图格式）
  const inputData = {
    datasets: [
      {
        label: signalType === 'ecg' ? labels.input : `${labels.input} - ${freq}Hz`,
        data: inputPoints,
        borderColor: 'rgb(54, 162, 235)',
        backgroundColor: 'rgba(54, 162, 235, 0.1)',
        borderWidth: signalType === 'ecg' ? 1.5 : 2.5,
        tension: smoothLevel,  // 动态平滑度
        pointRadius: 0,
        pointHoverRadius: 5,
        showLine: true,  // 显示连线
        cubicInterpolationMode: signalType === 'ecg' ? 'default' : 'monotone',  // ECG使用默认插值更平滑
        spanGaps: false,  // 不跨越间隙
        fill: false,
        stepped: false,  // 确保不使用阶梯线
        borderCapStyle: 'round',
        borderJoinStyle: 'round',
        segment: {
          borderColor: ctx => {
            // 如果相邻点时间间隔过大，改变颜色或透明度
            const p0 = ctx.p0;
            const p1 = ctx.p1;
            if (p0.parsed && p1.parsed) {
              const timeDiff = Math.abs(p1.parsed.x - p0.parsed.x);
              const normalInterval = 1000 / sampleRate;
              // 如果间隔过大，返回透明（断开）
              return timeDiff > normalInterval * 3 ? 'transparent' : 'rgb(54, 162, 235)';
            }
            return 'rgb(54, 162, 235)';
          }
        }
      }
    ]
  }

  // 输出信号数据（使用散点图格式）
  const outputData = {
    datasets: [
      {
        label: signalType === 'ecg' ? labels.output : `${labels.output} - ${freq}Hz`,
        data: outputPoints,
        borderColor: 'rgb(255, 99, 132)',
        backgroundColor: 'rgba(255, 99, 132, 0.1)',
        borderWidth: signalType === 'ecg' ? 1.5 : 2.5,
        tension: smoothLevel,  // 动态平滑度
        pointRadius: 0,
        pointHoverRadius: 5,
        showLine: true,  // 显示连线
        cubicInterpolationMode: signalType === 'ecg' ? 'default' : 'monotone',  // ECG使用默认插值更平滑
        spanGaps: false,  // 不跨越间隙
        fill: false,
        stepped: false,  // 确保不使用阶梯线
        borderCapStyle: 'round',
        borderJoinStyle: 'round',
        segment: {
          borderColor: ctx => {
            // 如果相邻点时间间隔过大，改变颜色或透明度
            const p0 = ctx.p0;
            const p1 = ctx.p1;
            if (p0.parsed && p1.parsed) {
              const timeDiff = Math.abs(p1.parsed.x - p0.parsed.x);
              const normalInterval = 1000 / sampleRate;
              // 如果间隔过大，返回透明（断开）
              return timeDiff > normalInterval * 3 ? 'transparent' : 'rgb(255, 99, 132)';
            }
            return 'rgb(255, 99, 132)';
          }
        }
      }
    ]
  }

  // 计算输入信号Y轴范围（使用全部数据）
  // 防止空数组导致 Infinity/-Infinity
  const inputMin = input.length > 0 ? Math.min(...input) : 0
  const inputMax = input.length > 0 ? Math.max(...input) : 4095
  const inputRange = inputMax - inputMin
  const inputPadding = Math.max(inputRange * 0.15, 50) // 至少留50的空白，或15%
  const inputYMin = Math.floor(inputMin - inputPadding)
  const inputYMax = Math.ceil(inputMax + inputPadding)

  // 计算输出信号Y轴范围（使用全部数据）
  const outputMin = output.length > 0 ? Math.min(...output) : 0
  const outputMax = output.length > 0 ? Math.max(...output) : 4095
  const outputRange = outputMax - outputMin
  const outputPadding = Math.max(outputRange * 0.15, 50) // 至少留50的空白，或15%
  const outputYMin = Math.floor(outputMin - outputPadding)
  const outputYMax = Math.ceil(outputMax + outputPadding)

  // 输入信号图表配置
  const inputOptions = {
    responsive: true,
    maintainAspectRatio: false,
    animation: false, // 禁用动画，避免缩放卡顿
    devicePixelRatio: window.devicePixelRatio || 1, // 提高像素密度
    elements: {
      line: {
        tension: smoothLevel,
        borderCapStyle: 'round',
        borderJoinStyle: 'round'
      },
      point: {
        radius: 0,
        hitRadius: 5
      }
    },
    scales: {
      x: {
        type: 'linear',
        title: {
          display: true,
          text: '时间 (ms)',
          font: { size: 12, weight: 'bold' }
        },
        min: xAxisMin,  // 滚动窗口的起始时间
        max: xAxisMax,  // 滚动窗口的结束时间
        ticks: { 
          maxTicksLimit: 11,
          font: { size: 10 },
          autoSkip: true
        }
      },
      y: {
        type: 'linear',
        title: {
          display: true,
          text: 'ADC值',
          font: { size: 12, weight: 'bold' }
        },
        min: inputYMin,
        max: inputYMax,
        ticks: { 
          font: { size: 10 },
          stepSize: Math.ceil(inputRange / 10) // 固定步长
        },
        grid: {
          color: 'rgba(0, 0, 0, 0.05)',  // 淡化网格线
          drawBorder: false,
          drawTicks: true
        },
        border: {
          display: false
        }
      }
    },
    plugins: {
      decimation: {
        enabled: false, // 禁用抽取，保持所有数据点
      },
      legend: { 
        display: true, 
        position: 'top',
        labels: {
          font: { size: 11 },
          usePointStyle: true,
          padding: 10
        }
      },
      tooltip: {
        enabled: true,
        mode: 'index',
        intersect: false,
        backgroundColor: 'rgba(0, 0, 0, 0.8)',
        callbacks: {
          label: function(context) {
            return `ADC: ${Math.round(context.parsed.y)}`
          }
        }
      },
      zoom: {
        limits: {
          x: {min: minTime, max: maxTime},  // 可以查看所有历史数据
          y: {min: inputYMin, max: inputYMax}
        },
        zoom: {
          wheel: {
            enabled: true,  // 启用滚轮缩放
            speed: 0.1
          },
          pinch: {
            enabled: true  // 启用触摸缩放
          },
          mode: 'x'  // 只缩放X轴
        },
        pan: {
          enabled: true,  // 启用X轴平移（左右拖动查看历史数据）
          mode: 'x',
          modifierKey: null  // 不需要按键，直接拖动
        }
      }
    },
    interaction: {
      mode: 'nearest',
      axis: 'x',
      intersect: false
    }
  }

  // 输出信号图表配置
  const outputOptions = {
    responsive: true,
    maintainAspectRatio: false,
    animation: false, // 禁用动画，避免缩放卡顿
    devicePixelRatio: window.devicePixelRatio || 1, // 提高像素密度
    elements: {
      line: {
        tension: smoothLevel,
        borderCapStyle: 'round',
        borderJoinStyle: 'round'
      },
      point: {
        radius: 0,
        hitRadius: 5
      }
    },
    scales: {
      x: {
        type: 'linear',
        title: {
          display: true,
          text: '时间 (ms)',
          font: { size: 12, weight: 'bold' }
        },
        min: xAxisMin,  // 滚动窗口的起始时间
        max: xAxisMax,  // 滚动窗口的结束时间
        ticks: { 
          maxTicksLimit: 11,
          font: { size: 10 },
          autoSkip: true
        }
      },
      y: {
        type: 'linear',
        title: {
          display: true,
          text: 'ADC值',
          font: { size: 12, weight: 'bold' }
        },
        min: outputYMin,
        max: outputYMax,
        ticks: { 
          font: { size: 10 },
          stepSize: Math.ceil(outputRange / 10) // 固定步长
        }
      }
    },
    plugins: {
      decimation: {
        enabled: false, // 禁用抽取，保持所有数据点
      },
      legend: { 
        display: true, 
        position: 'top',
        labels: {
          font: { size: 11 },
          usePointStyle: true,
          padding: 10
        }
      },
      tooltip: {
        enabled: true,
        mode: 'index',
        intersect: false,
        backgroundColor: 'rgba(0, 0, 0, 0.8)',
        callbacks: {
          label: function(context) {
            return `ADC: ${Math.round(context.parsed.y)}`
          }
        }
      },
      zoom: {
        limits: {
          x: {min: minTime, max: maxTime},  // 可以查看所有历史数据
          y: {min: outputYMin, max: outputYMax}
        },
        zoom: {
          wheel: {
            enabled: true,  // 启用滚轮缩放
            speed: 0.1
          },
          pinch: {
            enabled: true  // 启用触摸缩放
          },
          mode: 'x'  // 只缩放X轴
        },
        pan: {
          enabled: true,  // 启用X轴平移（左右拖动查看历史数据）
          mode: 'x',
          modifierKey: null  // 不需要按键，直接拖动
        }
      }
    },
    interaction: {
      mode: 'nearest',
      axis: 'x',
      intersect: false
    }
  }

  // 计算信号统计信息（使用全部数据）
  const inputMinAll = input.length > 0 ? Math.round(Math.min(...input)) : 0
  const inputMaxAll = input.length > 0 ? Math.round(Math.max(...input)) : 0
  const inputPP = Math.round(inputMaxAll - inputMinAll) // 峰峰值
  
  const outputMinAll = output.length > 0 ? Math.round(Math.min(...output)) : 0
  const outputMaxAll = output.length > 0 ? Math.round(Math.max(...output)) : 0
  const outputPP = Math.round(outputMaxAll - outputMinAll)
  
  // 计算幅值比，保留4位小数（防止除零）
  const amplitudeRatio = inputPP > 0 ? parseFloat((outputPP / inputPP).toFixed(4)) : 0

  return (
    <div className="waveform-container">
      <div className="waveform-header">
        <h3>{labels.title}</h3>
        <div className="waveform-info">
          {signalType === 'sine' && <span className="freq-badge">频率: {freq} Hz</span>}
          <span className="sample-badge">采样率: {sampleRate / 1000} kHz</span>
          <span className="points-badge">缓冲: {totalTimeMs.toFixed(1)}ms ({input.length}点) | 显示窗口: 0-{displayWindowMs}ms</span>
        </div>
        {signalType === 'sine' && availableFreqs.length > 1 && (
          <div className="freq-slider-control">
            <label>查看频率:</label>
            <input
              type="range"
              min="0"
              max={availableFreqs.length - 1}
              value={sliderValue >= 0 ? sliderValue : 0}
              onChange={handleSliderChange}
              className="freq-slider"
            />
            <span className="freq-display">{currentFreq}Hz</span>
            <span className="freq-range">({availableFreqs[0]}Hz - {availableFreqs[availableFreqs.length - 1]}Hz)</span>
            {isManualSelection && (
              <button className="live-btn" onClick={handleResetToLive} title="返回实时跟随">
                📡 实时
              </button>
            )}
          </div>
        )}
        <div className="smooth-control">
          <label>平滑度:</label>
          <button 
            className={`smooth-btn ${smoothLevel === 0 ? 'active' : ''}`}
            onClick={() => setSmoothLevel(0)}
            title="直线连接，显示真实采样点"
          >
            无
          </button>
          <button 
            className={`smooth-btn ${smoothLevel === 0.3 ? 'active' : ''}`}
            onClick={() => setSmoothLevel(0.3)}
            title="轻微平滑"
          >
            低
          </button>
          <button 
            className={`smooth-btn ${smoothLevel === 0.5 ? 'active' : ''}`}
            onClick={() => setSmoothLevel(0.5)}
            title="推荐平滑度"
          >
            中
          </button>
          <button 
            className={`smooth-btn ${smoothLevel === 0.7 ? 'active' : ''}`}
            onClick={() => setSmoothLevel(0.7)}
            title="高度平滑"
          >
            高
          </button>
          <button 
            className={`smooth-btn ${smoothLevel === 0.9 ? 'active' : ''}`}
            onClick={() => setSmoothLevel(0.9)}
            title="超高平滑，适合低频信号"
          >
            超高
          </button>
        </div>
      </div>
      
      <div className="waveform-stats">
        <div className="stat-card input-stat">
          <div className="stat-label">{labels.inputLabel}</div>
          <div className="stat-values">
            <span>峰峰值: <CountUp to={inputPP} duration={0.6} /></span>
            <span>最大: <CountUp to={inputMaxAll} duration={0.6} /></span>
            <span>最小: <CountUp to={inputMinAll} duration={0.6} /></span>
          </div>
        </div>
        <div className="stat-card output-stat">
          <div className="stat-label">{labels.outputLabel}</div>
          <div className="stat-values">
            <span>峰峰值: <CountUp to={outputPP} duration={0.6} /></span>
            <span>最大: <CountUp to={outputMaxAll} duration={0.6} /></span>
            <span>最小: <CountUp to={outputMinAll} duration={0.6} /></span>
          </div>
        </div>
        <div className="stat-card ratio-stat">
          <div className="stat-label">幅值比</div>
          <div className="stat-values">
            <span className="ratio-value">H = <CountUp to={amplitudeRatio} duration={0.6} /></span>
          </div>
        </div>
      </div>

      <div className="waveform-charts-grid">
        <div className="waveform-chart-item">
          <h4 className="chart-title">{labels.input}</h4>
          <div className="waveform-chart">
            <Line data={inputData} options={inputOptions} />
          </div>
        </div>
        
        <div className="waveform-chart-item">
          <h4 className="chart-title">{labels.output}</h4>
          <div className="waveform-chart">
            <Line data={outputData} options={outputOptions} />
          </div>
        </div>
      </div>

      <div className="waveform-hint">
        提示：固定显示0-{displayWindowMs}ms窗口，新数据在后台累积。可以鼠标拖动查看后续波形，滚轮缩放时间轴。建议在低频信号下使用"超高"平滑度以获得更流畅的波形显示。
      </div>
    </div>
  )
}

export default WaveformDisplay

