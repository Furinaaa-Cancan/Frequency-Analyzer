import { useState, useCallback } from 'react'

const MAX_LOG_ENTRIES = 200

export function useLogs() {
  const [logs, setLogs] = useState([
    { 
      message: '频率响应分析仪已加载', 
      type: 'info', 
      timestamp: new Date().toLocaleTimeString() 
    },
    { 
      message: '请点击"连接串口"按钮连接GD32F103设备', 
      type: 'info', 
      timestamp: new Date().toLocaleTimeString() 
    }
  ])

  const addLog = useCallback((message, type = 'info') => {
    const timestamp = new Date().toLocaleTimeString()
    
    setLogs(prev => {
      const newLogs = [...prev, { message, type, timestamp }]
      
      // 限制日志条数
      if (newLogs.length > MAX_LOG_ENTRIES) {
        return newLogs.slice(-MAX_LOG_ENTRIES)
      }
      
      return newLogs
    })
  }, [])

  return { logs, addLog }
}


