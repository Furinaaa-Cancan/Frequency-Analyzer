import { useState, useEffect } from 'react'
import '../styles/TopBar.css'

function TopBar() {
  const [currentTime, setCurrentTime] = useState('')

  useEffect(() => {
    const updateTime = () => {
      const now = new Date()
      const timeStr = now.toLocaleTimeString('zh-CN', { 
        hour: '2-digit', 
        minute: '2-digit' 
      })
      setCurrentTime(timeStr)
    }
    
    updateTime()
    const interval = setInterval(updateTime, 1000)
    
    return () => clearInterval(interval)
  }, [])

  return (
    <div className="top-bar">
      <div className="top-bar-left">
        <div className="top-bar-logo">
          <span className="logo-text">FreqAnalyzer Pro</span>
          <span className="version-badge">v2.0</span>
        </div>
        <div className="top-bar-links">
          <a href="https://github.com" target="_blank" rel="noopener noreferrer" className="top-link">
            GitHub
          </a>
          <a href="#docs" className="top-link">
            文档
          </a>
          <a href="#contact" className="top-link">
            联系我们
          </a>
        </div>
      </div>
      <div className="top-bar-right">
        <div className="time-display">
          <span className="time-label">当前时间</span>
          <span className="time-value">{currentTime || '--:--'}</span>
        </div>
      </div>
    </div>
  )
}

export default TopBar


