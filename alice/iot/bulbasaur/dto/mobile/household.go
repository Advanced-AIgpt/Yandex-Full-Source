package mobile

import (
	"context"
	"sort"

	sharingmodel "a.yandex-team.ru/alice/iot/bulbasaur/controller/sharing/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/suggestions"
	"a.yandex-team.ru/alice/library/go/geosuggest"
	"a.yandex-team.ru/library/go/ptr"
	"a.yandex-team.ru/library/go/valid"
)

type UserHouseholdsView struct {
	Status      string                         `json:"status"`
	RequestID   string                         `json:"request_id"`
	Households  []HouseholdView                `json:"households"`
	Invitations []HouseholdInvitationShortView `json:"invitations"`
}

func (view *UserHouseholdsView) FromHouseholds(currentHouseholdID string, households []model.Household, invitationSenders sharingmodel.Users, invitations model.HouseholdInvitations) {
	view.Households = make([]HouseholdView, 0, len(households))
	for _, h := range households {
		var householdView HouseholdView
		householdView.From(currentHouseholdID, h)
		view.Households = append(view.Households, householdView)
	}
	sendersMap := invitationSenders.ToMap()
	view.Invitations = make([]HouseholdInvitationShortView, 0, len(invitations))
	for _, invitation := range invitations {
		var invitationView HouseholdInvitationShortView
		sender, exist := sendersMap[invitation.SenderID]
		if !exist {
			continue
		}
		invitationView.From(invitation, sender)
		view.Invitations = append(view.Invitations, invitationView)
	}
	sort.Sort(HouseholdViewSorting(view.Households))
	sort.Sort(HouseholdInvitationShortViewSorting(view.Invitations))
}

type HouseholdView struct {
	ID          string                 `json:"id"`
	Name        string                 `json:"name"`
	Location    *HouseholdLocationView `json:"location,omitempty"`
	IsCurrent   bool                   `json:"is_current"`
	IsRemovable *bool                  `json:"is_removable,omitempty"`
}

type HouseholdWithLocationSuggestsView struct {
	HouseholdView
	HouseholdLocationSuggestsView
	HasGuests bool `json:"has_guests,omitempty"`
}

func (view *HouseholdView) From(currentHouseholdID string, household model.Household) {
	view.ID = household.ID
	view.Name = household.Name
	if household.Location != nil {
		var location HouseholdLocationView
		location.FromHouseholdLocation(*household.Location)
		view.Location = &location
	}
	if currentHouseholdID == view.ID {
		view.IsCurrent = true
	}
}

type UserHouseholdsDeviceConfigurationView struct {
	Status     string                             `json:"status"`
	RequestID  string                             `json:"request_id"`
	Households []HouseholdDeviceConfigurationView `json:"households"`
}

func (view *UserHouseholdsDeviceConfigurationView) From(currentHouseholdID string, households []model.Household, deviceCurrentHouseholdID string) {
	view.Households = make([]HouseholdDeviceConfigurationView, 0, len(households))
	for _, h := range households {
		var householdView HouseholdDeviceConfigurationView
		householdView.FromHousehold(h, deviceCurrentHouseholdID)
		view.Households = append(view.Households, householdView)
	}
	householdSorting := HouseholdDeviceConfigurationViewSorting{
		Households:         view.Households,
		CurrentHouseholdID: currentHouseholdID,
	}
	sort.Sort(&householdSorting)
	view.Households = householdSorting.Households
}

type HouseholdDeviceConfigurationView struct {
	ID       string                 `json:"id"`
	Name     string                 `json:"name"`
	Location *HouseholdLocationView `json:"location,omitempty"`
	IsActive *bool                  `json:"is_active,omitempty"`
}

func (view *HouseholdDeviceConfigurationView) FromHousehold(household model.Household, deviceCurrentHouseholdID string) {
	view.ID = household.ID
	view.Name = household.Name
	if household.Location != nil {
		var location HouseholdLocationView
		location.FromHouseholdLocation(*household.Location)
		view.Location = &location
	}
	if view.ID == deviceCurrentHouseholdID {
		view.IsActive = ptr.Bool(true)
	}
}

type HouseholdLocationValidateRequest struct {
	Address *string `json:"address,omitempty"`
}

func (req HouseholdLocationValidateRequest) ValidateAddressByGeosuggest(ctx context.Context, geoClient geosuggest.IClient) (*model.HouseholdLocation, error) {
	if req.Address == nil {
		return &model.HouseholdLocation{}, &model.UserHouseholdEmptyAddressError{}
	}
	return validateAddressByGeosuggest(ctx, *req.Address, geoClient)
}

type HouseholdNameValidateRequest struct {
	Name string `json:"name"`
}

func (req HouseholdNameValidateRequest) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	//name
	household := model.Household{Name: req.Name}
	if e := household.AssertName(); e != nil {
		err = append(err, e)
	}

	if len(err) == 0 {
		return false, nil
	}
	return false, err
}

type HouseholdCreateRequest struct {
	HouseholdNameValidateRequest
	HouseholdLocationValidateRequest
}

