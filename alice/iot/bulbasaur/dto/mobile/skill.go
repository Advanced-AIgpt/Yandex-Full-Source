package mobile

import (
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/dialogs"
)

type ProviderSkillInfo struct {
	RequestID          string                   `json:"request_id"`
	Status             string                   `json:"status"`
	StatusErrorMessage string                   `json:"status_error_message,omitempty"`
	IsBound            bool                     `json:"is_bound"`
	ApplicationName    string                   `json:"application_name"`
	SkillID            string                   `json:"skill_id"`
	LogoURL            string                   `json:"logo_url"`
	Name               string                   `json:"name"`
	Description        string                   `json:"description"`
	DeveloperName      string                   `json:"developer_name"`
	SecondaryTitle     string                   `json:"secondary_title"`
	Trusted            bool                     `json:"trusted"`
	RatingHistogram    []int                    `json:"rating_histogram"`
	AverageRating      float64                  `json:"average_rating"`
	UserReview         *ProviderSkillUserReview `json:"user_review"`
	CertifiedDevices   struct {
		Categories []CertifiedCategory `json:"categories"`
	} `json:"certified_devices"`
	LinkedDevices []ProviderSkillDeviceView `json:"linked_devices"`
}

type ProvidersListResponse struct {
	RequestID  string                   `json:"request_id"`
	Status     string                   `json:"status"`
	Message    string                   `json:"message,omitempty"`
	Skills     []ProviderSkillShortInfo `json:"skills"`
	UserSkills []ProviderSkillShortInfo `json:"user_skills"`
}

type ProviderSkillShortInfo struct {
	SkillID          string                  `json:"skill_id"`
	Name             string                  `json:"name"`
	Description      string                  `json:"description"`
	SecondaryTitle   string                  `json:"secondary_title"`
	LogoURL          string                  `json:"logo_url"`
	Private          bool                    `json:"private"`
	Trusted          bool                    `json:"trusted"`
	AverageRating    float64                 `json:"average_rating"`
	Status           string                  `json:"status,omitempty"`
	DiscoveryMethods []model.DiscoveryMethod `json:"discovery_methods"`
}

type ProviderSkillUserReview struct {
	Rating       int      `json:"rating"`
	ReviewText   string   `json:"review_text"`
	QuickAnswers []string `json:"quick_answers"`
}

type CertifiedCategory struct {
	DevicesCount int    `json:"devices_count"`
	BrandsIds    []int  `json:"brands_ids"`
	Name         string `json:"name"`
	ID           int    `json:"id"`
}

func (p ProviderSkillShortInfo) GetInnerRating() int {
	innerRating := 999
	if rating, hasInnerRating := skillsInnerRatingMap[p.SkillID]; hasInnerRating {
		innerRating = rating
	}
	return innerRating
}

func (psi *ProviderSkillInfo) FromSkillInfo(skillInfo dialogs.SkillInfo, linkedDevices model.Devices) {
	psi.SkillID = skillInfo.SkillID
	psi.ApplicationName = skillInfo.ApplicationName
	psi.Name = skillInfo.Name
	psi.Description = skillInfo.Description
	psi.DeveloperName = skillInfo.DeveloperName
	psi.SecondaryTitle = skillInfo.SecondaryTitle
	psi.Trusted = skillInfo.Trusted
	psi.RatingHistogram = skillInfo.RatingHistogram
	psi.AverageRating = skillInfo.AverageRating

	var providerSkillUserReview *ProviderSkillUserReview
	if skillInfo.UserReview != nil {
		providerSkillUserReview = &ProviderSkillUserReview{
			Rating:       skillInfo.UserReview.Rating,
			ReviewText:   skillInfo.UserReview.ReviewText,
			QuickAnswers: skillInfo.UserReview.QuickAnswers,
		}
	}
	psi.UserReview = providerSkillUserReview

	certifiedDevices := make([]CertifiedCategory, 0, len(skillInfo.CertifiedDevices.Categories))
	for _, category := range skillInfo.CertifiedDevices.Categories {
		certifiedCategory := CertifiedCategory{
			DevicesCount: category.DevicesCount,
			BrandsIds:    category.BrandIds,
			Name:         category.Name,
			ID:           category.ID,
		}
		certifiedDevices = append(certifiedDevices, certifiedCategory)
	}
	psi.CertifiedDevices.Categories = certifiedDevices
	psi.LinkedDevices = make([]ProviderSkillDeviceView, 0, len(linkedDevices))
	for _, linkedDevice := range linkedDevices {
		var view ProviderSkillDeviceView
		view.FromDevice(linkedDevice)
		psi.LinkedDevices = append(psi.LinkedDevices, view)
	}
	sort.Sort(ProviderSkillDeviceViewSorting(psi.LinkedDevices))
}

type ProviderSkillDeviceView struct {
	ID       string           `json:"id"`
	Name     string           `json:"name"`
	Type     model.DeviceType `json:"type"`
	RoomName string           `json:"room_name,omitempty"`
}

func (v *ProviderSkillDeviceView) FromDevice(device model.Device) {
	v.ID = device.ID
	v.Name = device.Name
	v.Type = device.Type
	if device.Room != nil {
		v.RoomName = device.Room.Name
	}
}
