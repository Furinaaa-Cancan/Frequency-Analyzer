import '../styles/Footer.css'

function Footer() {
  const currentYear = new Date().getFullYear()
  
  return (
    <footer className="footer">
      <div className="footer-content">
        <div className="footer-section">
          <h4>FreqAnalyzer Pro</h4>
          <p className="footer-desc">
            专业的频率响应分析系统<br/>
            基于GD32F103微控制器的Bode图测量解决方案
          </p>
          <div className="footer-badges">
            <span className="badge">GD32F103</span>
            <span className="badge">ADC双通道</span>
            <span className="badge">实时分析</span>
          </div>
        </div>

        <div className="footer-section">
          <h4>产品特性</h4>
          <ul className="footer-links">
            <li><a href="#features">自动频率扫描</a></li>
            <li><a href="#features">实时波形显示</a></li>
            <li><a href="#features">Bode图绘制</a></li>
            <li><a href="#features">数据导出</a></li>
          </ul>
        </div>

        <div className="footer-section">
          <h4>资源</h4>
          <ul className="footer-links">
            <li><a href="#docs">使用文档</a></li>
            <li><a href="#api">API参考</a></li>
            <li><a href="#examples">示例代码</a></li>
            <li><a href="#faq">常见问题</a></li>
          </ul>
        </div>

        <div className="footer-section">
          <h4>开发者</h4>
          <ul className="footer-links">
            <li>
              <a href="https://github.com" target="_blank" rel="noopener noreferrer">
                GitHub仓库
              </a>
            </li>
            <li>
              <a href="#changelog">
                更新日志
              </a>
            </li>
            <li>
              <a href="#contribute">
                贡献指南
              </a>
            </li>
            <li>
              <a href="#license">
                开源协议
              </a>
            </li>
          </ul>
        </div>

        <div className="footer-section">
          <h4>联系我们</h4>
          <ul className="footer-links">
            <li>
              <a href="mailto:support@freqanalyzer.com">
                support@freqanalyzer.com
              </a>
            </li>
            <li>
              <a href="tel:+86-xxx-xxxx-xxxx">
                +86 xxx-xxxx-xxxx
              </a>
            </li>
            <li>
              <a href="#wechat">
                微信客服
              </a>
            </li>
          </ul>
        </div>
      </div>

      <div className="footer-bottom">
        <div className="footer-bottom-left">
          <p>© {currentYear} FreqAnalyzer Pro. All rights reserved.</p>
          <div className="footer-legal">
            <a href="#privacy">隐私政策</a>
            <span className="separator">|</span>
            <a href="#terms">使用条款</a>
            <span className="separator">|</span>
            <a href="#cookies">Cookie政策</a>
          </div>
        </div>
        <div className="footer-bottom-right">
          <p className="footer-tech">
            Powered by <strong>React</strong> + <strong>GD32F103</strong> + <strong>Chart.js</strong>
          </p>
          <div className="footer-social">
            <a href="https://github.com" target="_blank" rel="noopener noreferrer" className="social-link" title="GitHub">
              GitHub
            </a>
            <a href="#twitter" className="social-link" title="Twitter">
              Twitter
            </a>
            <a href="#wechat" className="social-link" title="微信">
              微信
            </a>
            <a href="#qq" className="social-link" title="QQ">
              QQ
            </a>
          </div>
        </div>
      </div>
    </footer>
  )
}

export default Footer
