package server

import (
	"net/http"
	"encoding/json"
	"ldrserver/internal/database"
)


func (s *Server) LoginHandler(w http.ResponseWriter, r *http.Request) {
	var login LoginReq
	if err := json.NewDecoder(r.Body).Decode(&login); err != nil {
		return <503>
	}

	ok, err := s.DB.VerifyCreds(login.Username, login.Password)
	if err != nil {
		return  <503>
	}
	if !ok {
		return <401>
	}

	<call createJWT> return jwt in json


}


func (s *Server) AuthMiddleware(w http.ResponseWriter, r *http.Request) {

}