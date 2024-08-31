package data

import (
	"github.com/gofrs/uuid"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/library/go/random"
	"a.yandex-team.ru/alice/library/go/tools"
)

func GenerateStereopair(leader, follower model.Device) model.Stereopair {
	id := uuid.Must(uuid.NewV4()).String()
	sp := model.Stereopair{
		ID:   id,
		Name: tools.StandardizeSpaces(testing.RandCyrillicWithNumbersString(random.RandRange(1, 100))),
		Config: model.StereopairConfig{
			Devices: model.StereopairDeviceConfigs{
				{
					ID:      leader.ID,
					Channel: model.LeftChannel,
					Role:    model.LeaderRole,
				},
				{
					ID:      follower.ID,
					Channel: model.RightChannel,
					Role:    model.FollowerRole,
				},
			},
		},
		Devices: model.Devices{leader, follower},
	}
	return sp
}
