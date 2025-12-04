import '../styles/Header.css'
import DotPattern from './DotPattern'
import ScrollVelocity from './ScrollVelocity'

function Header() {
  const text = "频率响应分析仪"
  
  return (
    <>
      <div className="header">
        <DotPattern 
          dotSize={1.2} 
          dotColor="#c9a97a" 
          spacing={35} 
          opacity={0.25}
          animated={true}
        />
        <h1>
          {text.split('').map((char, index) => (
            <span 
              key={index} 
              className="char-animate" 
              style={{ '--char-index': index }}
            >
              {char}
            </span>
          ))}
        </h1>
        <p className="header-subtitle">GD32F103 ADC双通道采样 | Bode图实时绘制 | 专业级测量解决方案</p>
        <div className="header-features">
          <span className="feature-tag">10Hz-1000Hz全频段</span>
          <span className="feature-tag">20kHz高速采样</span>
          <span className="feature-tag">FFT频谱分析</span>
          <span className="feature-tag">实时波形显示</span>
        </div>
        <div className="header-badge">PROFESSIONAL ANALYSIS TOOL</div>
      </div>
      <div className="header-scroll-container">
        <ScrollVelocity
          texts={['  GD32F103  •  ADC双通道  •  DMA传输  •  DAC5311  •  DDS算法  •  FFT分析  •  USART通信  •  Bode图绘制  ']} 
          velocity={18} 
          numCopies={5}
        />
      </div>
    </>
  )
}

export default Header


