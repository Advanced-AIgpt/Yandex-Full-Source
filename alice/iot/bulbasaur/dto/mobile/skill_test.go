package mobile

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"github.com/stretchr/testify/assert"
)

func TestProviderSkillDeviceView(t *testing.T) {
	modelDevice := model.Device{
		ID:   "1",
		Name: "Колонка",
		Type: model.YandexStationDeviceType,
		Room: &model.Room{ID: "room-id", Name: "Колоночная"},
	}

	expected := ProviderSkillDeviceView{
		ID:       "1",
		Name:     "Колонка",
		Type:     model.YandexStationDeviceType,
		RoomName: "Колоночная",
	}
	var actual ProviderSkillDeviceView
	actual.FromDevice(modelDevice)
	assert.Equal(t, expected, actual)
}
