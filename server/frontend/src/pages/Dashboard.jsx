import { useEffect, useState, useRef } from 'react'
import { useNavigate, useLocation, Outlet, NavLink } from 'react-router-dom'
import { useCampaign } from '../context/CampaignContext'
import './Dashboard.css'

const TABS = [
  { label: 'Users',    path: 'users'    },
  { label: 'Campaign', path: 'campaign' },
  { label: 'Builder',  path: 'builder'  },
  { label: 'Files',    path: 'files'    },
  { label: 'Settings', path: 'settings' },
]

function ChevronIcon({ className }) {
  return (
    <svg className={className} viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="1.5">
      <path d="M4 6l4 4 4-4" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  )
}

function CampaignDropdown({ campaigns, current, onSelect }) {
  const [open, setOpen] = useState(false)
  const ref = useRef(null)

  useEffect(() => {
    function handleClick(e) {
      if (ref.current && !ref.current.contains(e.target)) setOpen(false)
    }
    document.addEventListener('mousedown', handleClick)
    return () => document.removeEventListener('mousedown', handleClick)
  }, [])

  function select(c) { onSelect(c); setOpen(false) }

  return (
    <div className="campaign-select-wrap" ref={ref}>
      <button className="campaign-btn" onClick={() => setOpen(o => !o)}>
        {current
          ? <span className="campaign-btn-name">{current.name}</span>
          : <span className="campaign-btn-placeholder">Select campaign</span>
        }
        <ChevronIcon className={`campaign-chevron${open ? ' open' : ''}`} />
      </button>

      {open && (
        <div className="campaign-dropdown">
          {campaigns.length === 0
            ? <div className="campaign-dropdown-empty">No campaigns</div>
            : campaigns.map(c => (
                <div
                  key={c.uuid}
                  className={`campaign-option${current?.uuid === c.uuid ? ' active' : ''}`}
                  onClick={() => select(c)}
                >
                  <span className="campaign-option-name">{c.name}</span>
                  <span className="campaign-option-date">{c.created_at}</span>
                </div>
              ))
          }
        </div>
      )}
    </div>
  )
}

export default function Dashboard() {
  const navigate = useNavigate()
  const { currentCampaign, setCurrentCampaign } = useCampaign()
  const [campaigns, setCampaigns] = useState([])

  useEffect(() => {
    const jwt = (() => { try { return localStorage.getItem('jwt') } catch { return null } })()
    if (!jwt) { navigate('/'); return }

    fetch('/rest/campaigns', { headers: { Authorization: jwt } })
      .then(res => { if (res.status === 401) { navigate('/'); return null } return res.json() })
      .then(data => { if (data) setCampaigns(data.campaigns ?? []) })
      .catch(() => {})
  }, [navigate])

  return (
    <div className="dashboard">
      <header className="topbar">
        <span className="topbar-wordmark">AetherLoader</span>

        <nav className="topbar-nav">
          {TABS.map(tab => (
            <NavLink
              key={tab.path}
              to={`/dashboard/${tab.path}`}
              className={({ isActive }) => `tab-link${isActive ? ' active' : ''}`}
            >
              {tab.label}
            </NavLink>
          ))}
        </nav>

        <CampaignDropdown
          campaigns={campaigns}
          current={currentCampaign}
          onSelect={setCurrentCampaign}
        />
      </header>

      <main className="dashboard-body">
        <Outlet />
      </main>
    </div>
  )
}
