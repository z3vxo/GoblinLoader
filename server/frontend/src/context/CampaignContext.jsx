import { createContext, useContext, useEffect, useState } from 'react'

const CampaignContext = createContext(null)
const STORAGE_KEY = 'campaign'

// Restore the last chosen campaign ({ uuid, name, created_at }) across visits.
function loadStored() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY)
    return raw ? JSON.parse(raw) : null
  } catch {
    return null
  }
}

export function CampaignProvider({ children }) {
  const [currentCampaign, setCurrentCampaign] = useState(loadStored)

  useEffect(() => {
    try {
      if (currentCampaign) localStorage.setItem(STORAGE_KEY, JSON.stringify(currentCampaign))
      else localStorage.removeItem(STORAGE_KEY)
    } catch {}
  }, [currentCampaign])

  return (
    <CampaignContext.Provider value={{ currentCampaign, setCurrentCampaign }}>
      {children}
    </CampaignContext.Provider>
  )
}

export function useCampaign() {
  return useContext(CampaignContext)
}
