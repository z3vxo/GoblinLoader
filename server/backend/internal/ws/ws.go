package ws

import (
	"net/http"
	"sync"

	"github.com/go-chi/chi/v5"
	"github.com/gorilla/websocket"
)

type WS struct {
	mu      sync.RWMutex
	Clients map[string]*websocket.Conn
}

func New() *WS {
	return &WS{
		Clients: make(map[string]*websocket.Conn),
	}
}

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool { return true },
}

func (ws *WS) Add(id string, conn *websocket.Conn) {
	ws.mu.Lock()
	defer ws.mu.Unlock()
	ws.Clients[id] = conn
}

func (ws *WS) Remove(id string) {
	ws.mu.Lock()
	defer ws.mu.Unlock()
	delete(ws.Clients, id)
}

func (ws *WS) Send(id, msg string) error {
	ws.mu.RLock()
	conn, ok := ws.Clients[id]
	ws.mu.RUnlock()
	if !ok {
		return nil
	}

	return conn.WriteMessage(websocket.TextMessage, []byte(msg))
}

func (ws *WS) Handler(w http.ResponseWriter, r *http.Request) {
	uuid := chi.URLParam(r, "agentID")
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		return
	}

	ws.Add(uuid, conn)
	defer func() {
		ws.Remove(uuid)
		conn.Close()
	}()

	for {
		mt, msg, err := conn.ReadMessage()
		if err != nil {
			break
		}

		if err := conn.WriteMessage(mt, msg); err != nil {
			break
		}
	}
}
