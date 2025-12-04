import '../styles/ConnectionBanner.css'

function ConnectionBanner({ show, message }) {
  return (
    <div className={`connection-banner ${show ? 'show' : ''}`}>
      <span className="banner-icon">✓</span>
      {message}
    </div>
  )
}

export default ConnectionBanner


