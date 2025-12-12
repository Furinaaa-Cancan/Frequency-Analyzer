import '../styles/StatusBar.css'
import CountUp from './CountUp'

function StatusBar({ isConnected, dataCount, currentFreq, dataRate, enableProcessing, onToggleProcessing, errorCount = 0, onShowErrors }) {
  return (
    <div className="status-bar-container">
      <div className="status-bar-header">
        <h3>系统状态</h3>
        <div className="status-header-right">
          {/* 错误指示器 */}
          {errorCount > 0 && (
            <div className="error-indicator" onClick={onShowErrors} title="点击查看错误详情">
              <span className="error-indicator-icon">⚠️</span>
              <span className="error-indicator-count">{errorCount}</span>
              <span className="error-indicator-label">个错误</span>
            </div>
          )}
          <div className={`connection-status ${isConnected ? 'connected' : 'disconnected'}`}>
            <div className="status-icon"></div>
            <span className="status-label">{isConnected ? '设备在线' : '设备离线'}</span>
          </div>
        </div>
      </div>
      <div className="status-bar">
      <div className="status-item">
        <strong>数据点数:</strong>
        <span>
          <CountUp to={dataCount} duration={0.5} separator="," />
        </span>
      </div>
      <div className="status-item">
        <strong>当前频率:</strong>
        <span>
          {currentFreq ? (
            <>
              <CountUp to={currentFreq} duration={0.5} /> Hz
            </>
          ) : '-- Hz'}
        </span>
      </div>
      <div className="status-item">
        <strong>接收速率:</strong>
        <span>
          <CountUp to={dataRate} duration={0.5} /> 点/秒
        </span>
      </div>
      <div className="status-item toggle-item">
        <strong>数据处理:</strong>
        <button 
          onClick={onToggleProcessing}
          className={`toggle-button ${enableProcessing ? 'active' : ''}`}
          title={enableProcessing ? '智能处理已启用：Unwrap + 平滑滤波' : '仅显示原始数据'}
        >
          {enableProcessing ? '智能处理' : '原始数据'}
        </button>
      </div>
      </div>
    </div>
  )
}

export default StatusBar


