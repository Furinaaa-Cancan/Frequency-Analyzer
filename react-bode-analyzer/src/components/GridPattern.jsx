import { useEffect, useRef } from 'react'
import '../styles/GridPattern.css'

const GridPattern = ({ 
  gridSize = 40, 
  lineColor = '#c9a97a', 
  lineWidth = 1,
  opacity = 0.15,
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

      const rows = Math.ceil(canvas.height / gridSize)
      const cols = Math.ceil(canvas.width / gridSize)

      // 绘制垂直线
      for (let col = 0; col <= cols; col++) {
        const x = col * gridSize
        const fadeOpacity = animated 
          ? opacity * (0.5 + 0.5 * Math.sin(offset + col * 0.1))
          : opacity

        ctx.strokeStyle = `${lineColor}${Math.floor(fadeOpacity * 255).toString(16).padStart(2, '0')}`
        ctx.lineWidth = lineWidth
        ctx.beginPath()
        ctx.moveTo(x, 0)
        ctx.lineTo(x, canvas.height)
        ctx.stroke()
      }

      // 绘制水平线
      for (let row = 0; row <= rows; row++) {
        const y = row * gridSize
        const fadeOpacity = animated 
          ? opacity * (0.5 + 0.5 * Math.sin(offset + row * 0.1))
          : opacity

        ctx.strokeStyle = `${lineColor}${Math.floor(fadeOpacity * 255).toString(16).padStart(2, '0')}`
        ctx.lineWidth = lineWidth
        ctx.beginPath()
        ctx.moveTo(0, y)
        ctx.lineTo(canvas.width, y)
        ctx.stroke()
      }

      if (animated) {
        offset += 0.03
        animationId = requestAnimationFrame(draw)
      }
    }

    draw()

    return () => {
      window.removeEventListener('resize', resize)
      if (animationId) cancelAnimationFrame(animationId)
    }
  }, [gridSize, lineColor, lineWidth, opacity, animated])

  return <canvas ref={canvasRef} className="grid-pattern-canvas" />
}

export default GridPattern
