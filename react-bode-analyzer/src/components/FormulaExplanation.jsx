import { useState, useEffect, useRef } from 'react'
import '../styles/FormulaExplanation.css'
import GridPattern from './GridPattern'
import FloatingParticles from './FloatingParticles'

// 简单的动画背景组件
const AnimatedBanner = () => {
  const canvasRef = useRef(null)
  
  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas) return
    
    const ctx = canvas.getContext('2d')
    const particles = []
    
    // 设置canvas大小
    const resize = () => {
      canvas.width = canvas.offsetWidth
      canvas.height = canvas.offsetHeight
    }
    resize()
    window.addEventListener('resize', resize)
    
    // 创建粒子
    for (let i = 0; i < 50; i++) {
      particles.push({
        x: Math.random() * canvas.width,
        y: Math.random() * canvas.height,
        vx: (Math.random() - 0.5) * 0.5,
        vy: (Math.random() - 0.5) * 0.5,
        size: Math.random() * 2 + 1
      })
    }
    
    // 动画循环
    const animate = () => {
      ctx.fillStyle = 'rgba(74, 63, 53, 0.15)'
      ctx.fillRect(0, 0, canvas.width, canvas.height)
      
      particles.forEach(p => {
        p.x += p.vx
        p.y += p.vy
        
        if (p.x < 0 || p.x > canvas.width) p.vx *= -1
        if (p.y < 0 || p.y > canvas.height) p.vy *= -1
        
        ctx.fillStyle = 'rgba(237, 232, 223, 0.8)'
        ctx.beginPath()
        ctx.arc(p.x, p.y, p.size, 0, Math.PI * 2)
        ctx.fill()
      })
      
      requestAnimationFrame(animate)
    }
    animate()
    
    return () => {
      window.removeEventListener('resize', resize)
    }
  }, [])
  
  return <canvas ref={canvasRef} className="animated-banner-canvas" />
}

