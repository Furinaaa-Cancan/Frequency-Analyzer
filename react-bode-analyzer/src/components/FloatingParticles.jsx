import { useEffect, useRef } from 'react'
import '../styles/FloatingParticles.css'

const FloatingParticles = ({ 
  count = 30, 
  color = '#c9a97a', 
  minSize = 2,
  maxSize = 5,
  speed = 0.5,
  opacity = 0.4
}) => {
  const canvasRef = useRef(null)

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas) return

    const ctx = canvas.getContext('2d')
    const particles = []

    const resize = () => {
      canvas.width = canvas.offsetWidth
      canvas.height = canvas.offsetHeight
    }
    resize()
    window.addEventListener('resize', resize)

    // 创建粒子
    for (let i = 0; i < count; i++) {
      particles.push({
        x: Math.random() * canvas.width,
        y: Math.random() * canvas.height,
        size: minSize + Math.random() * (maxSize - minSize),
        vx: (Math.random() - 0.5) * speed,
        vy: (Math.random() - 0.5) * speed,
        alpha: opacity * (0.5 + Math.random() * 0.5)
      })
    }

    const animate = () => {
      ctx.clearRect(0, 0, canvas.width, canvas.height)

      particles.forEach(p => {
        // 更新位置
        p.x += p.vx
        p.y += p.vy

        // 边界检查
        if (p.x < 0 || p.x > canvas.width) p.vx *= -1
        if (p.y < 0 || p.y > canvas.height) p.vy *= -1

        // 绘制粒子
        ctx.fillStyle = `${color}${Math.floor(p.alpha * 255).toString(16).padStart(2, '0')}`
        ctx.beginPath()
        ctx.arc(p.x, p.y, p.size, 0, Math.PI * 2)
        ctx.fill()

        // 连接附近的粒子
        particles.forEach(p2 => {
          const dx = p.x - p2.x
          const dy = p.y - p2.y
          const distance = Math.sqrt(dx * dx + dy * dy)

          if (distance < 100) {
            const lineOpacity = opacity * (1 - distance / 100) * 0.3
            ctx.strokeStyle = `${color}${Math.floor(lineOpacity * 255).toString(16).padStart(2, '0')}`
            ctx.lineWidth = 0.5
            ctx.beginPath()
            ctx.moveTo(p.x, p.y)
            ctx.lineTo(p2.x, p2.y)
            ctx.stroke()
          }
        })
      })

      requestAnimationFrame(animate)
    }

    animate()

    return () => {
      window.removeEventListener('resize', resize)
    }
  }, [count, color, minSize, maxSize, speed, opacity])

  return <canvas ref={canvasRef} className="floating-particles-canvas" />
}

export default FloatingParticles
