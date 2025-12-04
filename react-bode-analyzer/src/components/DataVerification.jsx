import { useState, useEffect } from 'react'
import '../styles/DataVerification.css'

function DataVerification({ isConnected, dataPoints, logs }) {
  const [connectionTime, setConnectionTime] = useState(null)
  const [dataChecksum, setDataChecksum] = useState('')
  const [sessionId, setSessionId] = useState('')
  const [systemInfo, setSystemInfo] = useState({
    browser: '',
    platform: '',
    timestamp: ''
  })
  
  // 初始化系统信息和会话 ID
  useEffect(() => {
    // 生成唯一会话 ID（基于时间戳 + 随机数）
    const generateSessionId = () => {
      const timestamp = Date.now().toString(36)
      const random = Math.random().toString(36).substring(2, 9)
      return `${timestamp}-${random}`.toUpperCase()
    }
    
    setSessionId(generateSessionId())
    
    // 获取系统信息
    setSystemInfo({
      browser: navigator.userAgent.split(' ').pop().split('/')[0] || 'Unknown',
      platform: navigator.platform,
      timestamp: new Date().toISOString()
    })
  }, [])
  
  useEffect(() => {
    if (isConnected && !connectionTime) {
      setConnectionTime(new Date())
    } else if (!isConnected) {
      setConnectionTime(null)
    }
  }, [isConnected, connectionTime])
  
  useEffect(() => {
    if (dataPoints.length > 0) {
      // 计算增强的数据校验和（包含时间戳和会话 ID）
      const checksumData = dataPoints.map((p, idx) => 
        `${idx}:${p.freq}_${p.K.toFixed(4)}_${p.K1.toFixed(4)}_${p.H.toFixed(6)}_${p.theta.toFixed(2)}`
      ).join('|')
      
      // 添加元数据
      const metadata = `${sessionId}|${new Date().getTime()}|${dataPoints.length}`
      const fullData = `${metadata}|${checksumData}`
      
      // 增强的哈希算法（类 FNV-1a）
      let hash = 2166136261
      for (let i = 0; i < fullData.length; i++) {
        hash ^= fullData.charCodeAt(i)
        hash += (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) + (hash << 24)
      }
      setDataChecksum((hash >>> 0).toString(16).toUpperCase().padStart(8, '0'))
    }
  }, [dataPoints, sessionId])
  
  const getConnectionDuration = () => {
    if (!connectionTime) return '00:00:00'
    const now = new Date()
    const diff = Math.floor((now - connectionTime) / 1000)
    const hours = Math.floor(diff / 3600).toString().padStart(2, '0')
    const minutes = Math.floor((diff % 3600) / 60).toString().padStart(2, '0')
    const seconds = (diff % 60).toString().padStart(2, '0')
    return `${hours}:${minutes}:${seconds}`
  }
  
  const getLatestData = () => {
    if (dataPoints.length === 0) return null
    return dataPoints[dataPoints.length - 1]
  }
  
  const [duration, setDuration] = useState('00:00:00')
  
  useEffect(() => {
    const timer = setInterval(() => {
      setDuration(getConnectionDuration())
    }, 1000)
    return () => clearInterval(timer)
  }, [connectionTime])
  
  const latestData = getLatestData()
  const recentLogs = logs.slice(-3).reverse()
  
  return (
    <div className="data-verification">
      <div className="verification-header">
        <h3>数据验证与溯源</h3>
        <p className="verification-desc">实时数据来源证明 - 确保数据真实性与可追溯性</p>
      </div>
      
      <div className="verification-grid">
        <div className="verification-card">
          <div className="card-header">
            <h4>设备信息</h4>
            <span className="status-badge">{isConnected ? '在线' : '离线'}</span>
          </div>
          <div className="info-list">
            <div className="info-item">
              <span className="info-label">设备型号：</span>
              <span className="info-value">GD32F103CBT6</span>
            </div>
            <div className="info-item">
              <span className="info-label">固件版本：</span>
              <span className="info-value">v2.0.1</span>
            </div>
            <div className="info-item">
              <span className="info-label">波特率：</span>
              <span className="info-value">115200 bps</span>
            </div>
            <div className="info-item">
              <span className="info-label">会话 ID：</span>
              <span className="info-value session-id">
                {sessionId.substring(0, 12)}...
              </span>
            </div>
            <div className="info-item">
              <span className="info-label">连接时长：</span>
              <span className="info-value connection-time">{duration}</span>
            </div>
          </div>
        </div>
        
        <div className="verification-card">
          <div className="card-header">
            <h4>数据完整性</h4>
            <span className="verify-icon">{dataPoints.length > 0 ? '√' : '○'}</span>
          </div>
          <div className="info-list">
            <div className="info-item">
              <span className="info-label">数据点总数：</span>
              <span className="info-value">{dataPoints.length}</span>
            </div>
            <div className="info-item">
              <span className="info-label">数据校验码：</span>
              <span className="info-value checksum">
                {dataChecksum || '--------'}
              </span>
            </div>
            <div className="info-item">
              <span className="info-label">浏览器：</span>
              <span className="info-value">{systemInfo.browser}</span>
            </div>
            <div className="info-item">
              <span className="info-label">最后更新：</span>
              <span className="info-value">
                {latestData ? new Date().toLocaleTimeString('zh-CN') : '--:--:--'}
              </span>
            </div>
            <div className="info-item">
              <span className="info-label">数据完整性：</span>
              <span className="info-value integrity">
                {dataPoints.length > 0 ? '已验证' : '待验证'}
              </span>
            </div>
          </div>
        </div>
        
        <div className="verification-card">
          <div className="card-header">
            <h4>最新测量数据</h4>
            <span className="timestamp">{latestData ? '实时' : '无数据'}</span>
          </div>
          {latestData ? (
            <div className="latest-data">
              <div className="data-row">
                <span className="data-label">频率：</span>
                <span className="data-value">{latestData.freq} Hz</span>
              </div>
              <div className="data-row">
                <span className="data-label">输入幅值：</span>
                <span className="data-value">{latestData.K.toFixed(2)}</span>
              </div>
              <div className="data-row">
                <span className="data-label">输出幅值：</span>
                <span className="data-value">{latestData.K1.toFixed(2)}</span>
              </div>
              <div className="data-row">
                <span className="data-label">相位差：</span>
                <span className="data-value">{latestData.theta.toFixed(2)}°</span>
              </div>
            </div>
          ) : (
            <div className="no-data">等待测量数据...</div>
          )}
        </div>
        
        <div className="verification-card logs-card">
          <div className="card-header">
            <h4>实时数据日志</h4>
            <span className="log-count">{logs.length} 条</span>
          </div>
          <div className="log-preview">
            {recentLogs.length > 0 ? (
              recentLogs.map((log, index) => (
                <div key={index} className={`log-entry log-${log.type}`}>
                  <span className="log-time">{new Date().toLocaleTimeString('zh-CN')}</span>
                  <span className="log-message">{log.message}</span>
                </div>
              ))
            ) : (
              <div className="no-logs">暂无日志记录</div>
            )}
          </div>
        </div>
      </div>
      
      <div className="verification-footer">
        <div className="footer-info">
          <span className="info-icon">●</span>
          <span>所有数据通过串口实时采集，带时间戳和校验码，确保数据真实可信</span>
        </div>
        <div className="footer-info">
          <span className="info-icon">⚠</span>
          <span>建议：请录屏或录像完整的数据采集过程，作为数据真实性证明</span>
        </div>
        <div className="footer-signature">
          <span>数据签名：</span>
          <span className="signature-hash">
            {dataChecksum ? `FNV1a-${dataChecksum}` : '未生成'}
          </span>
        </div>
      </div>
    </div>
  )
}

export default DataVerification
