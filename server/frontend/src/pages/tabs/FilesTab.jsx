import { useState, useEffect, useCallback, useMemo, useRef } from 'react'
import { useCampaign } from '../../context/CampaignContext'
import './FilesTab.css'

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

function UploadIcon() {
  return (
    <svg viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.5">
      <path d="M8 10.5V2.5" strokeLinecap="round" />
      <path d="M4.5 6L8 2.5 11.5 6" strokeLinecap="round" strokeLinejoin="round" />
      <path d="M2.5 11v1.5a1 1 0 001 1h9a1 1 0 001-1V11" strokeLinecap="round" />
    </svg>
  )
}

function TrashIcon() {
  return (
    <svg viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.5">
      <path d="M2 4h12M5 4V2h6v2M6 7v5M10 7v5M3 4l1 9h8l1-9" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  )
}

function CopyIcon() {
  return (
    <svg viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.5">
      <rect x="5.5" y="5.5" width="8" height="8" rx="1.5" />
      <path d="M10.5 5.5V4A1.5 1.5 0 009 2.5H4A1.5 1.5 0 002.5 4v5A1.5 1.5 0 004 10.5h1.5" strokeLinecap="round" />
    </svg>
  )
}

function CheckIcon() {
  return (
    <svg viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.5">
      <path d="M3.5 8.5L6.5 11.5 12.5 4.5" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  )
}

function formatSize(bytes) {
  const n = Number(bytes)
  if (bytes == null || Number.isNaN(n)) return '—'
  if (n < 1024) return `${n} B`
  if (n < 1024 * 1024) return `${(n / 1024).toFixed(1)} KB`
  return `${(n / (1024 * 1024)).toFixed(1)} MB`
}

function formatDate(str) {
  if (!str) return '—'
  const d = new Date(str)
  if (Number.isNaN(d.getTime())) return '—'
  return d.toLocaleDateString('en-GB', { day: 'numeric', month: 'short', year: 'numeric' })
    + ' ' + d.toLocaleTimeString('en-GB', { hour: '2-digit', minute: '2-digit' })
}

const KIND_LABEL = { exe: 'EXE', dll: 'DLL', shellcode: 'SHELLCODE' }

function KindBadge({ kind }) {
  const k = (kind || '').toLowerCase()
  return <span className={`kind-badge kind-${k || 'unknown'}`}>{KIND_LABEL[k] || k || '—'}</span>
}

function UploadModal({ onClose, onUpload }) {
  const [file, setFile] = useState(null)
  const [kind, setKind] = useState('auto')
  const [arch, setArch] = useState('x64')
  const [dragging, setDragging] = useState(false)
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState('')
  const inputRef = useRef(null)

  function pick(f) {
    if (!f) return
    setError('')
    setFile(f)
  }

  async function handleSubmit(e) {
    e.preventDefault()
    if (!file) return
    setLoading(true)
    const result = await onUpload(file, { kind, arch })
    setLoading(false)
    if (!result.ok) setError(result.error || 'Upload failed. The server rejected the file.')
  }

  function handleDrop(e) {
    e.preventDefault()
    setDragging(false)
    pick(e.dataTransfer.files?.[0])
  }

  function handleBackdrop(e) {
    if (e.target === e.currentTarget) onClose()
  }

  return (
    <div className="modal-backdrop" onMouseDown={handleBackdrop}>
      <div className="modal-card files-modal">
        <span className="modal-title">Upload File</span>
        <form onSubmit={handleSubmit} style={{ display: 'contents' }}>
          <div
            className={`files-drop${dragging ? ' dragging' : ''}`}
            onClick={() => inputRef.current?.click()}
            onDragOver={e => { e.preventDefault(); setDragging(true) }}
            onDragLeave={() => setDragging(false)}
            onDrop={handleDrop}
          >
            <UploadIcon />
            <span className="files-drop-title">
              {file ? file.name : 'Drop a file or click to browse'}
            </span>
            <span className="files-drop-sub">
              {file ? formatSize(file.size) : 'EXE, DLL, or raw shellcode'}
            </span>
            <input
              ref={inputRef}
              type="file"
              hidden
              onChange={e => pick(e.target.files?.[0])}
            />
          </div>

          <div className="files-fields">
            <div className="files-field">
              <label htmlFor="file-kind">Type</label>
              <select id="file-kind" value={kind} onChange={e => setKind(e.target.value)}>
                <option value="auto">Auto-detect</option>
                <option value="exe">EXE</option>
                <option value="dll">DLL</option>
                <option value="shellcode">Shellcode</option>
              </select>
            </div>
            {kind === 'shellcode' && (
              <div className="files-field">
                <label htmlFor="file-arch">Arch</label>
                <select id="file-arch" value={arch} onChange={e => setArch(e.target.value)}>
                  <option value="x64">x64</option>
                  <option value="x86">x86</option>
                  <option value="arm64">arm64</option>
                </select>
              </div>
            )}
          </div>

          {error && <span className="files-error">{error}</span>}

          <div className="modal-actions">
            <button type="button" className="btn-cancel" onClick={onClose}>Cancel</button>
            <button type="submit" className="btn-create" disabled={loading || !file}>
              {loading ? 'Uploading…' : 'Upload'}
            </button>
          </div>
        </form>
      </div>
    </div>
  )
}

