import { useState, useCallback, useMemo } from 'react'

const CONFIG = {
  FREQ_MIN: 10,
  FREQ_MAX: 2000,
  H_MIN: 0,
  H_MAX: 10,       // 增加到10，容许更大的增益（运放可能有放大）
  THETA_MIN: -200, // 扩大范围，容许相位计算的wrap现象
  THETA_MAX: 200,  // 扩大范围
  MAX_DATA_POINTS: 1000,
  // 数据处理参数
  PHASE_DEVIATION_THRESHOLD: 50,  // 相位偏差阈值（度）
  SMOOTH_WINDOW_LOW_FREQ: 7,      // 低频平滑窗口
  SMOOTH_WINDOW_MID_FREQ: 5,      // 中频平滑窗口
  SMOOTH_WINDOW_HIGH_FREQ: 3,     // 高频平滑窗口
  WAVEFORM_BUFFER_ECG_MS: 2000,   // ECG波形缓冲时长（ms）
  WAVEFORM_BUFFER_SINE_MS: 200    // 正弦波形缓冲时长（ms）- 200ms足够显示几个周期
}

export function useDataPoints() {
  const [dataPoints, setDataPoints] = useState([])
  const [waveformData, setWaveformData] = useState(null)
  const [enableProcessing, setEnableProcessing] = useState(true)  // 数据处理开关
  
  // 保存所有频率点的波形数据（新增）
  const [allWaveforms, setAllWaveforms] = useState(new Map())  // key: freq, value: {input, output, timeStamps, freq, sampleRate}
  
  // 波形滚动显示相关状态
  const [waveformTimeOffset, setWaveformTimeOffset] = useState(0)  // 累积时间偏移（ms）
  const [waveformBuffer, setWaveformBuffer] = useState({
    input: [],
    output: [],
    timeStamps: [],
    freq: 0,
    sampleRate: 0
  })

  const addDataPoint = useCallback((data, addLog, signalType = 'sine') => {
    // 处理真实欠采样波形：UWAVE:freq,sampleRate
    if (data.includes('UWAVE:')) {
      try {
        const idx = data.indexOf('UWAVE:')
        const parts = data.substring(idx + 6).split(',')
        window._uwaveTemp = {
          signalFreq: parseInt(parts[0], 10),
          sampleRate: parseInt(parts[1], 10)  // 真实欠采样率
        }
        console.log('[UWAVE] 真实欠采样:', window._uwaveTemp)
      } catch (e) {
        console.error('UWAVE解析错误:', e)
      }
      return
    }
    
    // 处理快速波形数据：WAVE:freq,sampleRate（可能有前缀，但排除RAWWAVE和UWAVE）
    if (data.includes('WAVE:') && !data.includes('RAWWAVE:') && !data.includes('UWAVE:')) {
      try {
        const waveIdx = data.indexOf('WAVE:')
        const waveData = data.substring(waveIdx + 5)
        const parts = waveData.split(',')
        window._waveTemp = {
          freq: parseInt(parts[0], 10),
          sampleRate: parseInt(parts[1], 10)
        }
        console.log('[WAVE] 解析:', window._waveTemp)
      } catch (e) {
        console.error('WAVE解析错误:', e)
      }
      return
    }
    
    // 处理真实欠采样数据内容：D:ch0,ch1;ch0,ch1;...（双通道）
    if (data.startsWith('D:') && window._uwaveTemp) {
      try {
        const pairs = data.substring(2).split(';')
        const ch0 = []
        const ch1 = []
        pairs.forEach(pair => {
          const [v0, v1] = pair.split(',').map(v => parseInt(v, 10))
          if (!isNaN(v0)) ch0.push(v0)
          if (!isNaN(v1)) ch1.push(v1)
        })
        
        // 触发uwave-data事件（真实欠采样数据，双通道）
        const event = new CustomEvent('uwave-data', {
          detail: {
            signalFreq: window._uwaveTemp.signalFreq,
            sampleRate: window._uwaveTemp.sampleRate,
            ch0: ch0,  // PA6
            ch1: ch1   // PB1
          }
        })
        window.dispatchEvent(event)
        console.log('[UWAVE] 真实数据: PA6=', ch0.length, '点, PB1=', ch1.length, '点')
        window._uwaveTemp = null
      } catch (e) {
        console.error('UWAVE D:解析错误:', e)
      }
      return
    }
    
    // 处理快速波形数据内容：D:ch0,ch1;ch0,ch1;...
    if (data.startsWith('D:') && window._waveTemp) {
      try {
        const pairs = data.substring(2).split(';')
        const input = []
        const output = []
        const timeStamps = []
        const dt = 1000 / window._waveTemp.sampleRate  // ms per sample
        
        pairs.forEach((pair, i) => {
          const [ch0, ch1] = pair.split(',').map(v => parseInt(v, 10))
          if (!isNaN(ch0) && !isNaN(ch1)) {
            input.push(ch0)
            output.push(ch1)
            timeStamps.push(i * dt)
          }
        })
        
        // 触发波形更新事件
        const waveData = {
          freq: window._waveTemp.freq,
          sampleRate: window._waveTemp.sampleRate,
          input,
          output,
          timeStamps
        }
        
        // 更新waveformBuffer用于LiveSineWaveMonitor
        console.log('[WAVE] 波形数据更新:', waveData.freq + 'Hz', input.length + '点')
        setWaveformBuffer(waveData)
        setWaveformData(waveData)
        
        window._waveTemp = null
      } catch (e) {
        console.error('D:解析错误:', e)
      }
      return
    }
    
    // 处理欠采样波形采集数据：RAWWAVE:freq,sampleRate,totalTime
    // 后续还有 CH0:data 和 CH1:data 行
    if (data.startsWith('RAWWAVE:')) {
      try {
        const parts = data.substring(8).split(',')
        const signalFreq = parseInt(parts[0], 10)
        const sampleRate = parseInt(parts[1], 10)
        const totalTime = parseFloat(parts[2])
        
        // 保存到临时存储，等待CH0和CH1数据
        window._rawwaveTemp = { signalFreq, sampleRate, totalTime, ch0: null, ch1: null }
        addLog(`[RAWWAVE] 信号${signalFreq}Hz, 采样${sampleRate}Hz, 时长${totalTime}ms`, 'info')
      } catch (e) {
        addLog(`RAWWAVE解析错误: ${e.message}`, 'warning')
      }
      return
    }
    
    // 处理CH0数据（欠采样波形）
    if (data.startsWith('CH0:') && window._rawwaveTemp) {
      try {
        const dataStr = data.substring(4)
        window._rawwaveTemp.ch0 = dataStr.split(',').map(v => parseInt(v, 10)).filter(v => !isNaN(v))
      } catch (e) {
        addLog(`CH0解析错误: ${e.message}`, 'warning')
      }
      return
    }
    
    // 处理CH1数据（欠采样波形）
    if (data.startsWith('CH1:') && window._rawwaveTemp) {
      try {
        const dataStr = data.substring(4)
        window._rawwaveTemp.ch1 = dataStr.split(',').map(v => parseInt(v, 10)).filter(v => !isNaN(v))
        
        // CH0和CH1都收到了，触发事件
        if (window._rawwaveTemp.ch0 && window._rawwaveTemp.ch1) {
          const event = new CustomEvent('rawwave-data', {
            detail: window._rawwaveTemp
          })
          window.dispatchEvent(event)
          addLog(`[RAWWAVE] 数据接收完成: CH0=${window._rawwaveTemp.ch0.length}点, CH1=${window._rawwaveTemp.ch1.length}点`, 'success')
          window._rawwaveTemp = null
        }
      } catch (e) {
        addLog(`CH1解析错误: ${e.message}`, 'warning')
      }
      return
    }
    
    // 处理波形数据：WAVEFORM:freq,sampleRate,inputData,outputData
    if (data.startsWith('WAVEFORM:')) {
      try {
        const parts = data.substring(9).split(',')
        const freq = parseInt(parts[0], 10)
        const sampleRate = parseInt(parts[1], 10)
        
        // 验证基本参数
        if (isNaN(freq) || isNaN(sampleRate) || sampleRate <= 0) {
          addLog('波形数据格式错误：频率或采样率无效', 'warning')
          return
        }
        const dataStart = data.indexOf(',', data.indexOf(',') + 1) + 1
        const dataStr = data.substring(dataStart)
        const splitData = dataStr.split('|')
        if (splitData.length !== 2) {
          addLog('波形数据格式错误：缺少输入输出分隔符', 'warning')
          return
        }
        
        const [inputStr, outputStr] = splitData
        const newInput = inputStr.split(',').map(v => parseInt(v, 10)).filter(v => !isNaN(v))
        const newOutput = outputStr.split(',').map(v => parseInt(v, 10)).filter(v => !isNaN(v))
        
        if (newInput.length > 0 && newOutput.length > 0) {
          const dataLengthMs = (newInput.length / sampleRate) * 1000
          
          // 检测频率或采样率是否变化
          const freqChanged = waveformBuffer.freq !== 0 && waveformBuffer.freq !== freq
          const sampleRateChanged = waveformBuffer.sampleRate !== 0 && waveformBuffer.sampleRate !== sampleRate
          
          // 使用函数式更新
          setWaveformTimeOffset(currentOffset => {
            if (sampleRateChanged && !freqChanged) {
              console.log(`${freq}Hz采样率变化: ${waveformBuffer.sampleRate}Hz → ${sampleRate}Hz`)
            }
            
            // 追加新数据到缓冲区并更新显示
            setWaveformBuffer(prev => {
              // 频率或采样率变化时清空
              const shouldReset = freqChanged || sampleRateChanged
              
              // 实时流数据：累积数据点
              // ECG模式(freq=1)保留3秒数据，其他模式保留较少
              const isEcgMode = freq === 1
              const maxPoints = isEcgMode ? 3000 : 500  // ECG:3秒@1000Hz, 其他:0.5秒
              
              let combinedInput, combinedOutput
              if (shouldReset || !prev.input || prev.input.length === 0) {
                combinedInput = newInput
                combinedOutput = newOutput
              } else {
                combinedInput = [...prev.input, ...newInput]
                combinedOutput = [...prev.output, ...newOutput]
              }
              
              // 保留最新的maxPoints个点
              const startIdx = combinedInput.length > maxPoints ? combinedInput.length - maxPoints : 0
              const trimmedInput = combinedInput.slice(startIdx)
              const trimmedOutput = combinedOutput.slice(startIdx)
              
              // 重新计算时间戳（从0开始，连续递增）
              const newTimeStamps = trimmedInput.map((_, index) => {
                return (index / sampleRate) * 1000  // ms
              })
              
              const newBuffer = {
                input: trimmedInput,
                output: trimmedOutput,
                timeStamps: newTimeStamps,
                freq,
                sampleRate
              }
              
              // 更新实时显示数据
              setWaveformData({
                input: newBuffer.input,
                output: newBuffer.output,
                timeStamps: newBuffer.timeStamps,
                freq: newBuffer.freq,
                sampleRate: newBuffer.sampleRate
              })
              
              // 保存到allWaveforms（按频率独立保存）
              setAllWaveforms(prev => {
                const newMap = new Map(prev)
                const existingData = newMap.get(freq)
                
                // 检查采样率是否变化
                const sampleRateChanged = existingData && existingData.sampleRate !== sampleRate
                
                if (existingData && !sampleRateChanged) {
                  // 采样率未变，累加同一频率的数据
                  const maxPoints = 1024
                  const combinedInput = [...existingData.input, ...newInput]
                  const combinedOutput = [...existingData.output, ...newOutput]
                  const combinedTimeStamps = [...existingData.timeStamps, ...newTimeStamps]
                  
                  const startIdx = combinedInput.length > maxPoints ? combinedInput.length - maxPoints : 0
                  
                  newMap.set(freq, {
                    input: combinedInput.slice(startIdx),
                    output: combinedOutput.slice(startIdx),
                    timeStamps: combinedTimeStamps.slice(startIdx),
                    freq,
                    sampleRate
                  })
                } else {
                  // 首次保存该频率 或 采样率变化时重置数据
                  if (sampleRateChanged) {
                    console.log(`${freq}Hz采样率变化: ${existingData.sampleRate}Hz → ${sampleRate}Hz，清空旧数据`)
                  }
                  newMap.set(freq, {
                    input: newInput,
                    output: newOutput,
                    timeStamps: newTimeStamps,
                    freq,
                    sampleRate
                  })
                }
                return newMap
              })
              
              return newBuffer
            })
            
            if (signalType !== 'ecg') {
              addLog(`波形数据: ${freq}Hz, ${newInput.length}点 (时长${dataLengthMs.toFixed(1)}ms)`, 'success')
            } else {
              // ECG模式日志减少频率
              // addLog(`ECG数据: ${newInput.length}点`, 'info')
            }
            
            // 返回更新后的时间偏移（总是返回0，下次从0开始）
            return 0
          })
        }
      } catch (e) {
        console.error('波形数据解析错误:', e)
      }
      return
    }
    
    // 如果是ECG模式，忽略FREQ_RESP数据（不生成Bode图）
    if (signalType === 'ecg') {
      return
    }
    
    // 解析 FREQ_RESP 格式（支持两种格式）
    // 格式1（未校准）: FREQ_RESP:freq,K,K1,H,theta
    // 格式2（已校准）: FREQ_RESP:freq,K,K1,H_raw,theta_raw,H_cal,theta_cal
    const parts = data.substring(10).split(',')  // 去掉"FREQ_RESP:"前缀
    
    if (parts.length < 5) return

    const freq = parseInt(parts[0], 10)
    const K = parseFloat(parts[1])
    const K1 = parseFloat(parts[2])
    
    let H, theta, H_raw, theta_raw, H_calibrated, theta_calibrated
    let isCalibrated = false
    
    if (parts.length >= 7) {
      // 校准格式：freq,K,K1,H_raw,theta_raw,H_cal,theta_cal
      H_raw = parseFloat(parts[3])
      theta_raw = parseFloat(parts[4])
      H_calibrated = parseFloat(parts[5])
      theta_calibrated = parseFloat(parts[6])
      H = H_calibrated  // 默认使用校准值
      theta = theta_calibrated
      isCalibrated = true
    } else {
      // 未校准格式：freq,K,K1,H,theta
      H = parseFloat(parts[3])
      theta = parseFloat(parts[4])
      H_raw = H
      theta_raw = theta
    }
    
    // 数据验证（更严格）
    if (!isFinite(freq) || !isFinite(K) || !isFinite(K1) || !isFinite(H) || !isFinite(theta)) {
      addLog(`数据无效(非有限数): freq=${freq}, K=${K}, K1=${K1}, H=${H}, theta=${theta}`, 'warning')
      return
    }
    
    if (freq < CONFIG.FREQ_MIN || freq > CONFIG.FREQ_MAX) {
      addLog(`频率超出范围: ${freq}Hz`, 'warning')
      return
    }
    
    if (H < CONFIG.H_MIN || H > CONFIG.H_MAX) {
      addLog(`幅频响应异常: ${H.toFixed(4)}`, 'warning')
      return
    }
    
    if (theta < CONFIG.THETA_MIN || theta > CONFIG.THETA_MAX) {
      addLog(`相频响应异常: ${theta.toFixed(2)}°`, 'warning')
      return
    }

    // 安全计算 dB 值，避免 log10(0) 或 log10(负数)
    const safeLog10 = (value) => {
      if (value <= 0) return -100  // 设置最小值为 -100dB
      return 20 * Math.log10(value)
    }
    
    const dataPoint = {
      freq: freq,
      omega: 2 * Math.PI * freq,
      K: K,
      K1: K1,
      H: H,
      H_dB: safeLog10(H),
      theta: theta,
      // 保存原始值和校准值（如果有）
      isCalibrated: isCalibrated,
      H_raw: H_raw,
      theta_raw: theta_raw,
      H_calibrated: isCalibrated ? H_calibrated : undefined,
      theta_calibrated: isCalibrated ? theta_calibrated : undefined,
      H_dB_raw: safeLog10(H_raw),
      H_dB_calibrated: isCalibrated ? safeLog10(H_calibrated) : undefined
    }

    setDataPoints(prev => {
      const existingIndex = prev.findIndex(p => p.freq === freq)
      
      if (existingIndex >= 0) {
        const newPoints = [...prev]
        newPoints[existingIndex] = dataPoint
        return newPoints
      } else {
        const newPoints = [...prev, dataPoint].sort((a, b) => a.freq - b.freq)
        
        if (newPoints.length > CONFIG.MAX_DATA_POINTS) {
          addLog(`数据点数超过${CONFIG.MAX_DATA_POINTS}，自动移除最旧数据`, 'warning')
          return newPoints.slice(1)
        }
        
        return newPoints
      }
    })

    if (isCalibrated) {
      addLog(`数据 [已校准]: f=${freq}Hz, H=${H_calibrated.toFixed(4)} (原始=${H_raw.toFixed(4)}), θ=${theta_calibrated.toFixed(2)}° (原始=${theta_raw.toFixed(2)}°)`, 'success')
    } else {
      addLog(`数据: f=${freq}Hz, H(ω)=${H.toFixed(4)}, θ(ω)=${theta.toFixed(2)}°`, 'success')
    }
  }, [])

  const clearDataPoints = useCallback(() => {
    setDataPoints([])
    setWaveformData(null)
    setAllWaveforms(new Map())
    setWaveformTimeOffset(0)
    setWaveformBuffer({
      input: [],
      output: [],
      timeStamps: [],
      freq: 0,
      sampleRate: 0
    })
  }, [])

  // 高级数据处理：Unwrapping + 异常值检测 + 自适应平滑
  const processedDataPoints = useMemo(() => {
    if (dataPoints.length < 2) return dataPoints
    
    // 如果用户关闭了处理，直接返回原始数据（无需额外处理）
    if (!enableProcessing) {
      return dataPoints
    }
    
    // 第1步：相位Unwrapping（修正±180°跳变）
    const unwrappedPoints = []
    let phaseOffset = 0
    
    for (let i = 0; i < dataPoints.length; i++) {
      const point = dataPoints[i]
      let unwrappedTheta = point.theta + phaseOffset
      
      if (i > 0) {
        const phaseDiff = point.theta - dataPoints[i-1].theta
        
        // 检测正向跳变（从-180°跳到+180°）
        if (phaseDiff > 270) {
          phaseOffset -= 360
          unwrappedTheta = point.theta + phaseOffset
        }
        // 检测反向跳变（从+180°跳到-180°）
        else if (phaseDiff < -270) {
          phaseOffset += 360
          unwrappedTheta = point.theta + phaseOffset
        }
      }
      
      unwrappedPoints.push({
        ...point,
        theta_unwrapped: unwrappedTheta,
        theta_raw: point.theta
      })
    }
    
    // 第2步：异常值检测和修正
    const cleanedPoints = unwrappedPoints.map((point, index) => {
      if (index === 0 || index === unwrappedPoints.length - 1) {
        return point
      }
      
      const prev = unwrappedPoints[index - 1]
      const next = unwrappedPoints[index + 1]
      const expected = (prev.theta_unwrapped + next.theta_unwrapped) / 2
      const deviation = Math.abs(point.theta_unwrapped - expected)
      
      // 如果偏差过大，使用线性插值修正
      if (deviation > CONFIG.PHASE_DEVIATION_THRESHOLD) {
        return {
          ...point,
          theta_corrected: expected,
          is_outlier: true
        }
      }
      
      return {
        ...point,
        theta_corrected: point.theta_unwrapped,
        is_outlier: false
      }
    })
    
    // 第3步：自适应平滑滤波
    const smoothedPoints = cleanedPoints.map((point, index) => {
      // 根据频率动态调整窗口大小
      const windowSize = point.freq < 100 
        ? CONFIG.SMOOTH_WINDOW_LOW_FREQ 
        : point.freq < 500 
          ? CONFIG.SMOOTH_WINDOW_MID_FREQ 
          : CONFIG.SMOOTH_WINDOW_HIGH_FREQ
      
      let sum = 0
      let count = 0
      
      const start = Math.max(0, index - Math.floor(windowSize / 2))
      const end = Math.min(cleanedPoints.length, index + Math.floor(windowSize / 2) + 1)
      
      for (let i = start; i < end; i++) {
        sum += cleanedPoints[i].theta_corrected
        count++
      }
      
      // 同时平滑幅频响应
      let sumH = 0
      for (let i = start; i < end; i++) {
        sumH += cleanedPoints[i].H
      }
      
      // 安全计算平滑后的 dB 值
      const avgH = count > 0 ? sumH / count : 0
      const H_dB_smooth = avgH > 0 ? 20 * Math.log10(avgH) : -100
      
      return {
        ...point,
        theta_smooth: count > 0 ? sum / count : point.theta_corrected,
        H_smooth: avgH,
        H_dB_smooth: H_dB_smooth
      }
    })
    
    return smoothedPoints
  }, [dataPoints, enableProcessing])

  const toggleProcessing = useCallback(() => {
    setEnableProcessing(prev => !prev)
  }, [])

  return {
    dataPoints: processedDataPoints,  // 返回处理后的数据
    waveformData,
    allWaveforms,  // 返回所有频率的波形数据（新增）
    addDataPoint,
    clearDataPoints,
    enableProcessing,
    toggleProcessing
  }
}


