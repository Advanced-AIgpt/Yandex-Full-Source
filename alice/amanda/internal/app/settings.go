package app

import (
	"encoding/json"
	"errors"

	"a.yandex-team.ru/alice/amanda/internal/session"
)

var ErrAccountNotFound = errors.New("account not found")

type Settings struct {
	data *session.Settings
}

func (s *Settings) GetDeviceState() (deviceState map[string]interface{}) {
	deviceState = map[string]interface{}{}
	if err := json.Unmarshal([]byte(s.data.DeviceState), &deviceState); err != nil {
		return map[string]interface{}{}
	}
	return
}

func (s *Settings) SaveDeviceState(deviceState map[string]interface{}) error {
	data, err := json.Marshal(deviceState)
	if err != nil {
		return err
	}
	s.data.DeviceState = string(data)
	return nil
}

func (s *Settings) GetLocation() *session.Location {
	return &s.data.Location
}

func (s *Settings) GetAccountDetails() *AccountDetails {
	return &AccountDetails{&s.data.AccountDetails}
}

func (s *Settings) GetApplicationDetails() *session.ApplicationDetails {
	return &s.data.ApplicationDetails
}

func (s *Settings) GetSystemDetails() *session.SystemDetails {
	return &s.data.SystemDetails
}

func (s *Settings) GetExperiments() map[string]session.Experiment {
	return s.data.Experiments
}

func (s *Settings) GetQueryParams() map[string]session.QueryParam {
	return s.data.QueryParams
}

func (s *Settings) GetSupportedFeatures() map[string]bool {
	return s.data.SupportedFeatures
}

func (s *Settings) GetRegionID() *int32 {
	return s.data.RegionID
}

func (s *Settings) SetRegionID(id int32) {
	s.data.RegionID = &id
}

func (s *Settings) ResetRegionID() {
	s.data.RegionID = nil
}

type AccountDetails struct {
	data *session.AccountDetails
}

func (details *AccountDetails) GetAccounts() []session.AccountInfo {
	accounts := make([]session.AccountInfo, 0, len(details.data.Accounts))
	for _, acc := range details.data.Accounts {
		accounts = append(accounts, acc)
	}
	return accounts
}

func (details *AccountDetails) GetActiveAccount() *session.AccountInfo {
	if len(details.data.Accounts) == 0 || details.data.Active == nil {
		return nil
	}
	if acc, ok := details.data.Accounts[*details.data.Active]; ok {
		return &acc
	}
	return nil
}

func (details *AccountDetails) SetActiveAccount(username string) error {
	if details.data.Accounts != nil {
		if _, ok := details.data.Accounts[username]; ok {
			details.data.Active = &username
			return nil
		}
	}
	return ErrAccountNotFound
}

func (details *AccountDetails) AddAccount(acc session.AccountInfo) {
	if details.data.Accounts == nil {
		details.data.Accounts = map[string]session.AccountInfo{}
	}
	details.data.Accounts[acc.Username] = acc
}

func (details *AccountDetails) RemoveAccount(username string) error {
	if details.data.Accounts == nil {
		return ErrAccountNotFound
	}
	if _, ok := details.data.Accounts[username]; ok {
		if active := details.GetActiveAccount(); active != nil && active.Username == username {
			details.ClearActiveAccount()
		}
		delete(details.data.Accounts, username)
	} else {
		return ErrAccountNotFound
	}
	return nil
}

func (details *AccountDetails) ClearActiveAccount() {
	details.data.Active = nil
}
