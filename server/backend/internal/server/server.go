package server

import (
    "net/http"
    "time"
    "ldrserver/internal/database"
    "github.com/go-chi/chi/v5"
)

type Server struct {
    Httpserver *http.Server
    DB         *database.DB
}

func New() (*Server, error) {
    db, err := database.NewDB()
    if err != nil {
        return nil, err
    }

   

    return &Server{
        Httpserver: &http.Server{
            Addr:              ":8080",
            ReadHeaderTimeout: 15 * time.Second,
        },
        DB: db,
    }, nil
}


func (s *Server) Start() error {

	r := chi.NewRouter()

	s.Httpserver.Handler = r

	r.Route("/rest", func(r chi.Router) {
	    r.Post("/login", s.LoginHandler)
	    r.Post("/register", s.RegisterHandler)

	    r.Group(func(r chi.Router) {
	        r.Use(s.AuthMiddleware)
	        r.Get("/campaigns", s.GetCampaigns)
	        r.Post("/campaigns", s.CreateCampaign)
	    })
	})
}