package mobile

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func TestMobileUserDeviceConfiguration(t *testing.T) {
	device1 := model.Device{
		ID:          "test-device-1",
		Name:        "Моя лампа",
		Room:        &model.Room{ID: "test-room-id-1", Name: "Комната"},
		HouseholdID: "test-household-id",
	}
	var viewV1 DeviceConfigureV1View
	viewV1.FromDevice(context.Background(), device1, model.Devices{}, model.Household{ID: "test-household-id-1", Name: "Дом"}, nil, model.Stereopairs{}, nil)
	assert.Equal(t, viewV1.RoomName, "Комната")
	assert.Equal(t, viewV1.HouseholdName, "Дом")

	var viewV2 DeviceConfigureV2View
	viewV2.FromDevice(context.Background(), device1, model.Devices{}, model.Household{ID: "test-household-id-1", Name: "Дом"}, nil, model.Stereopairs{}, nil)
	assert.Equal(t, viewV2.Room, &DeviceConfigureViewRoom{
		ID:   "test-room-id-1",
		Name: "Комната",
	})
	assert.Equal(t, viewV2.Household, DeviceConfigureViewHousehold{
		ID:   "test-household-id-1",
		Name: "Дом",
	})
}
