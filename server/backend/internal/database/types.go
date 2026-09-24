package database

type Campaign struct {
	UUID      string `json:"uuid"`
	Name      string `json:"name"`
	CreatedAt string `json:"created_at"`
	TotalAgents int `json:"total_agents"`
	TotalFiles int `json:"total_files"`
}

type CampaignsResp struct {
	Total     int        `json:"total"`
	Campaigns []Campaign `json:"campaigns"`
}


type Agent struct {
	UUID string `json:"uuid"`
	Username string `json:"username"`
	Hostname string `json:"hostname"`
	Domain string `json:"domain"`
	Arch   string `json:"arch"`
	Country string `json:"country"`
}

type Agents struct {
	Total int `json:"total"`
	Agents []Agent `json:"agents"`
}