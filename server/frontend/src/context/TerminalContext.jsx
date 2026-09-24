import { createContext, useContext, useState } from 'react'

const TerminalContext = createContext(null)

export function TerminalProvider({ children }) {
  const [activeAgent, setActiveAgent] = useState(null)

  return (
    <TerminalContext.Provider value={{ activeAgent, setActiveAgent }}>
      {children}
    </TerminalContext.Provider>
  )
}

export function useTerminal() {
  return useContext(TerminalContext)
}
