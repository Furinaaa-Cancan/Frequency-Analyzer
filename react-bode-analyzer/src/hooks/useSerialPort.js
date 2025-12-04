import { useState, useCallback, useRef } from 'react'

const CONFIG = {
  BAUD_RATE_DEFAULT: 115200,
  CONNECTION_TIMEOUT: 30000,
  DISCONNECT_TIMEOUT: 2000
}

export function useSerialPort() {
  const [isConnected, setIsConnected] = useState(false)
  const [dataRate, setDataRate] = useState(0)
  const portRef = useRef(null)
  const readerRef = useRef(null)
  const dataCountRef = useRef(0)
  const lastReceiveTimeRef = useRef(Date.now())
  const dataCallbackRef = useRef(null)

  const connect = useCallback(async (addLog, onDataReceived) => {
    try {
      if (!('serial' in navigator)) {
        addLog('Web Serial API 不支持，请使用Chrome、Edge或Opera浏览器', 'error')
        alert('您的浏览器不支持Web Serial API\n请使用最新版本的Chrome、Edge或Opera浏览器')
        return false
      }

      const portPromise = navigator.serial.requestPort()
      const timeoutPromise = new Promise((_, reject) => 
        setTimeout(() => reject(new Error('连接超时')), CONFIG.CONNECTION_TIMEOUT)
      )
      
      const port = await Promise.race([portPromise, timeoutPromise])
      await port.open({ baudRate: CONFIG.BAUD_RATE_DEFAULT })

      portRef.current = port
      dataCallbackRef.current = onDataReceived
      setIsConnected(true)
      addLog(`串口连接成功，波特率: ${CONFIG.BAUD_RATE_DEFAULT}`, 'success')
      
      // 开始读取数据
      readSerialData(addLog)
      
      return true
    } catch (error) {
      if (error.message === '连接超时') {
        addLog('连接超时，请重试', 'error')
      } else if (error.name === 'NotFoundError') {
        addLog('未选择串口设备', 'warning')
      } else {
        addLog(`串口连接失败: ${error.message}`, 'error')
      }
      console.error(error)
      return false
    }
  }, [])

  const disconnect = useCallback(async (addLog) => {
    try {
      // 如果已经断开，直接返回
      if (!portRef.current && !readerRef.current) {
        return
      }
      
      setIsConnected(false)
      
      if (readerRef.current) {
        try {
          await Promise.race([
            readerRef.current.cancel(),
            new Promise((resolve) => setTimeout(resolve, 1000))
          ])
        } catch (e) {
          // 忽略已取消或已关闭的错误
          if (e.name !== 'InvalidStateError' && e.name !== 'AbortError') {
            console.warn('Reader cancel error:', e)
          }
        }
        readerRef.current = null
      }
      
      if (portRef.current) {
        try {
          await Promise.race([
            portRef.current.close(),
            new Promise((resolve) => setTimeout(resolve, CONFIG.DISCONNECT_TIMEOUT))
          ])
        } catch (e) {
          // 忽略已关闭的错误
          if (e.name !== 'InvalidStateError') {
            console.warn('Port close error:', e)
          }
        }
        portRef.current = null
      }
      
      if (addLog) {
        addLog('串口已安全断开', 'info')
      }
    } catch (error) {
      if (addLog) {
        addLog(`断开串口失败: ${error.message}`, 'error')
      }
      readerRef.current = null
      portRef.current = null
    }
  }, [])

  const sendCommand = useCallback(async (command, addLog) => {
    if (!isConnected || !portRef.current) {
      addLog('串口未连接', 'error')
      return
    }

    try {
      const writer = portRef.current.writable.getWriter()
      const data = new TextEncoder().encode(command + '\r\n')
      await writer.write(data)
      writer.releaseLock()
      addLog(`发送命令: ${command}`, 'info')
    } catch (error) {
      addLog(`发送命令失败: ${error.message}`, 'error')
    }
  }, [isConnected])

  const readSerialData = async (addLog) => {
    let readableStreamClosed = null
    try {
      if (!portRef.current) {
        addLog('串口未初始化', 'error')
        return
      }
      
      const textDecoder = new TextDecoderStream()
      readableStreamClosed = portRef.current.readable.pipeTo(textDecoder.writable)
      readerRef.current = textDecoder.readable.getReader()

      let buffer = ''

      while (true) {
        const { value, done } = await readerRef.current.read()
        if (done) {
          readerRef.current.releaseLock()
          addLog('读取器已关闭', 'info')
          break
        }

        buffer += value
        
        let lines = buffer.split('\n')
        buffer = lines.pop()

        for (let line of lines) {
          line = line.trim()
          if (line) {
            // 触发数据处理回调
            if (dataCallbackRef.current) {
              dataCallbackRef.current(line)
            }
            // 记录日志（FREQ_RESP数据会在dataPoints中处理，不需要重复记录）
            if (!line.startsWith('FREQ_RESP:')) {
              addLog(`接收: ${line}`, 'info')
            }
          }
        }

        // 更新接收速率
        dataCountRef.current++
        const now = Date.now()
        if (now - lastReceiveTimeRef.current > 1000) {
          setDataRate(dataCountRef.current)
          dataCountRef.current = 0
          lastReceiveTimeRef.current = now
        }
      }
      
      if (readableStreamClosed) {
        await readableStreamClosed.catch(() => {})
      }
    } catch (error) {
      // AbortError 是正常的取消操作，不需要记录为错误
      if (error.name !== 'AbortError') {
        addLog(`读取数据错误: ${error.message}`, 'error')
        console.error('Serial read error:', error)
      }
    } finally {
      // 确保清理资源
      if (readerRef.current) {
        try {
          readerRef.current.releaseLock()
        } catch (e) {
          // reader可能已经释放，忽略错误
          console.warn('Reader release warning:', e.message)
        }
      }
      if (readableStreamClosed) {
        try {
          await readableStreamClosed.catch(() => {})
        } catch (e) {
          console.warn('Stream close warning:', e.message)
        }
      }
    }
  }

  return {
    isConnected,
    dataRate,
    serialPort: portRef.current,  // 暴露serialPort供其他组件使用
    connect,
    disconnect,
    sendCommand
  }
}


