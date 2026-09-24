package server



type LoginReq struct {
	Username string `json:"username"`
	Password string `json:"password"`
}

type LoginResp struct {
	Token    string  `json:"token"`
	RstToken string `json:"rst_token"`
}


type NewCampaignReq struct {
	Name string `json:"name"`
}