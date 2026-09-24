import { useState, useEffect, useCallback, useMemo, useRef } from 'react'
import { useCampaign } from '../../context/CampaignContext'
import { useTerminal } from '../../context/TerminalContext'
import './UsersTab.css'

function SearchIcon() {
  return (
    <svg viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.5">
      <circle cx="7" cy="7" r="4.5" />
      <path d="M10.5 10.5L14 14" strokeLinecap="round" />
    </svg>
  )
}

function RefreshIcon() {
  return (
    <svg viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.5">
      <path d="M13 8a5 5 0 1 1-1.5-3.5" strokeLinecap="round" />
      <path d="M13 2.5V5h-2.5" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  )
}

function TerminalIcon() {
  return (
    <svg viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.5">
      <rect x="1.5" y="2.5" width="13" height="11" rx="2" />
      <path d="M4.5 5.5L7 8l-2.5 2.5" strokeLinecap="round" strokeLinejoin="round" />
      <path d="M9 11h2.5" strokeLinecap="round" />
    </svg>
  )
}

function CloseIcon() {
  return (
    <svg viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.5">
      <path d="M4 4l8 8M12 4l-8 8" strokeLinecap="round" />
    </svg>
  )
}

function Terminal({ agent, onClose }) {
  const [lines, setLines] = useState([])
  const [input, setInput] = useState('')
  const [connState, setConnState] = useState('connecting')
  const socketRef = useRef(null)
  const bodyRef = useRef(null)

  useEffect(() => {
    const el = bodyRef.current
    if (el) el.scrollTop = el.scrollHeight
  }, [lines])

  useEffect(() => {
    let jwt = null
    try { jwt = localStorage.getItem('jwt') } catch {}

    if (!jwt) {
      setConnState('closed')
      return
    }

    const proto = window.location.protocol === 'https:' ? 'wss' : 'ws'
    const url = `${proto}://${window.location.host}/rest/ws/${encodeURIComponent(agent.uuid)}?token=${encodeURIComponent(jwt)}`

    const socket = new WebSocket(url)
    socketRef.current = socket

    socket.onopen = () => setConnState('open')
    socket.onclose = () => setConnState('closed')
    socket.onerror = () => setConnState('closed')
    socket.onmessage = ev => setLines(ls => [...ls, { kind: 'out', text: ev.data }])

    return () => {
      socket.onopen = socket.onclose = socket.onerror = socket.onmessage = null
      socket.close()
      socketRef.current = null
    }
  }, [agent.uuid])

  function submit(e) {
    e.preventDefault()
    const text = input
    if (!text) return
    setLines(ls => [...ls, { kind: 'cmd', text }])

    const socket = socketRef.current
    if (socket && socket.readyState === WebSocket.OPEN) {
      socket.send(text)
    } else {
      setLines(ls => [...ls, { kind: 'out', text: '[no connection]' }])
    }

    setInput('')
  }

  const statusLabel =
    connState === 'open' ? 'connected'
      : connState === 'closed' ? 'disconnected'
        : 'connecting…'

  function handleClose() {
    const socket = socketRef.current
    if (socket) {
      socket.onopen = socket.onclose = socket.onerror = socket.onmessage = null
      socket.close()
      socketRef.current = null
    }
    onClose()
  }

  return (
    <div className="terminal">
      <div className="terminal-header">
        <span className="terminal-eyebrow">Terminal</span>
        <span className="terminal-agent">{agent.uuid}</span>
        <span className={`terminal-status${connState === 'open' ? ' online' : ''}`}>
          <span className="terminal-status-dot" />
          {statusLabel}
        </span>
        <button className="terminal-close" onClick={handleClose} title="Close terminal">
          <CloseIcon />
        </button>
      </div>

      <div className="terminal-body" ref={bodyRef}>
        {lines.length === 0
          ? <span className="term-idle">Connected to {agent.hostname}. Type a command…</span>
          : lines.map((l, i) =>
              l.kind === 'cmd'
                ? <div key={i} className="term-cmd"><span className="term-prompt">›</span>{l.text}</div>
                : <div key={i} className="term-out">{l.text}</div>
            )
        }
      </div>

      <form className="terminal-form" onSubmit={submit}>
        <span className="term-prompt">›</span>
        <input
          type="text"
          value={input}
          onChange={e => setInput(e.target.value)}
          placeholder="Type a command…"
          autoFocus
          spellCheck={false}
        />
      </form>
    </div>
  )
}

