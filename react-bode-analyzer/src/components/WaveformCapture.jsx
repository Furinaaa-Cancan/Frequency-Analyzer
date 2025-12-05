import { useState, useEffect, useRef } from 'react'
import { Line } from 'react-chartjs-2'
import '../styles/WaveformCapture.css'

/**
 * 欠采样波形查看器 - 实时流畅动画版
 * 用高采样率快速采集，前端模拟欠采样效果
 */
function WaveformCapture({ isConnected, sendCommand, addLog }) {
  const [signalFreq, setSignalFreq] = useState(1000)
  const [sampleRate, setSampleRate] = useState(700)  // 用户期望的欠采样率
  const [isRunning, setIsRunning] = useState(false)
  const [waveData, setWaveData] = useState(null)
  const [frameCount, setFrameCount] = useState(0)
  
  const runningRef = useRef(false)
  const signalFreqRef = useRef(signalFreq)
  const sampleRateRef = useRef(sampleRate)
  const sendCommandRef = useRef(sendCommand)
  
  // 保持ref同步
  useEffect(() => { signalFreqRef.current = signalFreq }, [signalFreq])
  useEffect(() => { sampleRateRef.current = sampleRate }, [sampleRate])
  useEffect(() => { sendCommandRef.current = sendCommand }, [sendCommand])
  
  // 计算采样倍数
  const sampleRatio = sampleRate / signalFreq
  const isUndersampling = sampleRatio < 2
  
  // 计算混叠频率
  const calculateAliasFreq = () => {
    if (sampleRate >= signalFreq * 2) return null
    const k = Math.round(signalFreq / sampleRate)
    return Math.abs(signalFreq - k * sampleRate)
  }
  const aliasFreq = calculateAliasFreq()
  
  // 预设采样率选项
  const presetRates = [
    { label: '0.5×', factor: 0.5, warn: true },   // 直线（整数分数）
    { label: '0.7×', factor: 0.7, warn: true },   // 混叠300Hz
    { label: '0.9×', factor: 0.9, warn: true },   // 混叠100Hz
    { label: '2×', factor: 2, warn: false },      // 奈奎斯特临界
    { label: '10×', factor: 10, warn: false },    // 正常采样
  ]
  
  // 开始/停止实时采集
  const toggleRunning = () => {
    if (!isConnected) {
      addLog('请先连接串口', 'warning')
      return
    }
    
    if (isRunning) {
      runningRef.current = false
      setIsRunning(false)
      addLog('停止实时演示', 'info')
    } else {
      runningRef.current = true
      setIsRunning(true)
      setFrameCount(0)
      addLog(`开始实时演示: ${signalFreq}Hz信号, ${sampleRate}Hz采样`, 'info')
      requestNextFrame()
    }
  }
  
  // 请求下一帧（使用ref确保获取最新值）
  const requestNextFrame = () => {
    if (!runningRef.current) return
    const cmd = `UWAVE:${signalFreqRef.current},${sampleRateRef.current}`
    if (sendCommandRef.current) {
      sendCommandRef.current(cmd, () => {})
    }
  }
  
  // 监听UWAVE数据 - 真实欠采样数据（双通道）
  useEffect(() => {
    const handleUWave = (event) => {
      const { signalFreq: freq, sampleRate: sr, ch0, ch1 } = event.detail
      
      // 直接使用真实欠采样数据
      setWaveData({
        signalFreq: freq,
        sampleRate: sr,
        ch0: ch0,  // PA6
        ch1: ch1,  // PB1
        pointCount: ch0.length
      })
      setFrameCount(prev => prev + 1)
      
      // 立即请求下一帧（使用ref中的函数）
      if (runningRef.current) {
        setTimeout(() => {
          if (runningRef.current && sendCommandRef.current) {
            const cmd = `UWAVE:${signalFreqRef.current},${sampleRateRef.current}`
            sendCommandRef.current(cmd, () => {})
          }
        }, 10)
      }
    }
    
    window.addEventListener('uwave-data', handleUWave)
    return () => window.removeEventListener('uwave-data', handleUWave)
  }, [])
  
  // 图表配置
  const chartOptions = {
    responsive: true,
    maintainAspectRatio: false,
    animation: false,
    plugins: {
      legend: { 
        position: 'top',
        labels: { font: { family: 'Georgia', size: 12 }, color: '#4a3f35' }
      }
    },
    scales: {
      x: {
        title: { display: true, text: '时间 (ms)', font: { family: 'Georgia' }, color: '#7a6f5d' },
        ticks: { maxTicksLimit: 10, color: '#7a6f5d' },
        grid: { color: 'rgba(217, 208, 193, 0.5)' }
      },
      y: {
        title: { display: true, text: 'ADC值', font: { family: 'Georgia' }, color: '#7a6f5d' },
        min: 0, max: 4096,
        ticks: { color: '#7a6f5d' },
        grid: { color: 'rgba(217, 208, 193, 0.5)' }
      }
    }
  }
  
  // 计算时间轴（毫秒）
  const getTimeLabels = () => {
    if (!waveData) return []
    const dt = 1000 / waveData.sampleRate  // 每个采样点的时间间隔(ms)
    return Array.from({ length: waveData.ch0.length }, (_, i) => (i * dt).toFixed(1))
  }
  
  // 图表数据 - 显示双通道欠采样波形
  const chartData = waveData ? {
    labels: getTimeLabels(),
    datasets: [
      {
        label: 'PA6 (输入)',
        data: waveData.ch0,
        borderColor: '#b38b5e',
        backgroundColor: 'rgba(179, 139, 94, 0.1)',
        borderWidth: 2,
        pointRadius: waveData.ch0.length < 50 ? 3 : 0,
        pointBackgroundColor: '#b38b5e',
        tension: 0
      },
      {
        label: 'PB1 (输出)',
        data: waveData.ch1,
        borderColor: '#c8503c',
        backgroundColor: 'rgba(200, 80, 60, 0.1)',
        borderWidth: 2,
        pointRadius: waveData.ch1.length < 50 ? 3 : 0,
        pointBackgroundColor: '#c8503c',
        tension: 0
      }
    ]
  } : null

  return (
    <div className="waveform-capture">
      <div className="section-header">
        <h3>欠采样与混叠演示</h3>
        <span className="section-subtitle">奈奎斯特定理：采样率＜2倍信号频率时产生混叠，看到"假"的低频波形</span>
      </div>
      
      {/* 控制区域 */}
      <div className="capture-controls">
        <div className="control-item">
          <label>信号频率</label>
          <div className="input-with-unit">
            <input
              type="number"
              value={signalFreq}
              onChange={(e) => setSignalFreq(parseInt(e.target.value) || 0)}
            />
            <span className="unit">Hz</span>
          </div>
        </div>
        
        <div className="control-item">
          <label>采样率</label>
          <div className="input-with-unit">
            <input
              type="number"
              value={sampleRate}
              onChange={(e) => setSampleRate(parseInt(e.target.value) || 0)}
            />
            <span className="unit">Hz</span>
          </div>
        </div>
        
        <div className="control-item">
          <label>采样状态</label>
          <div className={`ratio-display ${isUndersampling ? 'warning' : 'ok'}`}>
            {sampleRatio.toFixed(2)}× {isUndersampling ? (aliasFreq !== null ? `→混叠${aliasFreq}Hz` : '(直线)') : '✓正常'}
          </div>
        </div>
        
        <div className="control-item presets">
          <label>快速设置</label>
          <div className="preset-btns">
            {presetRates.map((p) => (
              <button
                key={p.label}
                className={p.warn ? 'warn' : ''}
                onClick={() => setSampleRate(Math.round(signalFreq * p.factor))}
              >
                {p.label}
              </button>
            ))}
          </div>
        </div>
        
        <div className="control-item action">
          <button 
            className={`capture-btn ${isRunning ? 'active' : ''}`}
            onClick={toggleRunning}
            disabled={!isConnected}
          >
            {isRunning ? '⏹ 停止' : '▶ 开始演示'}
          </button>
        </div>
      </div>
      
      {/* 波形显示区域 */}
      <div className="waveform-display">
        <div className="wave-info">
          <span>信号: {signalFreq}Hz</span>
          <span>采样: {sampleRate}Hz</span>
          <span>倍数: {sampleRatio.toFixed(2)}×</span>
          {aliasFreq !== null && <span className="alias-freq">混叠: {aliasFreq}Hz</span>}
          {waveData && <span>点数: {waveData.pointCount}</span>}
          {isRunning && <span className="frame-count">帧: {frameCount}</span>}
        </div>
        <div className="wave-chart">
          {waveData ? (
            <Line data={chartData} options={chartOptions} />
          ) : (
            <div className="wave-placeholder">
              <p>点击"▶ 开始演示"查看实时欠采样效果</p>
              <p className="hint">调整采样率观察混叠现象</p>
            </div>
          )}
        </div>
      </div>
    </div>
  )
}

export default WaveformCapture
