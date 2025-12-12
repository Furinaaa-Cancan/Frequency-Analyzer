import { useState, useEffect } from 'react'
import '../styles/ErrorDialog.css'

/**
 * 协议错误对话框组件
 * 用于显示通信协议校验错误
 */
function ErrorDialog({ errors, onClose, onClearAll }) {
  const [isVisible, setIsVisible] = useState(false)

  useEffect(() => {
    if (errors && errors.length > 0) {
      setIsVisible(true)
    }
  }, [errors])

  const handleClose = () => {
    setIsVisible(false)
    if (onClose) onClose()
  }

  const handleClearAll = () => {
    if (onClearAll) onClearAll()
    setIsVisible(false)
  }

  if (!isVisible || !errors || errors.length === 0) {
    return null
  }

  // 按错误类型分组
  const groupedErrors = errors.reduce((acc, error) => {
    const type = error.type || 'unknown'
    if (!acc[type]) acc[type] = []
    acc[type].push(error)
    return acc
  }, {})

  const getErrorTypeLabel = (type) => {
    const labels = {
      'checksum': '校验和错误',
      'format': '格式错误',
      'range': '数据范围错误',
      'timeout': '超时错误',
      'parse': '解析错误',
      'unknown': '未知错误'
    }
    return labels[type] || type
  }

  const getErrorTypeIcon = (type) => {
    const icons = {
      'checksum': '⚠️',
      'format': '📋',
      'range': '📊',
      'timeout': '⏱️',
      'parse': '🔍',
      'unknown': '❓'
    }
    return icons[type] || '⚠️'
  }

  return (
    <div className="error-dialog-overlay" onClick={handleClose}>
      <div className="error-dialog" onClick={e => e.stopPropagation()}>
        <div className="error-dialog-header">
          <h3>⚠️ 协议错误</h3>
          <button className="close-btn" onClick={handleClose}>×</button>
        </div>
        
        <div className="error-dialog-content">
          <div className="error-summary">
            <span className="error-count">{errors.length}</span>
            <span className="error-label">个错误</span>
          </div>
          
          <div className="error-list">
            {Object.entries(groupedErrors).map(([type, typeErrors]) => (
              <div key={type} className="error-group">
                <div className="error-group-header">
                  <span className="error-icon">{getErrorTypeIcon(type)}</span>
                  <span className="error-type">{getErrorTypeLabel(type)}</span>
                  <span className="error-type-count">({typeErrors.length})</span>
                </div>
                <div className="error-group-items">
                  {typeErrors.slice(0, 5).map((error, idx) => (
                    <div key={idx} className="error-item">
                      <span className="error-time">{error.time}</span>
                      <span className="error-message">{error.message}</span>
                      {error.data && (
                        <code className="error-data">{error.data.substring(0, 50)}{error.data.length > 50 ? '...' : ''}</code>
                      )}
                    </div>
                  ))}
                  {typeErrors.length > 5 && (
                    <div className="error-more">
                      还有 {typeErrors.length - 5} 个类似错误...
                    </div>
                  )}
                </div>
              </div>
            ))}
          </div>
        </div>
        
        <div className="error-dialog-footer">
          <button className="btn-secondary" onClick={handleClearAll}>
            清除所有错误
          </button>
          <button className="btn-primary" onClick={handleClose}>
            确定
          </button>
        </div>
      </div>
    </div>
  )
}

export default ErrorDialog