function FormulaExplanation() {
  const [expandedSection, setExpandedSection] = useState('principle')
  
  const toggleSection = (section) => {
    setExpandedSection(expandedSection === section ? null : section)
  }

  return (
    <div className="formula-explanation">
      <div className="explanation-header fade-in">
        <h3>课题二  频率响应自动测量与计算</h3>
        <p className="subtitle">通过ADC双通道同步采样，自动测量待测电路系统两端正弦信号</p>
      </div>

      <div className="formula-content">
        {/* 测量原理 */}
        <div className="section-card slide-in" style={{ animationDelay: '0.1s', position: 'relative' }}>
          <GridPattern 
            gridSize={50} 
            lineColor="#c9a97a" 
            lineWidth={0.5}
            opacity={0.1}
            animated={true}
          />
          <div 
            className="card-header" 
            onClick={() => toggleSection('principle')}
          >
            <h4>测量原理</h4>
            <span className={`toggle-icon ${expandedSection === 'principle' ? 'expanded' : ''}`}>
              ▼
            </span>
          </div>
          <div className={`card-content ${expandedSection === 'principle' ? 'expanded' : 'collapsed'}`}>
            <div className="signal-box input-signal">
              <strong>ADC通道0 (PA6)</strong> → 输入信号 K sin(ωt)
              <div className="signal-desc">采集待测电路输入端正弦波，使用RMS能量法计算幅度K</div>
            </div>
            <div className="signal-box output-signal">
              <strong>ADC通道1 (PB1)</strong> → 输出信号 K₁ sin(ωt + θ)
              <div className="signal-desc">采集待测电路输出端正弦波，使用RMS能量法计算幅度K₁，DFT法计算相位差θ</div>
            </div>
          </div>
        </div>

        {/* 算法实现 */}
        <div className="section-card slide-in" style={{ animationDelay: '0.2s', position: 'relative' }}>
          <FloatingParticles 
            count={25} 
            color="#c9a97a" 
            minSize={1.5}
            maxSize={3}
            speed={0.3}
            opacity={0.3}
          />
          <div 
            className="card-header" 
            onClick={() => toggleSection('algorithm')}
          >
            <h4>算法实现</h4>
            <span className={`toggle-icon ${expandedSection === 'algorithm' ? 'expanded' : ''}`}>
              ▼
            </span>
          </div>
          <div className={`card-content ${expandedSection === 'algorithm' ? 'expanded' : 'collapsed'}`}>
            
            <div className="algorithm-section">
              <h5>1. 幅度提取 - RMS能量法</h5>
              <div className="algorithm-box">
                <div className="code-block">
                  <code>float CalculateAmplitude_DFT(uint16_t *signal, ...)</code>
                  <ul>
                    <li><strong>去直流偏移</strong>：计算512个采样点的平均值并减去</li>
                    <li><strong>计算能量</strong>：累加所有采样点的平方和 Σ(x²)</li>
                    <li><strong>计算RMS</strong>：RMS = √(Σ(x²) / N)</li>
                    <li><strong>转换峰值</strong>：幅度 = RMS × √2</li>
                    <li><strong>转换电压</strong>：voltage_mv = (幅度 × 3300) / 4096</li>
                  </ul>
                  <div className="formula-display" style={{ marginTop: '10px' }}>
                    <span className="math-large">A = √(Σx² / N) × √2</span>
                  </div>
                </div>
              </div>
            </div>

            <div className="algorithm-section">
              <h5>2. 相位差计算 - DFT方法</h5>
              <div className="algorithm-box">
                <div className="code-block">
                  <code>int32_t EstimatePhaseShift_Int(...)</code>
                  <ul>
                    <li>去除直流偏移：计算平均值并减去</li>
                    <li>对两路信号分别进行DFT变换，计算sin/cos分量</li>
                    <li>使用atan2f函数计算各自的相位角</li>
                    <li>两相位角相减得到相位差：θ = phase2 - phase1</li>
                  </ul>
                </div>
                <div className="formula-display">
                  <span className="math-large">θ = atan2(sin_sum, cos_sum)</span>
                </div>
              </div>
            </div>

            <div className="algorithm-section">
              <h5>3. 频率响应计算</h5>
              <div className="calculation-box">
                <div className="calc-item">
                  <strong>幅频响应</strong>
                  <div className="formula-display">
                    <span className="math-large">H(ω) = K₁ / K</span>
                  </div>
                  <p className="calc-desc">输出幅度除以输入幅度（RMS能量法），代码实现：H = amp_ch2 / amp_ch1</p>
                </div>
                <div className="calc-item">
                  <strong>相频响应</strong>
                  <div className="formula-display">
                    <span className="math-large">θ(ω)</span>
                  </div>
                  <p className="calc-desc">通过DFT+atan2计算得到的相位差（单位：度）</p>
                </div>
              </div>
            </div>
          </div>
        </div>

        {/* 测试流程 */}
        <div className="section-card slide-in" style={{ animationDelay: '0.3s' }}>
          <div 
            className="card-header" 
            onClick={() => toggleSection('workflow')}
          >
            <h4>自动扫频流程</h4>
            <span className={`toggle-icon ${expandedSection === 'workflow' ? 'expanded' : ''}`}>
              ▼
            </span>
          </div>
          <div className={`card-content ${expandedSection === 'workflow' ? 'expanded' : 'collapsed'}`}>
            <div className="workflow-steps">
              <div className="workflow-item">
                <span className="step-badge">1</span>
                <span className="step-text">初始化：UART(115200)、DAC5311(SPI)、DDS、ADC(DMA模式)</span>
              </div>
              <div className="workflow-item">
                <span className="step-badge">2</span>
                <span className="step-text">扫频循环：10Hz → 2000Hz，步进10Hz</span>
              </div>
              <div className="workflow-item">
                <span className="step-badge">3</span>
                <span className="step-text">每个频率点：DDS设置频率 → 延时稳定 → ADC采集512点</span>
              </div>
              <div className="workflow-item">
                <span className="step-badge">4</span>
                <span className="step-text">数据处理：提取幅度 → 计算相位 → 计算H(ω)和θ(ω)</span>
              </div>
              <div className="workflow-item">
                <span className="step-badge">5</span>
                <span className="step-text">UART输出：FREQ_RESP:频率,K,K₁,H,θ</span>
              </div>
              <div className="workflow-item">
                <span className="step-badge">6</span>
                <span className="step-text">Web接收：解析数据 → 更新Bode图</span>
              </div>
            </div>
          </div>
        </div>

        {/* 技术参数 */}
        <div className="section-card slide-in" style={{ animationDelay: '0.4s' }}>
          <div 
            className="card-header" 
            onClick={() => toggleSection('specs')}
          >
            <h4>技术参数</h4>
            <span className={`toggle-icon ${expandedSection === 'specs' ? 'expanded' : ''}`}>
              ▼
            </span>
          </div>
          <div className={`card-content ${expandedSection === 'specs' ? 'expanded' : 'collapsed'}`}>
            <div className="info-grid">
              <div className="info-item">
                <span className="label">MCU</span>
                <span className="value">GD32F103CBT6 @ 72MHz</span>
              </div>
              <div className="info-item">
                <span className="label">ADC</span>
                <span className="value">12位 / 20kHz采样率 / DMA传输</span>
              </div>
              <div className="info-item">
                <span className="label">信号源</span>
                <span className="value">DDS算法 + DAC5311(SPI)</span>
              </div>
              <div className="info-item">
                <span className="label">扫频范围</span>
                <span className="value">10Hz ~ 1000Hz (100点)</span>
              </div>
              <div className="info-item">
                <span className="label">采样点数</span>
                <span className="value">512点/频率点</span>
              </div>
              <div className="info-item">
                <span className="label">通信</span>
                <span className="value">UART 115200bps</span>
              </div>
            </div>
          </div>
        </div>

        {/* 底部总结 */}
        <div className="footer-banner">
          <AnimatedBanner />
          <div className="banner-text">
            <strong>完整实现课题要求</strong>
            <p>ADC双通道同步采样 · RMS能量法幅度计算 · DFT相位计算 · 自动扫频测量 · Bode图实时绘制</p>
          </div>
        </div>
      </div>
    </div>
  )
}

export default FormulaExplanation


