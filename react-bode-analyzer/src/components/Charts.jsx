import { useEffect, useRef } from 'react'
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
} from 'chart.js'
import { Line } from 'react-chartjs-2'
import '../styles/Charts.css'
import DotPattern from './DotPattern'

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
)

function Charts({ dataPoints }) {
  const frequencies = dataPoints.map(p => p.freq)
  
  // 检查是否有校准数据
  const hasCalibration = dataPoints.some(p => p.isCalibrated)
  
  // 只使用原始测量值，不做任何处理
  const magnitudes = dataPoints.map(p => p.H)  // 直接使用原始H(ω)
  
  // 原始相位数据（不做unwrap，不做平滑）
  const phases = dataPoints.map(p => p.theta)  // 直接使用原始θ(ω)
  
  const magnitudeData = {
    labels: frequencies,
    datasets: [
      // 只显示一条原始数据线
      {
        label: 'H(ω) - 原始测量值',
        data: magnitudes,
        borderColor: 'rgb(75, 192, 192)',
        backgroundColor: 'rgba(75, 192, 192, 0.1)',
        borderWidth: 2,
        tension: 0.1,  // 少量平滑以便观察
        pointRadius: 4,
        pointHoverRadius: 6
      }
    ]
  }

  const phaseData = {
    labels: frequencies,
    datasets: [
      // 只显示一条原始相位数据线
      {
        label: 'θ(ω) - 原始测量值',
        data: phases,
        borderColor: 'rgb(255, 99, 132)',
        backgroundColor: 'rgba(255, 99, 132, 0.1)',
        borderWidth: 2,
        tension: 0.1,  // 少量平滑以便观察
        pointRadius: 4,
        pointHoverRadius: 6
      }
    ]
  }

  const magnitudeOptions = {
    responsive: true,
    maintainAspectRatio: false,
    scales: {
      x: {
        title: {
          display: true,
          text: '频率 f (Hz)',
          font: { size: 14, weight: 'bold' }
        },
        ticks: { font: { size: 11 } }
      },
      y: {
        title: {
          display: true,
          text: 'H(ω) = K₁/K',
          font: { size: 14, weight: 'bold' }
        },
        beginAtZero: true,
        // 自动缩放，不设置max，让Chart.js根据数据自动调整
        ticks: { font: { size: 11 } }
      }
    },
    plugins: {
      legend: { display: true, position: 'top' },
      tooltip: {
        callbacks: {
          label: function(context) {
            let label = context.dataset.label || ''
            if (label) label += ': '
            label += context.parsed.y.toFixed(4)
            return label
          }
        }
      }
    }
  }

  const phaseOptions = {
    responsive: true,
    maintainAspectRatio: false,
    scales: {
      x: {
        title: {
          display: true,
          text: '频率 f (Hz)',
          font: { size: 14, weight: 'bold' }
        },
        ticks: { font: { size: 11 } }
      },
      y: {
        title: {
          display: true,
          text: 'θ(ω) (度)',
          font: { size: 14, weight: 'bold' }
        },
        ticks: { font: { size: 11 } }
      }
    },
    plugins: {
      legend: { display: true, position: 'top' },
      tooltip: {
        callbacks: {
          label: function(context) {
            let label = context.dataset.label || ''
            if (label) label += ': '
            label += context.parsed.y.toFixed(2) + '°'
            return label
          }
        }
      }
    }
  }

  return (
    <div className="charts-container">
      <div className="chart-wrapper" style={{ position: 'relative' }}>
        <DotPattern 
          dotSize={1} 
          dotColor="#c9a97a" 
          spacing={40} 
          opacity={0.15}
          animated={false}
        />
        <div className="chart-title" style={{ position: 'relative', zIndex: 1 }}>
          幅频特性曲线 H(ω) - 原始测量值
        </div>
        <div className="chart-description" style={{ position: 'relative', zIndex: 1 }}>
          测量方法：通过ADC双通道同步采样，计算输入PA6和输出PB1的幅值比H(ω)=K₁/K。采用RMS能量法计算幅值，每个频点自适应采样512点，确保低频有足够周期数。
        </div>
        <div className="chart-canvas" style={{ position: 'relative', zIndex: 1 }}>
          <Line data={magnitudeData} options={magnitudeOptions} />
        </div>
      </div>
      <div className="chart-wrapper" style={{ position: 'relative' }}>
        <DotPattern 
          dotSize={1} 
          dotColor="#c9a97a" 
          spacing={40} 
          opacity={0.15}
          animated={false}
        />
        <div className="chart-title" style={{ position: 'relative', zIndex: 1 }}>
          相频特性曲线 θ(ω) - 原始测量值
        </div>
        <div className="chart-description" style={{ position: 'relative', zIndex: 1 }}>
          测量方法：采用改进DFT相位检测算法，通过FFT分析基波分量的相位差θ(ω)。使用phase unwrapping算法消除±180°跳变，确保相位曲线连续。
        </div>
        <div className="chart-canvas" style={{ position: 'relative', zIndex: 1 }}>
          <Line data={phaseData} options={phaseOptions} />
        </div>
      </div>
    </div>
  )
}

export default Charts


