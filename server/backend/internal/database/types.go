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
	IsElev int `json:"is_elev"`
	Country string `json:"country"`
	LastSeen string `json:"last_seen"`
}

type Agents struct {
	Total int `json:"total"`
	Agents []Agent `json:"agents"`
}

type File struct {
	UUID         string `json:"uuid"`
	CampaignUUID string `json:"campaign_uuid"`
	Name         string `json:"name"`
	Size         int64  `json:"size"`
	Kind         string `json:"kind"`
	Arch         string `json:"arch"`
	HasReloc     bool   `json:"has_reloc"`
	SHA256       string `json:"sha256"`
	CreatedAt    string `json:"created_at"`
}

type Files struct {
	Total int    `json:"total"`
	Files []File `json:"files"`
}

type FileResp struct {
	UUID string `json:"uuid"`
	Name string `json:"name"`
}

type FileListResp struct {
	Total int        `json:"total"`
	Files []FileResp `json:"files"`
}


type AgentInfo struct {
	Username string `json:"username"`
	Hostname string `json:"hostname"`
	Domain   string `json:"domain"`
	Process  string `json:"process"`
	Arch     string `json:"arch"`
	Country  string `json:"country"`
	IsElev   int    `json:"is_elev"`

}