import { useState, useEffect, useCallback, useMemo, useRef } from 'react'
import { useCampaign } from '../../context/CampaignContext'
import { useTerminal } from '../../context/TerminalContext'
import { useSocket } from '../../context/SocketContext'
import './UsersTab.css'

const WS_LIST_FILES = 1
const WS_RUN_FILE = 2

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
  const { currentCampaign } = useCampaign()
  const { state: connState, send, on } = useSocket()
  const [lines, setLines] = useState([])
  const [input, setInput] = useState('')
  const bodyRef = useRef(null)

  const jwt = (() => { try { return localStorage.getItem('jwt') } catch { return null } })()

  useEffect(() => {
    const el = bodyRef.current
    if (el) el.scrollTop = el.scrollHeight
  }, [lines])

  useEffect(() => {
    return on('agent.output', data => {
      if (!data) return
      const id = data.agent_id || data.uuid
      if (id !== agent.uuid) return
      setLines(ls => [...ls, { kind: 'out', text: data.output ?? data.text ?? '' }])
    })
  }, [on, agent.uuid])

  async function submit(e) {
    e.preventDefault()
    const text = input
    if (!text) return
    setLines(ls => [...ls, { kind: 'cmd', text }])
    setInput('')

    const parts = text.trim().split(/\s+/)
    const cmd = parts[0].toLowerCase()

    if (cmd === 'info') {
      if (!currentCampaign) {
        setLines(ls => [...ls, { kind: 'out', text: '[no campaign selected]' }])
        return
      }
      try {
        const r = await fetch(`/rest/agents/${currentCampaign.uuid}/${agent.uuid}`, {
          headers: { Authorization: jwt },
        })
        if (!r.ok) {
          setLines(ls => [...ls, { kind: 'out', text: `[error] ${r.status} ${r.statusText}` }])
          return
        }
        const info = await r.json()
        const rows = [
          ['User', info.username],
          ['Host', info.hostname],
          ['Hostname', info.domain],
          ['Process', info.process],
          ['Arch', info.arch],
          ['Is Admin', info.is_elev ? 'TRUE' : 'FALSE'],
          ['Country', info.country],
        ]
        setLines(ls => [...ls, ...rows.map(([key, value]) => ({
          kind: 'kv',
          key,
          value: value || '—',
        }))])
      } catch {
        setLines(ls => [...ls, { kind: 'out', text: '[error] request failed' }])
      }
      return
    }

    if (!currentCampaign) {
      setLines(ls => [...ls, { kind: 'out', text: '[no campaign selected]' }])
      return
    }

    if (cmd === 'files') {
      const res = await send({
        code: WS_LIST_FILES,
        agent_id: agent.uuid,
        campaign_id: currentCampaign.uuid,
      })

      if (!res || !res.ok) {
        setLines(ls => [...ls, { kind: 'out', text: `[error] ${res?.msg || 'request failed'}` }])
        return
      }

      const files = res.data?.files ?? []
      setLines(ls => {
        const out = [{ kind: 'out', text: `files (${res.data?.total ?? files.length})` }]
        if (files.length === 0) out.push({ kind: 'out', text: '  no files' })
        for (const f of files) out.push({ kind: 'out', text: `  ${f.uuid}  ${f.name}` })
        return [...ls, ...out]
      })
      return
    }

    if (cmd === 'run') {
      const fileUUID = parts[1]
      if (!fileUUID) {
        setLines(ls => [...ls, { kind: 'out', text: '[usage] run <file_uuid>' }])
        return
      }

      const res = await send({
        code: WS_RUN_FILE,
        agent_id: agent.uuid,
        campaign_id: currentCampaign.uuid,
        payload: { file_uuid: fileUUID },
      })

      if (!res || !res.ok) {
        setLines(ls => [...ls, { kind: 'out', text: `[error] ${res?.msg || 'request failed'}` }])
        return
      }

      setLines(ls => [...ls, { kind: 'out', text: `[+] task queued: ${fileUUID}` }])
      return
    }

    setLines(ls => [...ls, { kind: 'out', text: '[not implemented]' }])
  }

  const statusLabel =
    connState === 'open' ? 'connected'
      : connState === 'closed' ? 'disconnected'
        : 'connecting…'

  return (
    <div className="terminal">
      <div className="terminal-header">
        <span className="terminal-eyebrow">Terminal</span>
        <span className="terminal-agent">{agent.uuid}</span>
        <span className={`terminal-status${connState === 'open' ? ' online' : ''}`}>
          <span className="terminal-status-dot" />
          {statusLabel}
        </span>
        <button className="terminal-close" onClick={onClose} title="Close terminal">
          <CloseIcon />
        </button>
      </div>

      <div className="terminal-body" ref={bodyRef}>
        {lines.length === 0
          ? <span className="term-idle">Connected to {agent.hostname}. Type a command…</span>
          : lines.map((l, i) =>
              l.kind === 'cmd'
                ? <div key={i} className="term-cmd"><span className="term-prompt">›</span>{l.text}</div>
                : l.kind === 'kv'
                  ? <div key={i} className="term-kv"><span className="term-key">{l.key}</span><span className="term-val">{l.value}</span></div>
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
  const { on } = useSocket()
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

  useEffect(() => {
    return on('agent.new', data => {
      if (!data?.agent) return
      if (data.campaign_id && currentCampaign && data.campaign_id !== currentCampaign.uuid) return
      const incoming = data.agent
      setAgents(prev => {
        const i = prev.findIndex(a => a.uuid === incoming.uuid)
        if (i === -1) return [...prev, incoming]
        const next = prev.slice()
        next[i] = incoming
        return next
      })
    })
  }, [on, currentCampaign])

  const filtered = useMemo(() => {
    const q = query.trim().toLowerCase()
    if (!q) return agents
    return agents.filter(a =>
      [a.hostname, a.username, a.domain, a.arch, a.country, String(a.is_elev)]
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
                <th className="col-admin">Is Admin</th>
                <th className="col-country">Country</th>
                <th className="col-lastseen">Last Seen</th>
              </tr>
            </thead>
            <tbody>
              {loading
                ? <tr><td colSpan={9} className="agents-cell-msg">Loading…</td></tr>
                : filtered.length === 0
                  ? <tr><td colSpan={9} className="agents-cell-msg">
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
                          <td className="col-domain">{a.domain || '—'}</td>
                          <td className="col-arch"><span className="agent-tag">{a.arch}</span></td>
                          <td className="col-admin">
                            {a.is_elev
                              ? <span className="agent-tag">yes</span>
                              : <span className="agent-admin-no">no</span>}
                          </td>
                          <td className="col-country">{a.country || '—'}</td>
                          <td className="col-lastseen">{a.last_seen || '—'}</td>
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
