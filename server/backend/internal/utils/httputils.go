package utils

import (
	"net/http"
	"encoding/json"
)


func writeJson(w http.ResponseWriter, msg string, status int) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	json.NewEncoder(w).Encode(map[string]string{"msg": msg})
} 


func Return500(w http.ResponseWriter, msg string) {
	writeJson(w, msg, http.StatusInternalServerError)
} 

func Return401(w http.ResponseWriter, msg string) {
	writeJson(w, msg, http.StatusUnauthorized)
} 


