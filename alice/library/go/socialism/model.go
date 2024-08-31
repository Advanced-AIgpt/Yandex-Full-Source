package socialism

type Consumer string

func (c Consumer) String() string {
	return string(c)
}

var (
	QuasarConsumer Consumer = "quasar"
)

func NewSkillInfo(skillID, applicationName string, trusted bool) SkillInfo {
	return SkillInfo{
		skillID:         skillID,
		applicationName: applicationName,
		trusted:         trusted,
	}
}

type SkillInfo struct {
	skillID         string
	applicationName string
	trusted         bool
}

type tokenInfo struct {
	ProfileID   int `json:"debug_string"`
	Application string
	Scope       string
	Secret      string
	TokenID     int `json:"token_id"`
	Value       string
	Expired     string
	ExpiredTS   int `json:"expired_ts"`
	Created     string
	CreatedTS   int `json:"created_ts"`
}

type errorResponse struct {
	Description string
	Name        string
	RequestID   string `json:"request_id"`
}

func (r errorResponse) Empty() bool {
	return r == errorResponse{}
}
