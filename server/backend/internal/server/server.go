package server


import (
	"github.com/go-chi/chi/v5"
	"time"
	"ldrserver/internal/database"
)


type Server struct {
	Httpserver *http.Server
	DB         *database.DB

}


func New() (*server, error) {

	db, err := database.NewDB()

	return &Server {
		Httpserver: http.Server{
			Addr: "0.0.0.0",
			ReadHeaderTimeout: 15 * time.Second,
			WriteTimeout: 0,
			IdleTimeout: 0,
		}
	}

}