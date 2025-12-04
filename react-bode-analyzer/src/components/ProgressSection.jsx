import CountUp from './CountUp'
import '../styles/ProgressSection.css'

function ProgressSection({ current, total }) {
  if (total === 0) return null
  
  const percentage = Math.round((current / total) * 100)
  
  return (
    <div className="progress-section">
      <div className="progress-header">
        <span className="progress-label">扫描进度</span>
        <span className="progress-text">
          <CountUp to={percentage} duration={0.5} />% (
          <CountUp to={current} duration={0.5} />/
          <CountUp to={total} duration={0.5} />)
        </span>
      </div>
      <div className="progress-container">
        <div className="progress-bar" style={{ width: `${percentage}%` }}></div>
      </div>
    </div>
  )
}

export default ProgressSection


