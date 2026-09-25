package server

import (
    "fmt"
    "io/fs"
    "net/http"
    "time"
    "ldrserver/internal/assets"
    "ldrserver/internal/database"
    "ldrserver/internal/ws"
    "github.com/go-chi/chi/v5"
    "github.com/spf13/pflag"
)

type Server struct {
    Httpserver *http.Server
    DB         *database.DB
    WS         *ws.WS
    Addr string
    Port int
    CertPath string
    KeyPath string
}

func New() (*Server, error) {
    db, err := database.NewDB()
    if err != nil {
        return nil, err
    }

    hub := ws.New(db)

    return &Server{
        Httpserver: &http.Server{
            Addr:              ":8080",
            ReadHeaderTimeout: 15 * time.Second,
        },
        DB: db,
        WS: hub,
    }, nil
}


func (s *Server) Parse(args []string) error {
    fs := pflag.NewFlagSet("run", pflag.ContinueOnError)
    fs.StringVar(&s.Addr, "addr", "127.0.0.1", "listen address(e.g 0.0.0.0, 127.0.0.1")
    fs.IntVar(&s.Port, "port", 443, "port to listen on")
    fs.StringVar(&s.CertPath, "cert", "", "HTTPS cert to use")
    fs.StringVar(&s.KeyPath, "key", "", "HTTPS key to use")

    return fs.Parse(args[2:])
}

func (s *Server) Start() error {

	r := chi.NewRouter()

	s.Httpserver.Handler = r

	r.Route("/rest", func(r chi.Router) {
	    r.Post("/login", s.LoginHandler)
        r.Post("/checkin", s.HandleAgentCheckin)

	    r.Group(func(r chi.Router) {
	        r.Use(s.AuthMiddleware)
            r.Get("/ws", s.WS.Handler)
	        r.Get("/campaigns", s.GetCampaigns)
	        r.Post("/campaigns", s.CreateCampaign)
	        r.Delete("/campaigns/{id}", s.DeleteCampaign)

            r.Get("/agents/{id}", s.GetAgents)
            r.Delete("/agents/{id}", s.DeleteAgent)

            

            r.Get("/files/{campaignID}", s.ListFiles)
            r.Post("/files/{campaignID}", s.UploadFile)
            r.Delete("/files/{id}", s.DeleteFile)
	    })
	})


    addr := fmt.Sprintf("%s:%d", s.Addr, s.Port)
   
    staticFS, err := fs.Sub(assets.StaticFiles, "static")
    if err != nil {
        return err
    }
    fileServer := http.FileServer(http.FS(staticFS))

    r.Get("/*", func(w http.ResponseWriter, r *http.Request) {
        _, err := staticFS.Open(r.URL.Path[1:])
        if err != nil {
            r.URL.Path = "/"
        }
        fileServer.ServeHTTP(w, r)
    })

    fmt.Printf("[*] Listening on %s\n", addr)
    return http.ListenAndServe(addr, r)
}