export default function UsersTab() {
  const { currentCampaign } = useCampaign()
  const { activeAgent, setActiveAgent } = useTerminal()
  const [agents, setAgents] = useState([])
  const [loading, setLoading] = useState(false)
  const [query, setQuery] = useState('')

  const jwt = (() => { try { return localStorage.getItem('jwt') } catch { return null } })()

  const load = useCallback(() => {
    if (!currentCampaign) return
    setLoading(true)
    fetch(`/rest/agents/${currentCampaign.uuid}`, { headers: { Authorization: jwt } })
      .then(r => r.json())
      .then(data => setAgents(data.agents ?? []))
      .catch(() => {})
      .finally(() => setLoading(false))
  }, [currentCampaign, jwt])

  useEffect(() => { load() }, [load])

  const filtered = useMemo(() => {
    const q = query.trim().toLowerCase()
    if (!q) return agents
    return agents.filter(a =>
      [a.hostname, a.username, a.domain, a.arch, a.country, a.uuid]
        .some(v => v && v.toLowerCase().includes(q))
    )
  }, [agents, query])

  if (!currentCampaign) {
    return (
      <div className="agents-empty-state">
        <span className="agents-empty-title">No campaign selected</span>
        <span className="agents-empty-sub">Choose a campaign from the dropdown to view its agents.</span>
      </div>
    )
  }

  return (
    <div className="users-layout">
      <div className="agents-panel">
        <div className="agents-toolbar">
          <div className="agents-heading">
            <span className="agents-title">Agents</span>
            <span className="agents-count">{agents.length}</span>
          </div>

          <div className="agents-toolbar-right">
            <div className="agents-search">
              <SearchIcon />
              <input
                type="text"
                placeholder="Filter agents…"
                value={query}
                onChange={e => setQuery(e.target.value)}
                spellCheck={false}
              />
            </div>
            <button className="agents-refresh" onClick={load} disabled={loading} title="Refresh">
              <RefreshIcon />
            </button>
          </div>
        </div>

        <div className="agents-scroll">
          <table className="agents-table">
            <thead>
              <tr>
                <th className="col-actions" />
                <th className="col-status" />
                <th className="col-host">Host</th>
                <th className="col-user">User</th>
                <th className="col-domain">Domain</th>
                <th className="col-arch">Arch</th>
                <th className="col-country">Country</th>
                <th className="col-uuid">UUID</th>
              </tr>
            </thead>
            <tbody>
              {loading
                ? <tr><td colSpan={8} className="agents-cell-msg">Loading…</td></tr>
                : filtered.length === 0
                  ? <tr><td colSpan={8} className="agents-cell-msg">
                      {query ? 'No agents match your filter.' : 'No agents for this campaign.'}
                    </td></tr>
                  : filtered.map(a => {
                      const isActive = activeAgent?.uuid === a.uuid
                      return (
                        <tr key={a.uuid}>
                          <td className="col-actions">
                            <button
                              className={`agent-term-btn${isActive ? ' active' : ''}`}
                              onClick={() => setActiveAgent(isActive ? null : a)}
                              title={isActive ? 'Close terminal' : 'Open terminal'}
                            >
                              <TerminalIcon />
                            </button>
                          </td>
                          <td className="col-status"><span className="agent-dot" /></td>
                          <td className="col-host">{a.hostname}</td>
                          <td className="col-user">{a.username}</td>
                          <td className="col-domain">{a.domain}</td>
                          <td className="col-arch"><span className="agent-tag">{a.arch}</span></td>
                          <td className="col-country">{a.country || '—'}</td>
                          <td className="col-uuid">{a.uuid}</td>
                        </tr>
                      )
                    })
              }
            </tbody>
          </table>
        </div>
      </div>

      {activeAgent && (
        <Terminal
          agent={activeAgent}
          onClose={() => setActiveAgent(null)}
        />
      )}
    </div>
  )
}
