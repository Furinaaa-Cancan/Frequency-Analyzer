import '../styles/ProjectSummary.css'

function ProjectSummary() {
  return (
    <div className="project-summary">
      <div className="summary-header">
        <h2>课题二：频率响应自动测量与计算</h2>
        <p className="summary-subtitle">基于GD32F103的Bode图绘制与频率特性分析系统</p>
      </div>

      <div className="summary-content">
        <div className="summary-section">
          <h3>项目概述</h3>
          <p>
            本系统通过ADC双通道同步采样技术，自动测量待测电路系统两端的正弦信号，
            实现频率响应的自动化测量与Bode图绘制。系统采用GD32F103微控制器为核心，
            配合DAC5311数模转换器和双通道ADC，构建完整的频率响应测试平台。
          </p>
        </div>

        <div className="summary-grid">
          <div className="summary-card">
            <div className="card-number">01</div>
            <h4>核心功能</h4>
            <ul>
              <li><strong>自动频率扫描</strong>：10Hz - 1000Hz范围内自动测量</li>
              <li><strong>双通道同步采样</strong>：ADC0/ADC1同步采集输入输出信号</li>
              <li><strong>实时波形显示</strong>：时域波形与频域特性同步呈现</li>
              <li><strong>Bode图绘制</strong>：自动计算并绘制幅频、相频特性曲线</li>
              <li><strong>数据导出</strong>：支持CSV格式数据导出和图表保存</li>
            </ul>
          </div>

          <div className="summary-card">
            <div className="card-number">02</div>
            <h4>技术特点</h4>
            <ul>
              <li><strong>高精度测量</strong>：12位ADC，采样率最高20kHz</li>
              <li><strong>DDS信号生成</strong>：基于查表法的高质量正弦波输出</li>
              <li><strong>DMA传输</strong>：高速数据传输，减轻CPU负担</li>
              <li><strong>实时计算</strong>：FFT频谱分析，精确计算幅值比和相位差</li>
              <li><strong>Web界面</strong>：基于React的现代化用户交互界面</li>
            </ul>
          </div>

          <div className="summary-card">
            <div className="card-number">03</div>
            <h4>系统架构</h4>
            <ul>
              <li><strong>信号生成模块</strong>：DDS + DAC5311 → PB1输出</li>
              <li><strong>信号采集模块</strong>：PA6/PB0双通道ADC同步采样</li>
              <li><strong>数据处理模块</strong>：FFT分析、幅频相频计算</li>
              <li><strong>通信模块</strong>：USART串口，115200波特率</li>
              <li><strong>显示模块</strong>：Web界面实时图表展示</li>
            </ul>
          </div>

          <div className="summary-card">
            <div className="card-number">04</div>
            <h4>测量参数</h4>
            <ul>
              <li><strong>频率范围</strong>：10 Hz - 1000 Hz</li>
              <li><strong>采样率</strong>：20000 Hz（可调）</li>
              <li><strong>采样点数</strong>：1024点/频率点</li>
              <li><strong>幅值精度</strong>：12位 (0-4095)</li>
              <li><strong>相位精度</strong>：0.1° (FFT计算)</li>
            </ul>
          </div>
        </div>

        <div className="summary-section features-section">
          <h3>创新点与优势</h3>
          <div className="features-list">
            <div className="feature-item">
              <div className="feature-number">01</div>
              <div className="feature-content">
                <h5>全自动测量流程</h5>
                <p>一键启动，自动完成频率扫描、数据采集、波形分析和图表绘制全流程</p>
              </div>
            </div>
            <div className="feature-item">
              <div className="feature-number">02</div>
              <div className="feature-content">
                <h5>实时可视化</h5>
                <p>时域波形与频域特性同步显示，直观展现系统频率响应特性</p>
              </div>
            </div>
            <div className="feature-item">
              <div className="feature-number">03</div>
              <div className="feature-content">
                <h5>高精度同步采样</h5>
                <p>采用DMA+双ADC同步模式，确保输入输出信号的时间对齐</p>
              </div>
            </div>
            <div className="feature-item">
              <div className="feature-number">04</div>
              <div className="feature-content">
                <h5>Web交互界面</h5>
                <p>现代化的React前端界面，支持图表缩放、数据导出等丰富功能</p>
              </div>
            </div>
          </div>
        </div>

        <div className="summary-section">
          <h3>应用场景</h3>
          <div className="applications">
            <div className="app-item">
              <strong>滤波器特性测试</strong>
              <p>RC、LC、有源滤波器的频率响应曲线测量</p>
            </div>
            <div className="app-item">
              <strong>放大器频率特性</strong>
              <p>运算放大器、功率放大器的增益-频率特性分析</p>
            </div>
            <div className="app-item">
              <strong>控制系统分析</strong>
              <p>闭环控制系统的开环传递函数测试</p>
            </div>
            <div className="app-item">
              <strong>教学实验</strong>
              <p>自动控制原理、信号与系统等课程实验</p>
            </div>
          </div>
        </div>

        <div className="summary-footer">
          <div className="tech-stack">
            <h4>技术栈</h4>
            <div className="tech-tags">
              <span className="tech-tag">GD32F103</span>
              <span className="tech-tag">ADC双通道</span>
              <span className="tech-tag">DMA传输</span>
              <span className="tech-tag">DAC5311</span>
              <span className="tech-tag">DDS算法</span>
              <span className="tech-tag">FFT分析</span>
              <span className="tech-tag">USART通信</span>
              <span className="tech-tag">React.js</span>
              <span className="tech-tag">Chart.js</span>
              <span className="tech-tag">Web Serial API</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  )
}

export default ProjectSummary
