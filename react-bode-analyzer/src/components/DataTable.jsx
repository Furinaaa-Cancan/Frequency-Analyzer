import { useState } from 'react'
import '../styles/DataTable.css'

function DataTable({ dataPoints }) {
  const [isOpen, setIsOpen] = useState(false)

  return (
    <>
      <div className="section-toggle" onClick={() => setIsOpen(!isOpen)}>
        <span>数据表格</span>
        <span className={`toggle-icon ${isOpen ? 'open' : ''}`}>▼</span>
      </div>
      {isOpen && (
        <div className="data-table-container">
          <table className="data-table">
            <thead>
              <tr>
                <th>序号</th>
                <th>频率 f (Hz)</th>
                <th>角频率 ω (rad/s)</th>
                <th>输入幅度 K</th>
                <th>输出幅度 K₁</th>
                <th>幅频响应 H(ω)=K₁/K</th>
                <th>幅频响应 (dB)</th>
                <th>相频响应 θ(ω) (°)</th>
              </tr>
            </thead>
            <tbody>
              {dataPoints.length === 0 ? (
                <tr>
                  <td colSpan="8" style={{ color: '#999', padding: '40px' }}>
                    暂无数据，请连接串口并开始测试
                  </td>
                </tr>
              ) : (
                dataPoints.map((p, index) => (
                  <tr key={index}>
                    <td>{index + 1}</td>
                    <td>{p.freq}</td>
                    <td>{p.omega.toFixed(2)}</td>
                    <td>{p.K.toFixed(2)}</td>
                    <td>{p.K1.toFixed(2)}</td>
                    <td>
                      {p.H.toFixed(4)}{' '}
                      <span style={{ color: '#999', fontSize: '11px' }}>
                        ({p.K1.toFixed(2)}/{p.K.toFixed(2)})
                      </span>
                    </td>
                    <td>{p.H_dB.toFixed(2)}</td>
                    <td>{p.theta.toFixed(2)}</td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      )}
    </>
  )
}

export default DataTable


