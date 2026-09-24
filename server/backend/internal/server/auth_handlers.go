package server

import (
	"encoding/json"
	"net/http"
	"ldrserver/internal/auth"
	"ldrserver/internal/utils"
	"fmt"
)


func (s *Server) LoginHandler(w http.ResponseWriter, r *http.Request) {
	var login LoginReq
	if err := json.NewDecoder(r.Body).Decode(&login); err != nil {
		utils.Return500(w, "failed decoding request")
		return
	}

	ok, err := s.DB.VerifyCreds(login.Username, login.Password)
	if err != nil {
		utils.Return500(w, "failed database lookup")
		return
	}
	if !ok {
		utils.Return401(w, "invalid creds")
		return
	}

	token, err := auth.CreateJWT(login.Username)
	if err != nil {
		utils.Return500(w, "failed creating token")
		return
	}
	fmt.Printf("[*] Login From: %s | %s\n", login.Username, login.Password)

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(map[string]string{"jwt": token})
	return


}


func (s *Server) AuthMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		tokenStr := r.Header.Get("Authorization")
		if tokenStr == "" {
			tokenStr = r.URL.Query().Get("token")
		}
		if tokenStr == "" {
			utils.Return401(w, "missing token")
			return
		}
		_, err := auth.VerfiyJWT(tokenStr)
		if err != nil {
			utils.Return401(w, "invalid token")
			return
		}
		next.ServeHTTP(w, r)
	})
}

