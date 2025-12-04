import { useState } from 'react'
import '../styles/LEDControl.css'

function LEDControl({ isConnected, sendCommand, addLog }) {
  const [ledMode, setLedMode] = useState(0)
  const [brightness, setBrightness] = useState(100)
  const [interval, setInterval] = useState(1000)
  const [ledMask, setLedMask] = useState(0x15)  // 默认值0x15 = LED3+LED4+LED5

  const LED_MODES = [
    { id: 0, name: 'OFF', desc: '关闭所有LED' },
    { id: 1, name: 'Flow', desc: '流水灯效果' },
    { id: 2, name: 'Breath', desc: '呼吸灯渐变' },
    { id: 3, name: 'Blink', desc: '全部闪烁' },
    { id: 4, name: 'Specific', desc: '特定LED闪烁' },
    { id: 5, name: 'Rotate', desc: '单个轮流' },
    { id: 6, name: 'Alarm', desc: '报警模式(2Hz快闪)' }
  ]

  const BRIGHTNESS_PRESETS = [25, 50, 75, 100]
  const INTERVAL_PRESETS = [
    { value: 100, label: '100ms' },
    { value: 500, label: '500ms' },
    { value: 1000, label: '1s' },
    { value: 2000, label: '2s' },
    { value: 5000, label: '5s' },
    { value: 10000, label: '10s' }
  ]
  
  // LED选择定义 (bit0=LED3, bit1=LED7, bit2=LED4, bit3=LED6, bit4=LED5)
  const LEDS = [
    { name: 'LED3', bit: 0 },
    { name: 'LED7', bit: 1 },
    { name: 'LED4', bit: 2 },
    { name: 'LED6', bit: 3 },
    { name: 'LED5', bit: 4 }
  ]

  const handleModeChange = (mode) => {
    setLedMode(mode)
    sendCommand(`LED:${mode}`, addLog)
    const modeName = LED_MODES[mode].name
    addLog(`LED模式: ${modeName}`, 'info')
    
    // 如果切换到Specific模式，同时发送当前掩码
    if(mode === 4) {
      setTimeout(() => applyLEDMask(ledMask), 100)
    }
  }

  const handleBrightnessChange = (value) => {
    setBrightness(value)
  }

  const applyBrightness = (value = brightness) => {
    sendCommand(`LED_BRIGHT:${value}`, addLog)
    addLog(`LED亮度: ${value}%`, 'info')
  }

  const handleIntervalChange = (value) => {
    setInterval(value)
  }

  // 对数刻度转换：滑块0-100 -> 实际1-30000ms
  const sliderToInterval = (sliderValue) => {
    // 使用对数映射: 1ms - 30000ms
    // 0 -> 1ms, 100 -> 30000ms
    const minLog = Math.log(1)
    const maxLog = Math.log(30000)
    const scale = (maxLog - minLog) / 100
    return Math.round(Math.exp(minLog + scale * sliderValue))
  }

  const intervalToSlider = (intervalValue) => {
    // 反向转换: 1-30000ms -> 0-100
    // 防止 log(0) 或 log(负数) 导致 NaN
    const safeValue = Math.max(1, intervalValue)
    const minLog = Math.log(1)
    const maxLog = Math.log(30000)
    const scale = (maxLog - minLog) / 100
    return Math.round((Math.log(safeValue) - minLog) / scale)
  }

  const handleSliderChange = (sliderValue) => {
    const actualInterval = sliderToInterval(Number(sliderValue))
    setInterval(actualInterval)
  }

  const applyInterval = (value = interval) => {
    console.log('发送LED间隔命令:', value, 'ms')
    sendCommand(`LED_FREQ:${value}`, addLog)
    addLog(`LED间隔: ${value}ms (发送到硬件)`, 'info')
  }

  const applyLEDMask = (mask = ledMask) => {
    console.log('发送LED掩码命令:', `0x${mask.toString(16).toUpperCase()}`)
    sendCommand(`LED_MASK:${mask}`, addLog)
    addLog(`LED掩码: 0x${mask.toString(16).toUpperCase().padStart(2, '0')}`, 'info')
  }

  return (
    <div className="led-control">
      <div className="section-header">
        <h3>课题一：LED控制</h3>
        <span className="section-subtitle">实时控制LED状态指示</span>
      </div>

      {/* LED模式选择 */}
      <div className="led-modes">
        <label className="control-label">LED模式:</label>
        <div className="mode-buttons">
          {LED_MODES.map(mode => (
            <button
              key={mode.id}
              className={`mode-btn ${ledMode === mode.id ? 'active' : ''}`}
              onClick={() => handleModeChange(mode.id)}
              disabled={!isConnected}
              title={mode.desc}
            >
              {mode.name}
            </button>
          ))}
        </div>
      </div>

      {/* 亮度控制 */}
      <div className="led-control-group">
        <label className="control-label">
          亮度: <span className="value-display">{brightness}%</span>
        </label>
        <div className="slider-container">
          <input
            type="range"
            min="0"
            max="100"
            value={brightness}
            onChange={(e) => handleBrightnessChange(Number(e.target.value))}
            onMouseUp={(e) => applyBrightness(Number(e.target.value))}
            onTouchEnd={(e) => applyBrightness(Number(e.target.value))}
            disabled={!isConnected}
            className="slider brightness-slider"
          />
          <div className="preset-buttons">
            {BRIGHTNESS_PRESETS.map(preset => (
              <button
                key={preset}
                className="preset-btn"
                onClick={() => {
                  setBrightness(preset)
                  applyBrightness(preset)
                }}
                disabled={!isConnected}
              >
                {preset}%
              </button>
            ))}
          </div>
        </div>
      </div>

      {/* 间隔控制 */}
      <div className="led-control-group">
        <label className="control-label">
          间隔: <span className="value-display">{interval}ms</span>
          <span className="interval-hint">
            （Flow/Rotate: 切换间隔 | Blink: 亮{interval}ms+灭{interval}ms）
          </span>
        </label>
        <div className="slider-container">
          <input
            type="range"
            min="0"
            max="100"
            value={intervalToSlider(interval)}
            onChange={(e) => handleSliderChange(e.target.value)}
            onMouseUp={(e) => {
              const actualInterval = sliderToInterval(Number(e.target.value))
              applyInterval(actualInterval)
            }}
            onTouchEnd={(e) => {
              const actualInterval = sliderToInterval(Number(e.target.value))
              applyInterval(actualInterval)
            }}
            disabled={!isConnected}
            className="slider interval-slider"
          />
          <div className="slider-labels">
            <span>1ms</span>
            <span>100ms</span>
            <span>1s</span>
            <span>10s</span>
            <span>30s</span>
          </div>
          <div className="preset-buttons">
            {INTERVAL_PRESETS.map(preset => (
              <button
                key={preset.value}
                className="preset-btn"
                onClick={() => {
                  console.log('预设按钮点击:', preset.label, '发送值:', preset.value, 'ms')
                  setInterval(preset.value)
                  applyInterval(preset.value)
                }}
                disabled={!isConnected}
                title={`发送值: ${preset.value}ms
Flow/Rotate模式: 每${preset.value}ms切换一次LED
Blink模式: 亮${preset.value}ms + 灭${preset.value}ms (周期${preset.value*2}ms)`}
              >
                {preset.label}
              </button>
            ))}
          </div>
        </div>
        <div className="interval-input-group">
          <input
            type="number"
            min="1"
            max="30000"
            value={interval}
            onChange={(e) => handleIntervalChange(Number(e.target.value))}
            disabled={!isConnected}
            className="interval-input"
            placeholder="1-30000ms"
          />
          <button
            className="apply-btn"
            onClick={applyInterval}
            disabled={!isConnected}
          >
            应用
          </button>
        </div>
      </div>

      {/* 特定LED选择 (仅在Specific模式下显示) */}
      {ledMode === 4 && (
        <div className="led-control-group">
          <label className="control-label">
            选择LED: <span className="value-display">0x{ledMask.toString(16).toUpperCase().padStart(2, '0')}</span>
          </label>
          <div className="led-selector">
            {LEDS.map(led => (
              <button
                key={led.bit}
                className={`led-select-btn ${(ledMask & (1 << led.bit)) ? 'active' : ''}`}
                onClick={() => {
                  const newMask = ledMask ^ (1 << led.bit)
                  setLedMask(newMask)
                  applyLEDMask(newMask)
                }}
                disabled={!isConnected}
                title={`${led.name} (bit${led.bit})`}
              >
                {led.name}
              </button>
            ))}
          </div>
          <button
            className="apply-btn"
            onClick={() => applyLEDMask()}
            disabled={!isConnected}
            style={{ marginTop: '10px' }}
          >
            应用选择
          </button>
        </div>
      )}

      {/* 快捷功能 */}
      <div className="led-quick-actions">
        <button
          className="quick-btn alarm-btn"
          onClick={() => handleModeChange(6)}
          disabled={!isConnected}
          title="触发报警模式"
        >
          报警模式
        </button>
        <button
          className="quick-btn off-btn"
          onClick={() => handleModeChange(0)}
          disabled={!isConnected}
          title="关闭所有LED"
        >
          关闭LED
        </button>
      </div>

      {/* 当前状态显示 */}
      <div className="led-status">
        <div className="status-item">
          <span className="status-label">当前模式:</span>
          <span className="status-value">
            {LED_MODES[ledMode].name}
          </span>
        </div>
        <div className="status-item">
          <span className="status-label">亮度:</span>
          <span className="status-value">{brightness}%</span>
        </div>
        <div className="status-item">
          <span className="status-label">间隔:</span>
          <span className="status-value">{interval}ms</span>
        </div>
      </div>

      {!isConnected && (
        <div className="led-warning">
          请先连接串口才能控制LED
        </div>
      )}
    </div>
  )
}

export default LEDControl