function ConfirmDeleteModal({ file, onClose, onConfirm }) {
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
        <span className="modal-title">Delete File</span>
        <p className="modal-warn">
          Are you sure you want to delete <strong>{file.name}</strong>?
          Any queued tasks referencing it will be removed as well.
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

export default function FilesTab() {
  const { currentCampaign } = useCampaign()
  const [files, setFiles] = useState([])
  const [loading, setLoading] = useState(false)
  const [query, setQuery] = useState('')
  const [showUpload, setShowUpload] = useState(false)
  const [deleteTarget, setDeleteTarget] = useState(null)
  const [copied, setCopied] = useState(null)

  const jwt = (() => { try { return localStorage.getItem('jwt') } catch { return null } })()

  const load = useCallback(() => {
    if (!currentCampaign) return
    setLoading(true)
    fetch(`/rest/files/${currentCampaign.uuid}`, { headers: { Authorization: jwt } })
      .then(r => (r.ok ? r.json() : { files: [] }))
      .then(data => setFiles(data.files ?? []))
      .catch(() => setFiles([]))
      .finally(() => setLoading(false))
  }, [currentCampaign, jwt])

  useEffect(() => { load() }, [load])

  async function handleUpload(file, opts) {
    const fd = new FormData()
    fd.append('file', file)
    fd.append('kind', opts?.kind ?? 'auto')
    fd.append('arch', opts?.arch ?? '')
    const res = await fetch(`/rest/files/${currentCampaign.uuid}`, {
      method: 'POST',
      headers: { Authorization: jwt },
      body: fd,
    })
    if (res.ok) {
      setShowUpload(false)
      load()
      return { ok: true }
    }
    let error = 'Upload failed.'
    try {
      const data = await res.json()
      if (data.msg) error = data.msg
    } catch {
      error = 'Upload failed. The server rejected the file.'
    }
    return { ok: false, error }
  }

  async function handleDelete() {
    const res = await fetch(`/rest/files/${deleteTarget.uuid}`, {
      method: 'DELETE',
      headers: { Authorization: jwt },
    })
    if (res.ok || res.status === 204) {
      setDeleteTarget(null)
      load()
    }
  }

  function copyId(uuid) {
    if (!navigator.clipboard) return
    navigator.clipboard.writeText(uuid).then(() => {
      setCopied(uuid)
      setTimeout(() => setCopied(c => (c === uuid ? null : c)), 1200)
    }).catch(() => {})
  }

  const filtered = useMemo(() => {
    const q = query.trim().toLowerCase()
    if (!q) return files
    return files.filter(f =>
      [f.name, f.kind, f.arch, f.uuid]
        .some(v => v && String(v).toLowerCase().includes(q))
    )
  }, [files, query])

  if (!currentCampaign) {
    return (
      <div className="files-empty-state">
        <span className="files-empty-title">No campaign selected</span>
        <span className="files-empty-sub">Choose a campaign from the dropdown to manage its files.</span>
      </div>
    )
  }

  return (
    <div className="files-panel">
      <div className="files-toolbar">
        <div className="files-heading">
          <span className="files-title">Files</span>
          <span className="files-count">{files.length}</span>
        </div>

        <div className="files-toolbar-right">
          <div className="files-search">
            <SearchIcon />
            <input
              type="text"
              placeholder="Filter files…"
              value={query}
              onChange={e => setQuery(e.target.value)}
              spellCheck={false}
            />
          </div>
          <button className="files-refresh" onClick={load} disabled={loading} title="Refresh">
            <RefreshIcon />
          </button>
          <button className="btn-new" onClick={() => setShowUpload(true)}>
            <UploadIcon /> Upload
          </button>
        </div>
      </div>

      <div className="files-scroll">
        <table className="files-table">
          <thead>
            <tr>
              <th className="fcol-name">Name</th>
              <th className="fcol-kind">Type</th>
              <th className="fcol-arch">Arch</th>
              <th className="fcol-size">Size</th>
              <th className="fcol-date">Added</th>
              <th className="fcol-uuid">UUID</th>
              <th className="fcol-actions" />
            </tr>
          </thead>
          <tbody>
            {loading
              ? <tr><td colSpan={7} className="files-cell-msg">Loading…</td></tr>
              : filtered.length === 0
                ? <tr><td colSpan={7} className="files-cell-msg">
                    {query ? 'No files match your filter.' : 'No files in this campaign yet.'}
                  </td></tr>
                : filtered.map(f => (
                    <tr key={f.uuid}>
                      <td className="fcol-name">{f.name}</td>
                      <td className="fcol-kind"><KindBadge kind={f.kind} /></td>
                      <td className="fcol-arch">{f.arch ? <span className="files-tag">{f.arch}</span> : '—'}</td>
                      <td className="fcol-size">{formatSize(f.size)}</td>
                      <td className="fcol-date">{formatDate(f.created_at)}</td>
                      <td className="fcol-uuid">{f.uuid}</td>
                      <td className="fcol-actions">
                        <button
                          className={`files-icon-btn${copied === f.uuid ? ' copied' : ''}`}
                          onClick={() => copyId(f.uuid)}
                          title="Copy UUID"
                        >
                          {copied === f.uuid ? <CheckIcon /> : <CopyIcon />}
                        </button>
                        <button
                          className="files-icon-btn danger"
                          onClick={() => setDeleteTarget(f)}
                          title="Delete file"
                        >
                          <TrashIcon />
                        </button>
                      </td>
                    </tr>
                  ))
            }
          </tbody>
        </table>
      </div>

      {showUpload && (
        <UploadModal
          onClose={() => setShowUpload(false)}
          onUpload={handleUpload}
        />
      )}

      {deleteTarget && (
        <ConfirmDeleteModal
          file={deleteTarget}
          onClose={() => setDeleteTarget(null)}
          onConfirm={handleDelete}
        />
      )}
    </div>
  )
}
