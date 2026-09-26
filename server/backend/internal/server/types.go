package server


const (
	CODE_CHECK_IN = 0xac
	CODE_REGISTER = 0xab

	TASK_NO_TASK  = 0xff
)

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