import { useState } from 'react'
import { useNavigate } from 'react-router-dom'
import './Login.css'

export default function Login() {
  const navigate = useNavigate()
  const [username, setUsername] = useState('')
  const [password, setPassword] = useState('')
  const [error, setError]       = useState('')
  const [loading, setLoading]   = useState(false)

  const hasError = error.length > 0

  function clearError() {
    if (hasError) setError('')
  }

  async function handleSubmit(e) {
    e.preventDefault()

    if (!username.trim() || !password) {
      setError('Username and password are required.')
      return
    }

    setLoading(true)
    setError('')

    try {
      const res = await fetch('/rest/login', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ username: username.trim(), password }),
      })

      if (res.ok) {
        const data = await res.json()
        try { localStorage.setItem('jwt', data.jwt) } catch (_) {}
        navigate('/dashboard')
      } else {
        try {
          const data = await res.json()
          setError(data.msg || `Server error (${res.status}).`)
        } catch (_) {
          setError(`Server error (${res.status}).`)
        }
      }
    } catch (_) {
      setError('Could not reach the server.')
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="login-wrap">
      <div className="login-card">
        <div className="login-header">
          <span className="login-wordmark">AetherLoader</span>
          <h1 className="login-title">Sign in</h1>
          <p className="login-subtitle">Enter your credentials to continue.</p>
        </div>

        <form className="login-form" onSubmit={handleSubmit} noValidate>
          {hasError && (
            <div className="login-error">
              <span className="error-dot" />
              <span>{error}</span>
            </div>
          )}

          <div className="login-field">
            <label htmlFor="username">Username</label>
            <input
              id="username"
              type="text"
              value={username}
              onChange={e => { setUsername(e.target.value); clearError() }}
              placeholder="admin"
              autoComplete="username"
              autoCorrect="off"
              autoCapitalize="off"
              spellCheck={false}
              className={hasError ? 'input-error' : ''}
            />
          </div>

          <div className="login-field">
            <label htmlFor="password">Password</label>
            <input
              id="password"
              type="password"
              value={password}
              onChange={e => { setPassword(e.target.value); clearError() }}
              placeholder="••••••••"
              autoComplete="current-password"
              className={hasError ? 'input-error' : ''}
            />
          </div>

          <button className="login-btn" type="submit" disabled={loading}>
            {loading && <span className="login-spinner" />}
            <span>{loading ? 'Signing in' : 'Sign in'}</span>
          </button>
        </form>

        <div className="login-footer">v0.1.0 · operator only</div>
      </div>
    </div>
  )
}
