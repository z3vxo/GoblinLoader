import { createContext, useContext, useState } from 'react'

const CampaignContext = createContext(null)

export function CampaignProvider({ children }) {
  const [currentCampaign, setCurrentCampaign] = useState(null) // { uuid, name, created_at }

  return (
    <CampaignContext.Provider value={{ currentCampaign, setCurrentCampaign }}>
      {children}
    </CampaignContext.Provider>
  )
}

export function useCampaign() {
  return useContext(CampaignContext)
}
