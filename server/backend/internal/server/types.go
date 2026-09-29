package server

const (
	CODE_CHECK_IN = 0xac
	CODE_REGISTER = 0xab
	MSG_OUTPUT    = 0xad

	OUTPUT_RAW  = 0x0
	OUTPUT_TEXT = 0x1
	OUTPUT_LS   = 0x2
	OUTPUT_CAT  = 0x3

	TASK_NO_TASK = 0xff
)

type LoginReq struct {
	Username string `json:"username"`
	Password string `json:"password"`
}

type LoginResp struct {
	Token    string `json:"token"`
	RstToken string `json:"rst_token"`
}

type NewCampaignReq struct {
	Name string `json:"name"`
}
