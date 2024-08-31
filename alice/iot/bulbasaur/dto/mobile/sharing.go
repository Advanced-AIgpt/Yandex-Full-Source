package mobile

import (
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/settings"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sharing/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

type AcceptSharingRequest struct {
	DeviceID  string `json:"device_id"`  // ID of sharing device
	OwnerPUID uint64 `json:"owner_puid"` // PUID of owner of the shared device
}

type RevokeSharingRequest struct {
	DeviceID  string `json:"device_id"`  // ID of sharing device
	OwnerPUID uint64 `json:"owner_puid"` // PUID of owner of the shared device
}

type VoiceprintStatus string

const (
	NonAvailableVoiceprintStatus VoiceprintStatus = "non_available" // Example: non-yandex speakers
	AvailableVoiceprintStatus    VoiceprintStatus = "available"     // Example: yandex speakers that do not have user voiceprint already
	SavedVoiceprintStatus        VoiceprintStatus = "saved"         // no comments
)

type VoiceprintMethod string

const (
	SoundVoiceprintMethod     VoiceprintMethod = "sound"
	DirectiveVoiceprintMethod VoiceprintMethod = "directive"
)

type VoiceprintView struct {
	Status VoiceprintStatus `json:"status"`
	Method VoiceprintMethod `json:"method,omitempty"`
}

func NewVoiceprintView(device model.Device, voiceprintDeviceConfigs settings.VoiceprintDeviceConfigs) *VoiceprintView {
	if !device.Type.IsSmartSpeaker() {
		return nil
	}
	if !model.VoiceprintSpeakers[device.Type] {
		return &VoiceprintView{
			Status: NonAvailableVoiceprintStatus,
		}
	}
	status := AvailableVoiceprintStatus
	method := DirectiveVoiceprintMethod
	_, exist := voiceprintDeviceConfigs.GetByDeviceID(device.ID)
	if exist {
		status = SavedVoiceprintStatus
	}
	if model.VoiceprintViaSoundSpeakers[device.Type] {
		method = SoundVoiceprintMethod
	}
	return &VoiceprintView{
		Status: status,
		Method: method,
	}
}

type HouseholdResidentsResponse struct {
	Status    string                  `json:"status"`
	RequestID string                  `json:"request_id"`
	Residents []HouseholdResidentView `json:"residents"`
}

func (r *HouseholdResidentsResponse) FromHouseholdResidents(residents sharingmodel.HouseholdResidents) {
	r.Residents = make([]HouseholdResidentView, 0, len(residents))
	for _, resident := range residents {
		var view HouseholdResidentView
		view.FromHouseholdResident(resident)
		r.Residents = append(r.Residents, view)
	}
	sort.Sort(HouseholdResidentViewSorting(r.Residents))
}

type HouseholdResidentView struct {
	SharingUserView
	Role model.HouseholdRole `json:"role"`
}

func (v *HouseholdResidentView) FromHouseholdResident(resident sharingmodel.HouseholdResident) {
	v.FromSharingUser(resident.User)
	v.Role = resident.Role
}

type DeleteGuestsFromHouseholdRequest struct {
	GuestIDs []uint64 `json:"guest_ids"`
}

type SharingInfoView struct {
	OwnerID uint64 `json:"owner_id"`
}

func NewSharingInfoView(sharingInfo *model.SharingInfo) *SharingInfoView {
	if sharingInfo == nil {
		return nil
	}
	return &SharingInfoView{
		OwnerID: sharingInfo.OwnerID,
	}
}

type SharingUserView struct {
	ID          uint64 `json:"id"`
	Login       string `json:"login,omitempty"`
	Email       string `json:"email,omitempty"`
	AvatarURL   string `json:"avatar_url,omitempty"`
	DisplayName string `json:"display_name,omitempty"`
	PhoneNumber string `json:"phone_number,omitempty"`
	YandexPlus  bool   `json:"yandex_plus"`
}

func (v *SharingUserView) FromSharingUser(user sharingmodel.User) {
	v.ID = user.ID
	v.Login = user.Login
	v.Email = user.Email
	v.AvatarURL = user.AvatarURL
	v.DisplayName = user.DisplayName
	v.PhoneNumber = user.PhoneNumber
	v.YandexPlus = user.YandexPlus
}

type HouseholdGetSharingLinkResponse struct {
	Status                 string `json:"status"`
	RequestID              string `json:"request_id"`
	Link                   string `json:"link"`
	ExpirationTime         string `json:"expiration_time"`
	PreviousExpirationTime string `json:"previous_expiration_time"`
}

func (r *HouseholdGetSharingLinkResponse) FromSharingLink(link model.HouseholdSharingLink, previousExpirationTime timestamp.PastTimestamp) {
	r.Link = link.ConstructSharingLink()
	r.ExpirationTime = formatTimestamp(link.ExpireAt)
	r.PreviousExpirationTime = formatTimestamp(previousExpirationTime)
}

type HouseholdAcceptSharingLinkRequest struct {
	Token string `json:"token"`
}

type HouseholdAcceptSharingLinkResponse struct {
	Status     string                  `json:"status"`
	RequestID  string                  `json:"request_id"`
	Invitation HouseholdInvitationView `json:"invitation"`
}

func (r *HouseholdAcceptSharingLinkResponse) From(invitation model.HouseholdInvitation, sender sharingmodel.User, household model.Household) {
	r.Invitation.From(invitation, sender, household)
}

type HouseholdAcceptSharingInvitationRequest struct {
	HouseholdName string `json:"household_name"`
}

type HouseholdInvitationView struct {
	ID        string          `json:"id"`
	Sender    SharingUserView `json:"sender"`
	Household HouseholdView   `json:"household"`
}

func (v *HouseholdInvitationView) From(invitation model.HouseholdInvitation, sender sharingmodel.User, household model.Household) {
	v.Sender.FromSharingUser(sender)
	v.ID = invitation.ID
	v.Household.From("", household)
}

type HouseholdInvitationShortView struct {
	ID     string          `json:"id"`
	Sender SharingUserView `json:"sender"`
}

func (v *HouseholdInvitationShortView) From(invitation model.HouseholdInvitation, sender sharingmodel.User) {
	v.ID = invitation.ID
	v.Sender.FromSharingUser(sender)
}

type GetHouseholdInvitationResponse struct {
	Status     string                  `json:"status"`
	RequestID  string                  `json:"request_id"`
	Invitation HouseholdInvitationView `json:"invitation"`
}

func (r *GetHouseholdInvitationResponse) From(invitation model.HouseholdInvitation, sender sharingmodel.User, household model.Household) {
	r.Invitation.From(invitation, sender, household)
}

type RevokeInvitationsRequest struct {
	GuestIDs []uint64 `json:"guest_ids"`
}

type HouseholdInvitationsListResponse struct {
	Status      string                         `json:"status"`
	RequestID   string                         `json:"request_id"`
	Invitations []HouseholdInvitationShortView `json:"invitations"`
}

func (r *HouseholdInvitationsListResponse) FromInvitations(invitations model.HouseholdInvitations, users sharingmodel.Users) {
	r.Invitations = make([]HouseholdInvitationShortView, 0, len(invitations))
	usersMap := users.ToMap()
	for _, invitation := range invitations {
		var view HouseholdInvitationShortView
		sender, ok := usersMap[invitation.SenderID]
		if !ok {
			continue
		}
		view.From(invitation, sender)
		r.Invitations = append(r.Invitations, view)
	}
	sort.Sort(HouseholdInvitationShortViewSorting(r.Invitations))
}