func (req HouseholdCreateRequest) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	//name
	if _, e := req.HouseholdNameValidateRequest.Validate(vctx); e != nil {
		if ves, ok := e.(valid.Errors); ok {
			err = append(err, ves...)
		} else {
			err = append(err, e)
		}
	}

	if len(err) == 0 {
		return false, nil
	}
	return false, err
}

func (req HouseholdCreateRequest) ToHousehold(location *model.HouseholdLocation) model.Household {
	return model.Household{
		Name:     req.Name,
		Location: location,
	}
}

type SetCurrentHouseholdRequest struct {
	ID string `json:"id"`
}

type UserHouseholdView struct {
	Status    string                            `json:"status"`
	RequestID string                            `json:"request_id"`
	Household HouseholdWithLocationSuggestsView `json:"household"`
}

func (view *UserHouseholdView) From(
	currentHouseholdID string,
	household model.Household,
	householdDevices model.Devices,
	households model.Households,
	locationSuggests []model.HouseholdLocation,
	residents model.HouseholdResidents,
) {
	view.Household.From(currentHouseholdID, household)
	view.Household.FillLocationSuggests(locationSuggests)
	view.Household.IsRemovable = ptr.Bool(len(householdDevices) == 0 && len(households) > 1)
	view.Household.HasGuests = household.SharingInfo == nil && len(residents.GuestsOrPendingInvitations()) > 0
}

type HouseholdInfoView struct {
	ID          string                 `json:"id"`
	Name        string                 `json:"name"`
	Location    *HouseholdLocationView `json:"location,omitempty"`
	IsCurrent   bool                   `json:"is_current"`
	SharingInfo *SharingInfoView       `json:"sharing_info,omitempty"`
}

func (view *HouseholdInfoView) FromHousehold(household model.Household, currentHouseholdID string) {
	view.ID = household.ID
	view.Name = household.Name
	view.SharingInfo = NewSharingInfoView(household.SharingInfo)
	if household.Location != nil {
		var location HouseholdLocationView
		location.FromHouseholdLocation(*household.Location)
		view.Location = &location
	}
	if household.ID == currentHouseholdID {
		view.IsCurrent = true
	}
}

type HouseholdLocationView struct {
	Address      string `json:"address"`
	ShortAddress string `json:"short_address"`
}

func (view *HouseholdLocationView) FromHouseholdLocation(location model.HouseholdLocation) {
	view.Address = location.Address
	view.ShortAddress = location.ShortAddress
}

type HouseholdNameEditView struct {
	Status    string   `json:"status"`
	RequestID string   `json:"request_id"`
	Suggests  []string `json:"suggests"`
}

func (view *HouseholdNameEditView) FillSuggests() {
	view.Suggests = suggestions.HouseholdNames
	sort.Strings(view.Suggests)
}

type HouseholdAddView struct {
	Status    string `json:"status"`
	RequestID string `json:"request_id"`
	HouseholdLocationSuggestsView
}

type HouseholdLocationSuggestsView struct {
	LocationSuggests []HouseholdLocationView `json:"location_suggests"`
}

func (view *HouseholdLocationSuggestsView) FillLocationSuggests(locationSuggests []model.HouseholdLocation) {
	view.LocationSuggests = make([]HouseholdLocationView, 0)
	if len(locationSuggests) == 0 {
		return
	}
	view.LocationSuggests = make([]HouseholdLocationView, 0, len(locationSuggests))
	for _, suggest := range locationSuggests {
		var locationView HouseholdLocationView
		locationView.FromHouseholdLocation(suggest)
		view.LocationSuggests = append(view.LocationSuggests, locationView)
	}
}

type HouseholdCreateResponse struct {
	Status      string `json:"status"`
	RequestID   string `json:"request_id"`
	HouseholdID string `json:"household_id"`
}

type HouseholdFavoriteView struct {
	ID        string `json:"id"`
	Name      string `json:"name"`
	IsCurrent bool   `json:"is_current,omitempty"`
}

func (v *HouseholdFavoriteView) FromHousehold(currentHouseholdID string, household model.Household) {
	v.ID = household.ID
	v.Name = household.Name
	v.IsCurrent = currentHouseholdID == v.ID
}

func GetValidLocationSuggests(ctx context.Context, locationSuggests []model.HouseholdLocation, geoClient geosuggest.IClient) []model.HouseholdLocation {
	res := make([]model.HouseholdLocation, 0, len(locationSuggests))
	for _, suggest := range locationSuggests {
		location, err := validateAddressByGeosuggest(ctx, suggest.Address, geoClient)
		if err == nil {
			res = append(res, *location)
		}
	}
	return res
}

func validateAddressByGeosuggest(ctx context.Context, address string, geoClient geosuggest.IClient) (*model.HouseholdLocation, error) {
	if address == "" {
		return nil, &model.UserHouseholdEmptyAddressError{}
	}
	suggests, err := geoClient.GetGeosuggestFromAddress(ctx, address)
	if err != nil {
		return nil, err
	}
	gs, hasAddress := suggests.HasValidAddress(address, []string{"Россия"})
	if !hasAddress {
		return nil, &model.UserHouseholdInvalidAddressError{}
	}
	var location model.HouseholdLocation
	if err := location.FromGeosuggest(gs); err != nil {
		return nil, err
	}
	return &location, nil
}
