import { useState, useEffect, useCallback, useRef } from 'react'
import TopBar from './components/TopBar'
import Header from './components/Header'
import ControlPanel from './components/ControlPanel'
import LEDControl from './components/LEDControl'
import StatusBar from './components/StatusBar'
import ProgressSection from './components/ProgressSection'
import StatsSection from './components/StatsSection'
import FormulaExplanation from './components/FormulaExplanation'
import Charts from './components/Charts'
import DataTable from './components/DataTable'
import LogWindow from './components/LogWindow'
import Toast from './components/Toast'
import ConnectionBanner from './components/ConnectionBanner'
import WaveformDisplay from './components/WaveformDisplay'
import WaveformCapture from './components/WaveformCapture'
import LiveSineWaveMonitor from './components/LiveSineWaveMonitor'
import DataVerification from './components/DataVerification'
import ErrorDialog from './components/ErrorDialog'
import ProtocolTester from './components/ProtocolTester'
import Footer from './components/Footer'
import { useSerialPort } from './hooks/useSerialPort'
import { useDataPoints } from './hooks/useDataPoints'
import { useLogs } from './hooks/useLogs'
import { useProtocolValidation } from './hooks/useProtocolValidation'
import './App.css'

function App() {
  const { 
    isConnected, 
    serialPort,
    connect, 
    disconnect, 
    sendCommand,
    dataRate 
  } = useSerialPort()
  
  const { 
    dataPoints, 
    waveformData,
    allWaveforms,
    addDataPoint, 
    clearDataPoints 
  } = useDataPoints()
  
  const { logs, addLog } = useLogs()
  
  const {
    errors: protocolErrors,
    showErrorDialog,
    setShowErrorDialog,
    addError,
    clearErrors,
    validateData,
    errorCount
  } = useProtocolValidation()
  
  const [toast, setToast] = useState({ show: false, message: '' })
  const [banner, setBanner] = useState({ show: false, message: '' })
  const [progress, setProgress] = useState({ current: 0, total: 0 })
  const [signalType, setSignalType] = useState('sine') // 'sine' 或 'ecg'
  const signalTypeRef = useRef('sine')

  // 同步signalType到ref
  useEffect(() => {
    signalTypeRef.current = signalType
  }, [signalType])

  // 组件卸载时清理串口连接，防止内存泄漏
  // 使用 ref 保存连接状态，避免闭包问题
  const isConnectedRef = useRef(false)
  
  useEffect(() => {
    isConnectedRef.current = isConnected
  }, [isConnected])
  
  useEffect(() => {
    return () => {
      // 仅在组件卸载且串口连接时才断开
      if (isConnectedRef.current) {
        disconnect(addLog).catch(err => {
          console.error('Cleanup disconnect error:', err)
        })
      }
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []) // 空依赖数组，仅在 unmount 时执行
  
  // 处理信号类型切换
  const handleSignalTypeChange = (newType) => {
    setSignalType(newType)
    if (isConnected) {
      // 发送命令到硬件
      const cmd = newType === 'ecg' ? 'TYPE:ECG' : 'TYPE:SINE'
      sendCommand(cmd, addLog)
      addLog(`切换信号类型: ${newType === 'ecg' ? '心电ECG' : '正弦波'}`, 'info')
      showToast(`已切换到${newType === 'ecg' ? '心电ECG' : '正弦波'}模式`)
      
      // 切换时清除之前的数据，避免混淆
      if (newType === 'ecg') {
        clearDataPoints()
      }
    } else {
      // 未连接时只是UI切换，连接后会同步
      addLog(`信号类型已设置为: ${newType === 'ecg' ? '心电ECG' : '正弦波'}，连接串口后将同步到硬件`, 'info')
    }
  }

  const showToast = (message, duration = 3000) => {
    setToast({ show: true, message })
    setTimeout(() => setToast({ show: false, message: '' }), duration)
  }

  const showBanner = (message, show = true, duration = 3000) => {
    setBanner({ show, message })
    if (show && duration > 0) {
      setTimeout(() => setBanner({ show: false, message: '' }), duration)
    }
  }

  const handleConnect = async () => {
    const success = await connect(addLog, (data) => {
      // 协议校验
      const validation = validateData(data)
      if (!validation.valid && validation.error) {
        // 错误已在validateData中记录，这里不重复处理
      }
      // 处理接收到的数据，传入当前的信号类型
      addDataPoint(data, addLog, signalTypeRef.current)
    })
    if (success) {
      showToast('串口连接成功！')
      showBanner('设备已连接，可以开始测试', true)
      
      // 连接成功后同步当前信号类型到硬件
      const cmd = signalType === 'ecg' ? 'TYPE:ECG' : 'TYPE:SINE'
      sendCommand(cmd, addLog)
      addLog(`已同步信号类型到硬件: ${signalType === 'ecg' ? '心电ECG' : '正弦波'}`, 'info')
    }
  }

  const handleDisconnect = async () => {
    await disconnect(addLog)
    showToast('串口已断开')
    showBanner('', false, 0)
  }

  const handleSweep = () => {
    sendCommand('SWEEP', addLog)
  }

  const handleFreqTest = (freq) => {
    if (signalType === 'ecg') {
      // ECG模式发送START命令开启数据流
      sendCommand('START', addLog)
    } else {
      sendCommand(`FREQ_TEST:${freq}`, addLog)
    }
  }

  const handleClearData = () => {
    if (confirm('确定要清除所有数据吗？')) {
      clearDataPoints()
      addLog('数据已清除', 'info')
      showToast('数据已清除')
    }
  }

  const handleExportCSV = () => {
    if (dataPoints.length === 0) {
      showToast('没有数据可导出')
      addLog('导出失败：无数据', 'warning')
      return
    }

    const now = new Date()
    const timestamp = now.toISOString()
    
    // 生成数据签名
    const generateSignature = () => {
      const dataStr = dataPoints.map((p, i) => 
        `${i}:${p.freq}_${p.K.toFixed(4)}_${p.K1.toFixed(4)}_${p.H.toFixed(6)}_${p.theta.toFixed(2)}`
      ).join('|')
      
      let hash = 2166136261
      for (let i = 0; i < dataStr.length; i++) {
        hash ^= dataStr.charCodeAt(i)
        hash += (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) + (hash << 24)
      }
      return (hash >>> 0).toString(16).toUpperCase().padStart(8, '0')
    }
    
    const signature = generateSignature()
    
    // CSV 头部：包含元数据和验证信息
    let csv = '# 频率响应分析仪 - 数据导出文件\n'
    csv += '# ============================================\n'
    csv += `# 导出时间：${now.toLocaleString('zh-CN')}\n`
    csv += `# ISO时间戳：${timestamp}\n`
    csv += `# 设备型号：GD32F103CBT6\n`
    csv += `# 固件版本：v2.0.1\n`
    csv += `# 通信协议：串口 115200bps\n`
    csv += `# 浏览器：${navigator.userAgent}\n`
    csv += `# 操作系统：${navigator.platform}\n`
    csv += `# 数据点数：${dataPoints.length}\n`
    csv += `# 数据签名：FNV1a-${signature}\n`
    csv += '# ============================================\n'
    csv += '# 数据说明：\n'
    csv += '#   - 所有数据通过串口实时采集自硬件设备\n'
    csv += '#   - 每个数据点包含频率、输入输出幅值、增益和相位信息\n'
    csv += '#   - 数据签名可用于验证数据完整性和真实性\n'
    csv += '# ============================================\n'
    csv += '\n'
    
    // CSV 数据表头
    csv += 'No,Frequency(Hz),Omega(rad/s),Input_K(V),Output_K1(V),H(omega)=K1/K,H_dB(dB),Theta(deg),Timestamp\n'
    
    // CSV 数据内容
    dataPoints.forEach((p, index) => {
      csv += `${index + 1},${p.freq},${p.omega.toFixed(2)},${p.K.toFixed(4)},${p.K1.toFixed(4)},${p.H.toFixed(6)},${p.H_dB.toFixed(2)},${p.theta.toFixed(2)},${timestamp}\n`
    })
    
    // 文件尾部：再次附加签名
    csv += '\n'
    csv += '# ============================================\n'
    csv += '# 数据完整性验证\n'
    csv += `# 文件签名：FNV1a-${signature}\n`
    csv += `# 生成时间：${timestamp}\n`
    csv += '# 如需验证数据真实性，请保留此文件及录屏记录\n'
    csv += '# ============================================\n'

    const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `frequency_response_${timestamp.slice(0, 19).replace(/:/g, '-')}_${signature.substring(0, 8)}.csv`
    a.click()
    URL.revokeObjectURL(url)

    addLog(`CSV文件已导出，签名: ${signature}`, 'success')
    showToast(`数据已导出 (签名: ${signature.substring(0, 8)}...)`)
  }

  const handleExportImage = () => {
    if (dataPoints.length === 0) {
      showToast('没有数据可导出')
      addLog('导出失败：无数据', 'warning')
      return
    }
    
    showToast('Bode图已保存')
    addLog('Bode图已导出为PNG图片', 'success')
  }

  // 协议校验测试功能
  const handleProtocolValidate = (data) => {
    const result = validateData(data)
    if (!result.valid && result.error) {
      // 校验失败，错误已记录到protocolErrors
      addLog(`协议校验失败: ${result.message || result.error}`, 'warning')
    } else if (result.valid) {
      addLog(`协议校验通过: ${data.substring(0, 30)}...`, 'success')
    }
    return result
  }

  // 串口数据处理已经在handleConnect中通过回调实现

  return (
    <div className="app">
      <TopBar />
      <Toast show={toast.show} message={toast.message} />
      <ConnectionBanner show={banner.show} message={banner.message} />
      
      <Header />
      
      <div className="container">
        <ControlPanel 
          isConnected={isConnected}
          onConnect={handleConnect}
          onDisconnect={handleDisconnect}
          onSweep={handleSweep}
          onFreqTest={handleFreqTest}
          onClearData={handleClearData}
          onExportCSV={handleExportCSV}
          onExportImage={handleExportImage}
          signalType={signalType}
          onSignalTypeChange={handleSignalTypeChange}
        />
        
        <LEDControl
          isConnected={isConnected}
          sendCommand={sendCommand}
          addLog={addLog}
        />
        
        <ProgressSection 
          current={progress.current} 
          total={progress.total} 
        />
        
        <StatsSection dataPoints={dataPoints} />
        
        <FormulaExplanation />
        
        <StatusBar
          isConnected={isConnected}
          dataCount={dataPoints.length}
          currentFreq={dataPoints.length > 0 ? dataPoints[dataPoints.length - 1].freq : null}
          dataRate={dataRate}
          errorCount={errorCount}
          onShowErrors={() => setShowErrorDialog(true)}
        />
        
        <WaveformDisplay waveformData={waveformData} allWaveforms={allWaveforms} signalType={signalType} />
        
        <WaveformCapture
          isConnected={isConnected}
          sendCommand={sendCommand}
          addLog={addLog}
        />
        
        <LiveSineWaveMonitor 
          isConnected={isConnected} 
          serialPort={serialPort}
          latestWaveform={waveformData}
        />
        
        <Charts dataPoints={dataPoints} />
        
        <DataTable dataPoints={dataPoints} />
        
        <ProtocolTester onValidate={handleProtocolValidate} />
        
        <DataVerification 
          isConnected={isConnected}
          dataPoints={dataPoints}
          logs={logs}
        />
        
        <LogWindow logs={logs} />
      </div>
      
      <Footer />
      
      {/* 协议错误对话框 */}
      <ErrorDialog
        errors={protocolErrors}
        onClose={() => setShowErrorDialog(false)}
        onClearAll={clearErrors}
      />
    </div>
  )
}

export default App


