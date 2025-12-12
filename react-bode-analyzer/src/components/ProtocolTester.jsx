import { useState } from 'react'
import '../styles/ProtocolTester.css'

/**
 * 协议测试组件
 * 用户输入协议数据，系统校验并显示结果
 */
function ProtocolTester({ onValidate }) {
  const [input, setInput] = useState('')
  const [result, setResult] = useState(null)

  // 协议示例
  const examples = [
    { label: '正确: FREQ_RESP', value: 'FREQ_RESP:100,1.5,-45' },
    { label: '错误: 参数不足', value: 'FREQ_RESP:100' },
    { label: '错误: 频率超范围', value: 'FREQ_RESP:999999,1.5,-45' },
    { label: '错误: H值异常', value: 'FREQ_RESP:100,abc,-45' },
    { label: '正确: UWAVE', value: 'UWAVE:1000,700' },
    { label: '错误: 采样率超范围', value: 'UWAVE:1000,100000' },
  ]

  const handleValidate = () => {
    if (!input.trim()) {
      setResult({ valid: false, message: '请输入协议数据' })
      return
    }

    // 调用校验
    const validation = onValidate(input.trim())
    setResult(validation)
  }

  const handleExample = (value) => {
    setInput(value)
    setResult(null)
  }

  const handleKeyPress = (e) => {
    if (e.key === 'Enter') {
      handleValidate()
    }
  }

  return (
    <div className="protocol-tester">
      <div className="section-header">
        <h3>协议校验测试</h3>
        <span className="section-subtitle">输入协议数据，检测格式是否正确</span>
      </div>

      <div className="tester-content">
        {/* 输入区域 */}
        <div className="input-section">
          <label>输入协议数据</label>
          <div className="input-row">
            <input
              type="text"
              value={input}
              onChange={(e) => setInput(e.target.value)}
              onKeyPress={handleKeyPress}
              placeholder="例如: FREQ_RESP:100,1.5,-45"
              className="protocol-input"
            />
            <button className="btn-validate" onClick={handleValidate}>
              校验
            </button>
          </div>
        </div>

        {/* 快速示例 */}
        <div className="examples-section">
          <label>快速示例</label>
          <div className="example-buttons">
            {examples.map((ex, idx) => (
              <button
                key={idx}
                className={`example-btn ${ex.label.includes('错误') ? 'error' : 'success'}`}
                onClick={() => handleExample(ex.value)}
                title={ex.value}
              >
                {ex.label}
              </button>
            ))}
          </div>
        </div>

        {/* 校验结果 */}
        {result && (
          <div className={`result-section ${result.valid ? 'valid' : 'invalid'}`}>
            <div className="result-icon">
              {result.valid ? '✓' : '✗'}
            </div>
            <div className="result-content">
              <div className="result-title">
                {result.valid ? '校验通过' : '校验失败'}
              </div>
              {result.message && (
                <div className="result-message">{result.message}</div>
              )}
              {result.error && (
                <div className="result-error">
                  <span className="error-type">{result.error}</span>
                </div>
              )}
              {result.valid && result.freq && (
                <div className="result-details">
                  <span>频率: {result.freq}Hz</span>
                  {result.H !== undefined && <span>H: {result.H}</span>}
                  {result.theta !== undefined && <span>θ: {result.theta}°</span>}
                </div>
              )}
            </div>
          </div>
        )}

        {/* 协议说明 */}
        <div className="protocol-help">
          <h4>支持的协议格式</h4>
          <ul>
            <li><code>FREQ_RESP:freq,H,theta</code> - 频率响应数据</li>
            <li><code>UWAVE:signalFreq,sampleRate</code> - 欠采样波形</li>
            <li><code>D:v0,v1;v0,v1;...</code> - ADC数据</li>
          </ul>
        </div>
      </div>
    </div>
  )
}

export default ProtocolTester
