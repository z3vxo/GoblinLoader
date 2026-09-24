import { useState, useEffect, useCallback } from 'react'
import './CampaignTab.css'

function formatDate(str) {
  if (!str) return '—'
  const d = new Date(str)
  return d.toLocaleDateString('en-GB', { day: 'numeric', month: 'short', year: 'numeric' })
    + ' ' + d.toLocaleTimeString('en-GB', { hour: '2-digit', minute: '2-digit' })
}

function TrashIcon() {
  return (
    <svg width="14" height="14" viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.5">
      <path d="M2 4h12M5 4V2h6v2M6 7v5M10 7v5M3 4l1 9h8l1-9" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  )
}

function PlusIcon() {
  return (
    <svg width="13" height="13" viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="2">
      <path d="M8 3v10M3 8h10" strokeLinecap="round" />
    </svg>
  )
}

function NewCampaignModal({ onClose, onCreate }) {
  const [name, setName] = useState('')
  const [loading, setLoading] = useState(false)

  async function handleSubmit(e) {
    e.preventDefault()
    if (!name.trim()) return
    setLoading(true)
    await onCreate(name.trim())
    setLoading(false)
  }

  function handleBackdrop(e) {
    if (e.target === e.currentTarget) onClose()
  }

  return (
    <div className="modal-backdrop" onMouseDown={handleBackdrop}>
      <div className="modal-card">
        <span className="modal-title">New Campaign</span>
        <form onSubmit={handleSubmit} style={{ display: 'contents' }}>
          <div className="modal-field">
            <label htmlFor="camp-name">Name</label>
            <input
              id="camp-name"
              type="text"
              placeholder="Operation..."
              value={name}
              onChange={e => setName(e.target.value)}
              autoFocus
            />
          </div>
          <div className="modal-actions">
            <button type="button" className="btn-cancel" onClick={onClose}>Cancel</button>
            <button type="submit" className="btn-create" disabled={loading || !name.trim()}>
              {loading ? 'Creating…' : 'Create'}
            </button>
          </div>
        </form>
      </div>
    </div>
  )
}

function ConfirmDeleteModal({ campaign, onClose, onConfirm }) {
  const [loading, setLoading] = useState(false)

  function handleBackdrop(e) {
    if (e.target === e.currentTarget) onClose()
  }

  async function handleConfirm() {
    setLoading(true)
    await onConfirm()
    setLoading(false)
  }

  return (
    <div className="modal-backdrop" onMouseDown={handleBackdrop}>
      <div className="modal-card">
        <span className="modal-title">Delete Campaign</span>
        <p className="modal-warn">
          Are you sure you want to delete <strong>{campaign.name}</strong>?
          This will also permanently delete all agents and files connected to it.
        </p>
        <div className="modal-actions">
          <button type="button" className="btn-cancel" onClick={onClose}>Cancel</button>
          <button type="button" className="btn-danger" onClick={handleConfirm} disabled={loading}>
            {loading ? 'Deleting…' : 'Delete'}
          </button>
        </div>
      </div>
    </div>
  )
}

export default function CampaignTab() {
  const [campaigns, setCampaigns] = useState([])
  const [showModal, setShowModal] = useState(false)
  const [deleteTarget, setDeleteTarget] = useState(null)

  const jwt = (() => { try { return localStorage.getItem('jwt') } catch { return null } })()

  const load = useCallback(() => {
    fetch('/rest/campaigns', { headers: { Authorization: jwt } })
      .then(r => r.json())
      .then(data => setCampaigns(data.campaigns ?? []))
      .catch(() => {})
  }, [jwt])

  useEffect(() => { load() }, [load])

  async function handleCreate(name) {
    const res = await fetch('/rest/campaigns', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json', Authorization: jwt },
      body: JSON.stringify({ name }),
    })
    if (res.ok) {
      setShowModal(false)
      load()
    }
  }

  async function handleDelete() {
    const res = await fetch(`/rest/campaigns/${deleteTarget.uuid}`, {
      method: 'DELETE',
      headers: { Authorization: jwt },
    })
    if (res.ok || res.status === 204) {
      setDeleteTarget(null)
      load()
    }
  }

  return (
    <div className="camp-wrap">
      <div className="camp-header">
        <span className="camp-title">Campaigns</span>
        <button className="btn-new" onClick={() => setShowModal(true)}>
          <PlusIcon /> New Campaign
        </button>
      </div>

      <div className="camp-table">
        <div className="camp-table-head">
          <span>Name</span>
          <span>Agents</span>
          <span>Files</span>
          <span>Created</span>
          <span />
        </div>

        {campaigns.length === 0
          ? <div className="camp-empty">No campaigns yet.</div>
          : campaigns.map(c => (
              <div className="camp-row" key={c.uuid}>
                <span className="camp-row-name">{c.name}</span>
                <span className="camp-row-num">{c.total_agents}</span>
                <span className="camp-row-num">{c.total_files}</span>
                <span className="camp-row-date">{formatDate(c.created_at)}</span>
                <button
                  className="btn-delete"
                  onClick={() => setDeleteTarget(c)}
                  title="Delete campaign"
                >
                  <TrashIcon />
                </button>
              </div>
            ))
        }
      </div>

      {showModal && (
        <NewCampaignModal
          onClose={() => setShowModal(false)}
          onCreate={handleCreate}
        />
      )}

      {deleteTarget && (
        <ConfirmDeleteModal
          campaign={deleteTarget}
          onClose={() => setDeleteTarget(null)}
          onConfirm={handleDelete}
        />
      )}
    </div>
  )
}
