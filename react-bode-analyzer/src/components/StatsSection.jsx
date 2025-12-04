import { useMemo } from 'react'
import CountUp from './CountUp'
import '../styles/StatsSection.css'

function StatsSection({ dataPoints }) {
  const stats = useMemo(() => {
    if (dataPoints.length === 0) return null
    
    const frequencies = dataPoints.map(p => p.freq)
    const Hs = dataPoints.map(p => p.H)
    const thetas = dataPoints.map(p => p.theta)
    
    // 安全检查：确保数组不为空
    if (Hs.length === 0 || thetas.length === 0 || frequencies.length === 0) return null
    
    return {
      minFreq: Math.round(Math.min(...frequencies)),
      maxFreq: Math.round(Math.max(...frequencies)),
      avgH: parseFloat((Hs.reduce((a, b) => a + b, 0) / Hs.length).toFixed(4)),
      avgTheta: parseFloat((thetas.reduce((a, b) => a + b, 0) / thetas.length).toFixed(2))
    }
  }, [dataPoints])
  
  if (!stats) return null
  
  return (
    <div className="stats-grid">
      <div className="stat-card">
        <div className="stat-value">
          <CountUp to={stats.minFreq} duration={0.8} />
        </div>
        <div className="stat-label">最小频率 (Hz)</div>
      </div>
      <div className="stat-card">
        <div className="stat-value">
          <CountUp to={stats.maxFreq} duration={0.8} />
        </div>
        <div className="stat-label">最大频率 (Hz)</div>
      </div>
      <div className="stat-card">
        <div className="stat-value">
          <CountUp to={stats.avgH} duration={0.8} />
        </div>
        <div className="stat-label">平均增益</div>
      </div>
      <div className="stat-card">
        <div className="stat-value">
          <CountUp to={stats.avgTheta} duration={0.8} />
        </div>
        <div className="stat-label">平均相位 (°)</div>
      </div>
    </div>
  )
}

export default StatsSection


