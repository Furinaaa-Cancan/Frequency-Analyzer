import { useState, useEffect, useRef } from 'react'
import { Line } from 'react-chartjs-2'
import '../styles/LiveSineWaveMonitor.css'

/**
 * 实时正弦波监视器
 * 输入频率后实时显示DAC输出的正弦波
 */
function LiveSineWaveMonitor({ isConnected, serialPort, latestWaveform }) {
  const [targetFreq, setTargetFreq] = useState(500)
  const [isMonitoring, setIsMonitoring] = useState(false)
  const [currentFreq, setCurrentFreq] = useState(0)
  const [waveformData, setWaveformData] = useState(null)
  const [refreshRate, setRefreshRate] = useState(50) // 刷新间隔(ms) - 更快的实时效果
  
  const timerRef = useRef(null)
  const chartRef = useRef(null)
  const monitoringRef = useRef(false)  // 用于连续模式
  
  // 实时监视：保存接收时间戳
  const lastUpdateTimeRef = useRef(0)
  
  // 监听最新波形数据 - 连续模式：收到数据后立即请求下一次
  useEffect(() => {
    if (isMonitoring && latestWaveform && latestWaveform.input && latestWaveform.input.length > 0) {
      const now = Date.now()
      const timeSinceLastUpdate = now - lastUpdateTimeRef.current
      
      // 确保是新数据（避免重复使用旧数据）
      if (timeSinceLastUpdate > 5) {
        // 直接使用最新数据
        setWaveformData(latestWaveform)
        lastUpdateTimeRef.current = now
        
        // 🚀 连续模式：立即请求下一帧
        if (monitoringRef.current) {
          setTimeout(() => {
            if (monitoringRef.current) {
              requestWave()
            }
          }, refreshRate)
        }
      }
    }
  }, [latestWaveform, isMonitoring, refreshRate])

  // 发送频率设置命令
  const setFrequency = async (freq) => {
    if (!serialPort || !isConnected) {
      console.log('串口未连接')
      return false
    }
    
    try {
      const command = `FREQ:${freq}\r\n`  // 统一使用\r\n格式
      const writer = serialPort.writable.getWriter()
      await writer.write(new TextEncoder().encode(command))
      writer.releaseLock()
      console.log(`✅ 发送FREQ命令: ${freq}Hz，等待信号稳定...`)
      
      // ⚠️ 关键：等待信号稳定（频率改变后需要时间稳定）
      // 至少等待3个周期：3000ms / freq
      const settleTime = Math.max(300, Math.ceil(3000 / freq))
      await new Promise(resolve => setTimeout(resolve, settleTime))
      console.log(`✅ 信号已稳定（等待${settleTime}ms）`)
      
      return true
    } catch (error) {
      console.error('发送频率命令失败:', error)
      return false
    }
  }

  // 请求快速波形数据
  const requestWave = async () => {
    if (!serialPort || !isConnected) return false
    
    try {
      const command = 'WAVE\r\n'  // 使用快速WAVE命令
      const writer = serialPort.writable.getWriter()
      await writer.write(new TextEncoder().encode(command))
      writer.releaseLock()
      return true
    } catch (error) {
      console.error('发送波形命令失败:', error)
      return false
    }
  }
  
  // 兼容旧版本
  const requestMeasure = requestWave

  // 开始监视
  const startMonitoring = async () => {
    if (!isConnected) {
      alert('请先连接串口')
      return
    }

    // 验证输入值
    const freq = parseInt(targetFreq, 10)
    const rate = parseInt(refreshRate, 10)
    
    if (isNaN(freq) || freq < 10 || freq > 10000) {
      alert('请输入有效的频率值 (10-10000 Hz)')
      return
    }
    
    if (isNaN(rate) || rate < 10 || rate > 1000) {
      alert('请输入有效的刷新间隔 (10-1000 ms)')
      return
    }

    // 清空当前显示数据，准备接收新的实时数据
    setWaveformData(null)
    lastUpdateTimeRef.current = 0
    console.log(`[实时监视] 🚀 开始监视 ${freq}Hz，已清空旧数据`)

    // 设置频率（内部会等待信号稳定）
    const success = await setFrequency(freq)
    if (!success) {
      alert('设置频率失败')
      return
    }

    setCurrentFreq(freq)
    setIsMonitoring(true)
    monitoringRef.current = true  // 启用连续模式

    // 立即开始采样（setFrequency已经等待过了）
    console.log(`[实时监视] 📊 开始实时采样，频率=${freq}Hz，间隔=${rate}ms`)
    requestMeasure()  // 首次请求，后续由useEffect连续触发
  }

  // 停止监视
  const stopMonitoring = () => {
    console.log(`[实时监视] ⏹️ 停止监视`)
    monitoringRef.current = false  // 停止连续模式
    setIsMonitoring(false)
    if (timerRef.current) {
      clearInterval(timerRef.current)
      timerRef.current = null
    }
    // 清空当前频率和显示数据，准备下次开始
    setCurrentFreq(0)
    setWaveformData(null)
    lastUpdateTimeRef.current = 0
  }

  // 监视状态变化（调试用）
  useEffect(() => {
    console.log(`[实时监视] 🔄 状态变化: isMonitoring=${isMonitoring}, currentFreq=${currentFreq}`)
  }, [isMonitoring, currentFreq])

  // 组件卸载时清理定时器
  useEffect(() => {
    return () => {
      if (timerRef.current) {
        clearInterval(timerRef.current)
      }
    }
  }, [])

  // 频率输入变化
  const handleFreqChange = (e) => {
    const value = e.target.value
    // 允许空值和数字输入
    if (value === '') {
      setTargetFreq('')
      return
    }
    const numValue = parseInt(value, 10)
    if (!isNaN(numValue) && numValue >= 0) {
      setTargetFreq(numValue)
    }
  }

  // 刷新率变化（连续模式下自动生效）
  const handleRefreshRateChange = (e) => {
    const value = e.target.value
    // 允许空值和数字输入
    if (value === '') {
      setRefreshRate('')
      return
    }
    const numValue = parseInt(value, 10)
    if (!isNaN(numValue) && numValue >= 0) {
      setRefreshRate(numValue)
      // 连续模式下，刷新间隔会在下次请求时自动生效
    }
  }

  // 图表数据准备（全部显示，无需过滤，已在累积时裁剪）
  let filteredInput = []
  let filteredOutput = []
  let filteredTimeStamps = []
  
  if (waveformData && waveformData.timeStamps.length > 0) {
    // 直接使用累积的数据（已经在接收时裁剪到1秒窗口）
    filteredInput = waveformData.input
    filteredOutput = waveformData.output
    filteredTimeStamps = waveformData.timeStamps
  }

  const chartData = waveformData && filteredTimeStamps.length > 0 ? {
    datasets: [
      {
        label: `输入信号 PA6 - ${waveformData.freq}Hz`,
        data: filteredInput.map((val, idx) => ({
          x: filteredTimeStamps[idx],
          y: val
        })),
        borderColor: 'rgb(54, 162, 235)',
        backgroundColor: 'rgba(54, 162, 235, 0.1)',
        borderWidth: 2.5,
        tension: 0.5,  // 增加平滑度
        pointRadius: 0,
        showLine: true,
        cubicInterpolationMode: 'monotone'  // 使用单调插值
      },
      {
        label: `输出信号 PB1 - ${waveformData.freq}Hz`,
        data: filteredOutput.map((val, idx) => ({
          x: filteredTimeStamps[idx],
          y: val
        })),
        borderColor: 'rgb(255, 99, 132)',
        backgroundColor: 'rgba(255, 99, 132, 0.1)',
        borderWidth: 2.5,
        tension: 0.5,  // 增加平滑度
        pointRadius: 0,
        showLine: true,
        cubicInterpolationMode: 'monotone'  // 使用单调插值
      }
    ]
  } : null

  // 计算Y轴范围（自适应）
  let yMin = 0, yMax = 4095
  if (waveformData) {
    const allValues = [...waveformData.input, ...waveformData.output]
    const dataMin = Math.min(...allValues)
    const dataMax = Math.max(...allValues)
    const range = dataMax - dataMin
    const padding = Math.max(range * 0.2, 100) // 20%边距或至少100
    yMin = Math.max(0, Math.floor(dataMin - padding))
    yMax = Math.min(4095, Math.ceil(dataMax + padding))
  }
  
  // 计算X轴范围（显示数据的完整时间范围）
  let xMin = 0, xMax = 100
  if (waveformData && waveformData.timeStamps.length > 0) {
    xMin = waveformData.timeStamps[0]
    xMax = waveformData.timeStamps[waveformData.timeStamps.length - 1]
  }

  const chartOptions = {
    responsive: true,
    maintainAspectRatio: false,
    animation: false,  // ⚡ 关闭动画，实现丝滑无延迟更新
    scales: {
      x: {
        type: 'linear',
        title: {
          display: true,
          text: '时间 (ms)',
          font: { size: 14, weight: 'bold' }
        },
        min: xMin,
        max: xMax,
        ticks: {
          font: { size: 11 }
        }
      },
      y: {
        type: 'linear',
        title: {
          display: true,
          text: 'ADC值',
          font: { size: 14, weight: 'bold' }
        },
        min: yMin,
        max: yMax,
        ticks: {
          font: { size: 11 }
        }
      }
    },
    plugins: {
      legend: {
        display: true,
        position: 'top',
        labels: {
          font: { size: 12 },
          usePointStyle: true
        }
      },
      tooltip: {
        enabled: true,
        mode: 'index',
        intersect: false,
        backgroundColor: 'rgba(0, 0, 0, 0.8)',
        callbacks: {
          label: function(context) {
            return `${context.dataset.label}: ${Math.round(context.parsed.y)}`
          }
        }
      }
    },
    interaction: {
      mode: 'nearest',
      axis: 'x',
      intersect: false
    }
  }

  return (
    <div className="live-sine-monitor">
      <div className="monitor-header">
        <h3>特定频率波形检验</h3>
        <p className="monitor-subtitle">指定目标频率，实时验证DAC输出波形质量 · Single Frequency Verification</p>
        <div className="monitor-method">
          <strong>检验方法：</strong>设置特定频率后，系统以自适应采样率（频率×10）连续采集波形数据。通过定时发送MEASURE命令（默认200ms间隔），获取最新波形快照，实时显示输入输出信号统计特性，用于验证单一频点的输出质量。
        </div>
      </div>

      <div className="monitor-controls">
        <div className="control-group">
          <label>目标频率</label>
          <input 
            type="number" 
            min="10" 
            max="10000"
            step="10"
            value={targetFreq}
            onChange={handleFreqChange}
            disabled={!isConnected || isMonitoring}
            placeholder="10-10000 Hz"
          />
        </div>
        
        <div className="control-group">
          <label>刷新间隔</label>
          <input 
            type="number" 
            min="10" 
            max="1000"
            step="10"
            value={refreshRate}
            onChange={handleRefreshRateChange}
            disabled={!isConnected}
            placeholder="10-1000 ms"
          />
        </div>
        
        <div className="button-group">
          <button 
            className="btn-start" 
            onClick={startMonitoring}
            disabled={!isConnected || isMonitoring}
          >
            开始检验
          </button>
          <button 
            className="btn-stop" 
            onClick={stopMonitoring}
            disabled={!isMonitoring}
          >
            停止检验
          </button>
        </div>
      </div>

      {isMonitoring && (
        <div className="monitoring-status">
          <div className="status-indicator pulsing"></div>
          <span>正在检验 {currentFreq}Hz 频率输出</span>
          <span className="refresh-info">每 {refreshRate}ms 采样刷新</span>
        </div>
      )}

      {waveformData ? (
        <div className="monitor-display">
          <div className="waveform-info">
            <div className="info-card">
              <span className="info-label">当前频率</span>
              <span className="info-value">{waveformData.freq} Hz</span>
            </div>
            <div className="info-card">
              <span className="info-label">采样率</span>
              <span className="info-value">{(waveformData.sampleRate / 1000).toFixed(1)} kHz</span>
            </div>
            <div className="info-card">
              <span className="info-label">数据点数</span>
              <span className="info-value">{filteredInput.length} 点</span>
            </div>
            <div className="info-card">
              <span className="info-label">时间跨度</span>
              <span className="info-value">
                {waveformData.timeStamps.length > 0 
                  ? (waveformData.timeStamps[waveformData.timeStamps.length - 1] - waveformData.timeStamps[0]).toFixed(1) 
                  : '0'} ms
              </span>
            </div>
          </div>

          <div className="waveform-tech-note">
            <strong>技术说明：</strong>波形数据通过DMA+ADC双通道同步采集，时间戳从0开始计算。每次MEASURE命令返回固定点数的最新数据快照，避免累积导致的显示密度问题。图表禁用动画，实现无延迟实时更新。
          </div>

          <div className="chart-container">
            <Line ref={chartRef} data={chartData} options={chartOptions} />
          </div>

          <div className="signal-stats">
            <div className="stats-group input-stats">
              <h4>输入信号统计 (PA6)</h4>
              <div className="stats-values">
                <div className="stat-item">
                  <span className="stat-label">最小值</span>
                  <span className="stat-value">{Math.min(...waveformData.input)}</span>
                </div>
                <div className="stat-item">
                  <span className="stat-label">最大值</span>
                  <span className="stat-value">{Math.max(...waveformData.input)}</span>
                </div>
                <div className="stat-item">
                  <span className="stat-label">峰峰值</span>
                  <span className="stat-value">
                    {Math.max(...waveformData.input) - Math.min(...waveformData.input)}
                  </span>
                </div>
              </div>
            </div>

            <div className="stats-group output-stats">
              <h4>输出信号统计 (PB1)</h4>
              <div className="stats-values">
                <div className="stat-item">
                  <span className="stat-label">最小值</span>
                  <span className="stat-value">{Math.min(...waveformData.output)}</span>
                </div>
                <div className="stat-item">
                  <span className="stat-label">最大值</span>
                  <span className="stat-value">{Math.max(...waveformData.output)}</span>
                </div>
                <div className="stat-item">
                  <span className="stat-label">峰峰值</span>
                  <span className="stat-value">
                    {Math.max(...waveformData.output) - Math.min(...waveformData.output)}
                  </span>
                </div>
              </div>
            </div>
          </div>
        </div>
      ) : (
        <div className="monitor-placeholder">
          <div className="placeholder-icon">〜</div>
          <p>输入目标频率并点击"开始检验"进行波形验证</p>
          <p className="placeholder-hint">
            系统将设置指定频率并实时采样验证输出质量
          </p>
          <div style={{marginTop: '20px', fontSize: '13px', color: '#9d8e7a'}}>
            <p><strong>检验说明：</strong></p>
            <p>• 适用于验证单一频率输出的波形质量</p>
            <p>• 采样率自动设置为目标频率的10倍</p>
            <p>• 实时显示输入/输出信号统计数据</p>
            <p>• 可调节刷新间隔以平衡流畅度与系统负载</p>
          </div>
        </div>
      )}
    </div>
  )
}

export default LiveSineWaveMonitor
