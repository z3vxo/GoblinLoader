package server

import (
	"encoding/json"
	"net/http"
	"github.com/go-chi/chi/v5"
	"ldrserver/internal/utils"
)

func (s *Server) GetCampaigns(w http.ResponseWriter, r *http.Request) {
	campaigns, err := s.DB.GetCampaigns()
	if err != nil {
		utils.Return500(w, "failed getting campaigns from db")
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(campaigns)
}

func (s *Server) CreateCampaign(w http.ResponseWriter, r *http.Request) {
	var req NewCampaignReq
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		utils.Return500(w, "failed decoding json")
		return
	}

	campaign, err := s.DB.InsertCampaign(req.Name)
	if err != nil {
		utils.Return500(w, "failed creating campaign")
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusCreated)
	json.NewEncoder(w).Encode(campaign)
}

func (s *Server) DeleteCampaign(w http.ResponseWriter, r *http.Request) {
	id := chi.URLParam(r, "id")
	if err := s.DB.DeleteCampaign(id); err != nil {
		utils.Return500(w, "failed deleting campaign")
		return
	}
	w.WriteHeader(http.StatusNoContent)
}

