package server

import (
	"encoding/json"
	"net/http"
	"github.com/go-chi/chi/v5"
	"ldrserver/internal/utils"
)

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
