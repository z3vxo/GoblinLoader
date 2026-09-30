import { createContext, useContext, useEffect, useRef, useState, useCallback } from 'react'

const SocketContext = createContext(null)

function newId() {
  if (typeof crypto !== 'undefined' && crypto.randomUUID) return crypto.randomUUID()
  return `${Date.now()}-${Math.random().toString(16).slice(2)}`
}

export function SocketProvider({ children }) {
  const [state, setState] = useState('connecting')
  const socketRef = useRef(null)
  const handlersRef = useRef(new Map())
  const pendingRef = useRef(new Map())
  const backoffRef = useRef(1000)

  const send = useCallback(req => {
    return new Promise(resolve => {
      const socket = socketRef.current
      if (!socket || socket.readyState !== WebSocket.OPEN) {
        resolve({ ok: false, msg: 'no connection' })
        return
      }
      const id = req.id || newId()
      pendingRef.current.set(id, resolve)
      socket.send(JSON.stringify({ type: 'req', ...req, id }))
    })
  }, [])

  const on = useCallback((event, handler) => {
    const set = handlersRef.current.get(event) ?? new Set()
    set.add(handler)
    handlersRef.current.set(event, set)
    return () => { set.delete(handler) }
  }, [])

  useEffect(() => {
    let disposed = false
    let reconnectTimer = null

    function connect() {
      let jwt = null
      try { jwt = localStorage.getItem('jwt') } catch {}
      if (!jwt) {
        setState('closed')
        return
      }

      const proto = window.location.protocol === 'https:' ? 'wss' : 'ws'
      const url = `${proto}://${window.location.host}/rest/ws?token=${encodeURIComponent(jwt)}`
      const socket = new WebSocket(url)
      socketRef.current = socket

      socket.onopen = () => {
        backoffRef.current = 1000
        setState('open')
      }

      socket.onclose = ev => {
        socketRef.current = null
        setState('closed')
        if (ev && ev.code !== 1000) {
          console.warn(`[ws] closed (code ${ev.code}${ev.reason ? `: ${ev.reason}` : ''}), retrying in ${backoffRef.current}ms`)
        }
        if (!disposed) {
          reconnectTimer = setTimeout(connect, backoffRef.current)
          backoffRef.current = Math.min(backoffRef.current * 2, 15000)
        }
      }

      socket.onerror = () => {}

      socket.onmessage = ev => {
        let f = null
        try { f = JSON.parse(ev.data) } catch { return }

        if (f.type === 'res' && f.id) {
          const resolve = pendingRef.current.get(f.id)
          if (resolve) {
            pendingRef.current.delete(f.id)
            resolve(f)
          }
        } else if (f.type === 'event' && f.event) {
          const set = handlersRef.current.get(f.event)
          if (set) for (const h of set) h(f.data)
        }
      }
    }

    connect()

    return () => {
      disposed = true
      if (reconnectTimer) clearTimeout(reconnectTimer)
      const socket = socketRef.current
      if (socket) {
        socket.onclose = null
        socket.close()
      }
      socketRef.current = null
    }
  }, [])

  return (
    <SocketContext.Provider value={{ state, send, on }}>
      {children}
    </SocketContext.Provider>
  )
}

export function useSocket() {
  return useContext(SocketContext)
}
