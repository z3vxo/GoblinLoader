package server

import (
	"bytes"
	"encoding/json"
	"io"
	"net/http"

	"github.com/go-chi/chi/v5"

	"ldrserver/internal/parser"
	"ldrserver/internal/utils"
	"ldrserver/internal/ws"
)

func (s *Server) HandleAgentCheckin(w http.ResponseWriter, r *http.Request) {
	defer r.Body.Close()

	body, err := io.ReadAll(r.Body)
	if err != nil {
		utils.Return400(w, "failed reading request body")
		return
	}

	reader := parser.NewReader(bytes.NewReader(body))
	code, agentID := reader.GetCodeAndAgentID()
	if reader.Err() != nil {
		utils.Return400(w, "failed parsing request")
		return
	}

	switch code {
	case parser.CHECK_IN:
		if _, err := s.DB.GetTasks(agentID); err != nil {
			utils.Return500(w, "failed getting tasks")
			return
		}
		s.WS.Broadcast(ws.EventAgentCheckin, map[string]string{"agent_id": agentID})

	default:
		return
	}
}

func (s *Server) GetAgents(w http.ResponseWriter, r *http.Request) {
	id := chi.URLParam(r, "id")
	agents, err := s.DB.GetAgents(id)
	if err != nil {
		utils.Return500(w, "failed getting agents from db")
		return
	}
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(agents)
}

func (s *Server) DeleteAgent(w http.ResponseWriter, r *http.Request) {
	id := chi.URLParam(r, "id")
	if err := s.DB.DeleteAgent(id); err != nil {
		utils.Return500(w, "Failed Deleting Agent")
		return
	}
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{"msg": "Succesfully deleted agent"})
	return
}

