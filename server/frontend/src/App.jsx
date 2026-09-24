import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom'
import { CampaignProvider } from './context/CampaignContext'
import { TerminalProvider } from './context/TerminalContext'
import Login from './pages/Login'
import Dashboard from './pages/Dashboard'
import UsersTab from './pages/tabs/UsersTab'
import CampaignTab from './pages/tabs/CampaignTab'

function Stub({ name }) {
  return (
    <div style={{ color: 'var(--text-subtle)', fontSize: 13, padding: 24 }}>
      {name}
    </div>
  )
}

export default function App() {
  return (
    <BrowserRouter>
      <CampaignProvider>
        <TerminalProvider>
          <Routes>
          <Route path="/" element={<Login />} />
          <Route path="/dashboard" element={<Dashboard />}>
            <Route index element={<Navigate to="users" replace />} />
            <Route path="users"    element={<UsersTab />} />
            <Route path="campaign" element={<CampaignTab />} />
            <Route path="builder"  element={<Stub name="Builder" />} />
            <Route path="files"    element={<Stub name="Files" />} />
            <Route path="settings" element={<Stub name="Settings" />} />
          </Route>
          <Route path="*" element={<Navigate to="/" replace />} />
        </Routes>
        </TerminalProvider>
      </CampaignProvider>
    </BrowserRouter>
  )
}
