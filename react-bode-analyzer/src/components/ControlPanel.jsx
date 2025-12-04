import { useState } from 'react'
import '../styles/ControlPanel.css'

function ControlPanel({ 
  isConnected, 
  onConnect, 
  onDisconnect, 
  onSweep,
  onFreqTest,
  onClearData,
  onExportCSV,
  onExportImage,
  signalType,
  onSignalTypeChange
}) {
  const [singleFreq, setSingleFreq] = useState(100)

  const handleFreqChange = (e) => {
    const value = parseInt(e.target.value, 10) || 10
    setSingleFreq(Math.max(10, Math.min(1000, value)))
  }

  return (
    <div className="control-panel">
      <div className="section-header">
        <h3>课题二：频率响应测量与计算</h3>
        <span className="section-subtitle">ADC双通道同步采样，自动测量幅频响应H(ω)和相位差θ(ω)</span>
      </div>
      
      {/* 第一行：基础设置 */}
      <div className="control-row">
        <div className="control-group" style={{flex: '0 0 200px'}}>
          <label>信号类型</label>
          <select 
            value={signalType} 
            onChange={(e) => onSignalTypeChange(e.target.value)}
            title="选择输入信号类型"
          >
            <option value="sine">正弦波信号</option>
            <option value="ecg">心电ECG信号</option>
          </select>
        </div>
        
        <div className="control-group" style={{flex: '1', minWidth: '350px'}}>
          <label>串口连接</label>
          <div className="button-group">
            <button 
              className="btn-primary" 
              onClick={onConnect}
              disabled={isConnected}
              title="连接GD32开发板串口，波特率115200（固定）"
            >
              连接串口 (115200)
            </button>
            <button 
              className="btn-danger" 
              onClick={onDisconnect}
              disabled={!isConnected}
              title="断开与开发板的串口连接"
            >
              断开连接
            </button>
          </div>
        </div>
      </div>

      {/* 第二行：测试功能 */}
      {signalType === 'sine' ? (
        <>
          <div className="control-row">
            <div className="control-group" style={{flex: '1', minWidth: '300px'}}>
              <label>频率扫描</label>
              <button 
                className="btn-success" 
                onClick={onSweep}
                disabled={!isConnected}
                title="自动扫描10Hz到1000Hz频率范围，测量系统频率响应"
                style={{width: '100%'}}
              >
                开始频率扫描 (10-1000Hz)
              </button>
            </div>
            
            <div className="control-group" style={{flex: '1', minWidth: '250px'}}>
              <label>单频测试</label>
              <div className="input-group">
                <input 
                  type="number" 
                  min="10" 
                  max="1000" 
                  value={singleFreq}
                  onChange={handleFreqChange}
                  placeholder="10-1000 Hz"
                  style={{flex: '1'}}
                />
                <button 
                  className="btn-primary" 
                  onClick={() => onFreqTest(singleFreq)}
                  disabled={!isConnected}
                >
                  测试
                </button>
              </div>
            </div>
          </div>
          
          <div className="control-row">
            <div className="control-group" style={{flex: '1'}}>
              <label>数据管理</label>
              <div className="button-group">
                <button className="btn-secondary" onClick={onClearData}>
                  清除数据
                </button>
                <button className="btn-secondary" onClick={onExportCSV}>
                  导出CSV
                </button>
                <button className="btn-secondary" onClick={onExportImage}>
                  保存图片
                </button>
              </div>
            </div>
          </div>
        </>
      ) : (
        <>
          <div className="control-row">
            <div className="control-group" style={{flex: '1', minWidth: '250px'}}>
              <label>ECG监测</label>
              <button 
                className="btn-success" 
                onClick={() => onFreqTest(0)}
                disabled={!isConnected}
                title="开始实时采集并显示心电信号波形"
                style={{width: '100%'}}
              >
                开始实时监测
              </button>
            </div>
            
            <div className="control-group" style={{flex: '1', minWidth: '300px'}}>
              <label>数据管理</label>
              <div className="button-group">
                <button className="btn-secondary" onClick={onClearData}>
                  清除数据
                </button>
                <button className="btn-secondary" onClick={onExportImage}>
                  保存图片
                </button>
              </div>
            </div>
          </div>
          
          <div className="control-hint">
            💡 提示: ECG模式仅显示波形，不进行频响分析
          </div>
        </>
      )}
    </div>
  )
}

export default ControlPanel


