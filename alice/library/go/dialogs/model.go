package dialogs

type AuthorizationData struct {
	UserID  uint64
	SkillID string
	Success bool
}

type SkillInfo struct {
	UserID           uint64
	SkillID          string
	ApplicationName  string
	ClientID         string
	BackendURL       string
	Name             string
	Description      string
	DeveloperName    string
	LogoAvatarID     string
	SecondaryTitle   string
	Trusted          bool
	Public           bool
	FunctionID       string
	Channel          string
	RatingHistogram  []int
	AverageRating    float64
	UserReview       *SkillUserReview
	CertifiedDevices CertifiedDevices
}

type SkillShortInfo struct {
	SkillID        string  `json:"skill_id"`
	Name           string  `json:"name"`
	SecondaryTitle string  `json:"secondary_title"`
	LogoURL        string  `json:"logo_url"`
	Private        bool    `json:"private"`
	Trusted        bool    `json:"trusted"`
	AverageRating  float64 `json:"averageRating"`
}

type Skill struct {
	ID             string
	Name           string
	Description    string
	SecondaryTitle string `json:"secondary_title"`
	LogoURL        string `json:"logo_url"`
	Private        bool
	Trusted        bool
	AverageRating  float64 `json:"averageRating"`
}

func (skill *Skill) toSkillShortInfo() SkillShortInfo {
	return SkillShortInfo{
		SkillID:        skill.ID,
		Name:           skill.Name,
		SecondaryTitle: skill.Description,
		LogoURL:        skill.LogoURL,
		Private:        skill.Private,
		AverageRating:  skill.AverageRating,
		Trusted:        skill.Trusted,
	}
}

type SkillUserReview struct {
	Rating       int      `json:"rating"`
	ReviewText   string   `json:"reviewText"`
	QuickAnswers []string `json:"quickAnswers"`
}

// see https://wiki.yandex-team.ru/dialogs/development/external-api-v2/
type dialogResponse struct {
	ID    string
	Error struct {
		Message string
	}
	UserID         string
	Name           string
	Channel        string
	Description    string
	DeveloperName  string
	AccountLinking struct {
		ApplicationName string
		ClientID        string
	}
	Logo struct {
		AvatarID string
		Color    string
	}
	BackendURL     string
	SecondaryTitle string
	Trusted        bool
	SkillAccess    string `json:"skillAccess"`
	FunctionID     string

	RatingHistogram []int            `json:"ratingHistogram"`
	AverageRating   float64          `json:"averageRating"`
	UserReview      *SkillUserReview `json:"userReview"`

	AccessToSkillTesting struct {
		HasAccess bool
		Role      string
	}
}

type CertifiedCategory struct {
	DevicesCount int
	BrandIds     []int
	Name         string
	ID           int
}

type CertifiedDevices struct {
	Categories []CertifiedCategory
}

type certifiedDevicesResponse struct {
	Message          string
	CertifiedDevices CertifiedDevices
}
