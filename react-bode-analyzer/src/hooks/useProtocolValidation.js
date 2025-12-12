import { useState, useCallback, useRef } from 'react'

/**
 * 协议校验Hook
 * 用于检测和记录通信协议错误
 */
export function useProtocolValidation() {
  const [errors, setErrors] = useState([])
  const [showErrorDialog, setShowErrorDialog] = useState(false)
  const errorCountRef = useRef(0)
  const maxErrors = 100  // 最多保留100条错误

  // 添加错误
  const addError = useCallback((type, message, data = null) => {
    const now = new Date()
    const time = now.toLocaleTimeString('zh-CN', { 
      hour: '2-digit', 
      minute: '2-digit', 
      second: '2-digit',
      fractionalSecondDigits: 3 
    })
    
    const error = {
      id: errorCountRef.current++,
      type,
      message,
      data,
      time,
      timestamp: now.getTime()
    }
    
    setErrors(prev => {
      const newErrors = [error, ...prev]
      // 限制错误数量
      if (newErrors.length > maxErrors) {
        return newErrors.slice(0, maxErrors)
      }
      return newErrors
    })
    
    console.warn(`[协议错误] ${type}: ${message}`, data)
    return error
  }, [])

  // 清除所有错误
  const clearErrors = useCallback(() => {
    setErrors([])
  }, [])

  // 校验FREQ_RESP数据
  // 格式: FREQ_RESP:freq,K,K1,H,theta (5个参数，无校验和)
  const validateFreqResp = useCallback((data, strictMode = false) => {
    if (!data.startsWith('FREQ_RESP:')) {
      if (strictMode) {
        return { valid: false, error: 'format', message: '不是FREQ_RESP格式' }
      }
      return { valid: true }
    }
    
    const content = data.substring(10)
    const parts = content.split(',')
    
    // 实际格式: freq,K,K1,H,theta (5个参数)
    if (parts.length < 3) {
      if (strictMode) {
        addError('format', `FREQ_RESP格式错误：需要至少3个参数，收到${parts.length}个`, data)
      }
      return { valid: false, error: 'format', message: '参数不足' }
    }
    
    const freq = parseFloat(parts[0])
    // 根据参数数量判断格式
    let H, theta
    if (parts.length >= 5) {
      // 新格式: freq,K,K1,H,theta
      H = parseFloat(parts[3])
      theta = parseFloat(parts[4])
    } else {
      // 旧格式: freq,H,theta
      H = parseFloat(parts[1])
      theta = parseFloat(parts[2])
    }
    
    // 检查数值有效性
    if (isNaN(freq) || isNaN(H) || isNaN(theta)) {
      if (strictMode) {
        addError('parse', `FREQ_RESP解析错误：freq=${parts[0]}, H=${H}, theta=${theta}`, data)
      }
      return { valid: false, error: 'parse', message: '数值解析失败' }
    }
    
    // 仅在严格模式下检查范围
    if (strictMode) {
      if (freq < 1 || freq > 100000) {
        addError('range', `频率超出范围: ${freq}Hz (有效范围: 1-100000Hz)`, data)
        return { valid: false, error: 'range', message: '频率超出范围' }
      }
      
      if (H < 0 || H > 1000) {
        addError('range', `幅度异常: H=${H} (预期范围: 0-1000)`, data)
        return { valid: false, error: 'range', message: '幅度异常' }
      }
      
      if (theta < -360 || theta > 360) {
        addError('range', `相位超出范围: ${theta}° (有效范围: -360°~360°)`, data)
        return { valid: false, error: 'range', message: '相位超出范围' }
      }
    }
    
    return { valid: true, freq, H, theta }
  }, [addError])

  // 校验UWAVE数据
  const validateUWave = useCallback((data) => {
    if (!data.includes('UWAVE:')) {
      return { valid: false, error: 'format', message: '不是UWAVE格式' }
    }
    
    const idx = data.indexOf('UWAVE:')
    const content = data.substring(idx + 6)
    const parts = content.split(',')
    
    if (parts.length < 2) {
      addError('format', `UWAVE格式错误：需要2个参数，收到${parts.length}个`, data)
      return { valid: false, error: 'format', message: '参数不足' }
    }
    
    const signalFreq = parseInt(parts[0], 10)
    const sampleRate = parseInt(parts[1], 10)
    
    if (isNaN(signalFreq) || isNaN(sampleRate)) {
      addError('parse', `UWAVE解析错误：freq=${parts[0]}, sr=${parts[1]}`, data)
      return { valid: false, error: 'parse', message: '数值解析失败' }
    }
    
    if (signalFreq < 10 || signalFreq > 2000) {
      addError('range', `信号频率超出范围: ${signalFreq}Hz (有效: 10-2000Hz)`, data)
      return { valid: false, error: 'range', message: '信号频率超出范围' }
    }
    
    if (sampleRate < 10 || sampleRate > 50000) {
      addError('range', `采样率超出范围: ${sampleRate}Hz (有效: 10-50000Hz)`, data)
      return { valid: false, error: 'range', message: '采样率超出范围' }
    }
    
    return { valid: true, signalFreq, sampleRate }
  }, [addError])

  // 校验D:数据
  const validateDData = useCallback((data, expectedPoints = 64) => {
    if (!data.startsWith('D:')) {
      return { valid: false, error: 'format', message: '不是D:格式' }
    }
    
    const content = data.substring(2)
    const pairs = content.split(';')
    
    if (pairs.length < expectedPoints * 0.5) {
      addError('format', `数据点不足：期望${expectedPoints}点，收到${pairs.length}点`, data.substring(0, 100))
      return { valid: false, error: 'format', message: '数据点不足' }
    }
    
    let invalidCount = 0
    for (let i = 0; i < Math.min(pairs.length, 10); i++) {
      const parts = pairs[i].split(',')
      if (parts.length < 2) {
        invalidCount++
        continue
      }
      const v0 = parseInt(parts[0], 10)
      const v1 = parseInt(parts[1], 10)
      if (isNaN(v0) || isNaN(v1)) {
        invalidCount++
      }
      // 检查ADC范围
      if (v0 < 0 || v0 > 4095 || v1 < 0 || v1 > 4095) {
        addError('range', `ADC值超出范围: [${v0}, ${v1}] (有效: 0-4095)`, `点${i}: ${pairs[i]}`)
      }
    }
    
    if (invalidCount > pairs.length * 0.3) {
      addError('parse', `数据格式错误率过高: ${invalidCount}/${pairs.length}`, data.substring(0, 100))
      return { valid: false, error: 'parse', message: '数据格式错误率过高' }
    }
    
    return { valid: true, pointCount: pairs.length }
  }, [addError])

  // 通用数据校验入口（仅用于手动测试，不自动校验串口数据）
  const validateData = useCallback((data, strictMode = true) => {
    if (!data || typeof data !== 'string') {
      return { valid: false, error: 'format', message: '无效数据' }
    }
    
    // 根据数据类型选择校验方法
    if (data.startsWith('FREQ_RESP:')) {
      return validateFreqResp(data, strictMode)
    }
    if (data.includes('UWAVE:')) {
      return validateUWave(data)
    }
    if (data.startsWith('D:')) {
      return validateDData(data)
    }
    
    // 其他数据类型暂不校验
    return { valid: true }
  }, [validateFreqResp, validateUWave, validateDData])

  // 计算校验和（XOR）
  const calculateChecksum = (str) => {
    let checksum = 0
    for (let i = 0; i < str.length; i++) {
      checksum ^= str.charCodeAt(i)
    }
    return checksum
  }

  return {
    errors,
    showErrorDialog,
    setShowErrorDialog,
    addError,
    clearErrors,
    validateData,
    validateFreqResp,
    validateUWave,
    validateDData,
    errorCount: errors.length
  }
}
