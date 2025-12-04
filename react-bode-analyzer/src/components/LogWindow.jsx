import { useState, useEffect, useRef } from 'react'
import '../styles/LogWindow.css'

function LogWindow({ logs }) {
  const [isOpen, setIsOpen] = useState(false)
  const logContentRef = useRef(null)

  useEffect(() => {
    if (logContentRef.current) {
      logContentRef.current.scrollTop = logContentRef.current.scrollHeight
    }
  }, [logs])

  return (
    <>
      <div className="section-toggle" onClick={() => setIsOpen(!isOpen)}>
        <span>通讯日志</span>
        <span className={`toggle-icon ${isOpen ? 'open' : ''}`}>▼</span>
      </div>
      {isOpen && (
        <div className="log-container" ref={logContentRef}>
          {logs.map((log, index) => (
            <div key={index} className={`log-entry ${log.type}`}>
              [{log.timestamp}] {log.message}
            </div>
          ))}
        </div>
      )}
    </>
  )
}

export default LogWindow


