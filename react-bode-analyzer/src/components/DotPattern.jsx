import { useEffect, useRef } from 'react'
import '../styles/DotPattern.css'

const DotPattern = ({ 
  dotSize = 1.5, 
  dotColor = '#c9a97a', 
  spacing = 30,
  opacity = 0.3,
  animated = true 
}) => {
  const canvasRef = useRef(null)

  useEffect(() => {
    const canvas = canvasRef.current
    if (!canvas) return

    const ctx = canvas.getContext('2d')
    let animationId

    const resize = () => {
      canvas.width = canvas.offsetWidth
      canvas.height = canvas.offsetHeight
    }
    resize()
    window.addEventListener('resize', resize)

    let offset = 0

    const draw = () => {
      ctx.clearRect(0, 0, canvas.width, canvas.height)

      const rows = Math.ceil(canvas.height / spacing) + 1
      const cols = Math.ceil(canvas.width / spacing) + 1

      for (let row = 0; row < rows; row++) {
        for (let col = 0; col < cols; col++) {
          const x = col * spacing
          const y = row * spacing

          // 动画波纹效果
          const distance = Math.sqrt(
            Math.pow(x - canvas.width / 2, 2) + 
            Math.pow(y - canvas.height / 2, 2)
          )
          const wave = animated ? Math.sin((distance / 100) - offset) : 0
          const currentOpacity = opacity * (0.5 + wave * 0.5)

          ctx.fillStyle = `${dotColor}${Math.floor(currentOpacity * 255).toString(16).padStart(2, '0')}`
          ctx.beginPath()
          ctx.arc(x, y, dotSize, 0, Math.PI * 2)
          ctx.fill()
        }
      }

      if (animated) {
        offset += 0.02
        animationId = requestAnimationFrame(draw)
      }
    }

    draw()

    return () => {
      window.removeEventListener('resize', resize)
      if (animationId) cancelAnimationFrame(animationId)
    }
  }, [dotSize, dotColor, spacing, opacity, animated])

  return <canvas ref={canvasRef} className="dot-pattern-canvas" />
}

export default DotPattern
