package mobile

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/settings"
)

type UserSettingsView struct {
	Status    string                `json:"status"`
	RequestID string                `json:"request_id"`
	Settings  settings.UserSettings `json:"settings"`
}
