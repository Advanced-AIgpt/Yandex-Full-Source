package repository

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type IController interface {
	SelectUser(context.Context, uint64) (model.User, error)
	SelectUserDevice(ctx context.Context, user model.User, deviceID string) (model.Device, error)
	SelectUserDeviceSimple(context.Context, model.User, string) (model.Device, error)
	SelectUserDevices(context.Context, model.User) (model.Devices, error)
	SelectUserDevicesSimple(context.Context, model.User) (model.Devices, error)
	SelectUserGroupDevices(context.Context, model.User, string) ([]model.Device, error)
	SelectUserRoomDevices(context.Context, model.User, string) ([]model.Device, error)
	SelectUserHouseholdDevices(context.Context, model.User, string) (model.Devices, error)
	SelectUserHouseholdDevicesSimple(context.Context, model.User, string) (model.Devices, error)
	SelectUserHousehold(context.Context, model.User, string) (model.Household, error)
	SelectHouseholdResidents(ctx context.Context, userID uint64, household model.Household) (model.HouseholdResidents, error)

	DevicesAndScenarios(context.Context, model.User) (model.Devices, model.Scenarios, error)
	UserInfo(context.Context, model.User) (model.UserInfo, error)
	UserInfoWithPumpkin(context.Context, model.User) (model.UserInfo, error)

	SelectScenarioDeviceTriggers(context.Context, model.User) (model.Devices, error)
}
