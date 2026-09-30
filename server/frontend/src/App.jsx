import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom'
import { SocketProvider } from './context/SocketContext'
import { CampaignProvider } from './context/CampaignContext'
import { TerminalProvider } from './context/TerminalContext'
import Login from './pages/Login'
import Dashboard from './pages/Dashboard'
import UsersTab from './pages/tabs/UsersTab'
import CampaignTab from './pages/tabs/CampaignTab'
import FilesTab from './pages/tabs/FilesTab'

function Stub({ name }) {
  return (
    <div style={{ color: 'var(--text-subtle)', fontSize: 13, padding: 24 }}>
      {name}
    </div>
  )
}

// Mounted only inside the authenticated area. SocketProvider reads the JWT at
// connect time, so it must mount *after* login — at the app root it runs its
// one-shot connect() before a token exists and never retries.
function AuthenticatedProviders({ children }) {
  return (
    <SocketProvider>
      <CampaignProvider>
        <TerminalProvider>{children}</TerminalProvider>
      </CampaignProvider>
    </SocketProvider>
  )
}

export default function App() {
  return (
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<Login />} />
        <Route
          path="/dashboard"
          element={<AuthenticatedProviders><Dashboard /></AuthenticatedProviders>}
        >
          <Route index element={<Navigate to="users" replace />} />
          <Route path="users"    element={<UsersTab />} />
          <Route path="campaign" element={<CampaignTab />} />
          <Route path="builder"  element={<Stub name="Builder" />} />
          <Route path="files"    element={<FilesTab />} />
          <Route path="settings" element={<Stub name="Settings" />} />
        </Route>
        <Route path="*" element={<Navigate to="/" replace />} />
      </Routes>
    </BrowserRouter>
  )
}
