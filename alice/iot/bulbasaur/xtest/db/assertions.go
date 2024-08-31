package xtestdb

import (
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
)

func (f *DB) AssertDevicesNotPresent(user *model.TestUser, deviceIDs ...string) {
	devices, err := f.client.SelectUserDevices(f.ctx, user.ID)
	if err != nil {
		f.t.Fatalf("select user devices failed: %v", err)
		return
	}
	intersection := tools.Intersect(devices.GetIDs(), deviceIDs)
	assert.Empty(f.t, intersection, f.logger.JoinedLogs())
}

func (f *DB) AssertDevicesPresent(user *model.TestUser, deviceIDs ...string) {
	devices, err := f.client.SelectUserDevices(f.ctx, user.ID)
	if err != nil {
		f.t.Fatalf("select user devices failed: %v", err)
		return
	}
	intersection := tools.Intersect(devices.GetIDs(), deviceIDs)
	assert.ElementsMatch(f.t, deviceIDs, intersection, f.logger.JoinedLogs())
}

func (f *DB) AssertStereopairsNotPresent(user *model.TestUser, stereopairIDs ...string) {
	stereopairs, err := f.client.SelectStereopairs(f.ctx, user.ID)
	if err != nil {
		f.t.Fatalf("select user stereopairs failed: %v", err)
		return
	}
	assert.Empty(f.t, tools.Intersect(stereopairs.IDs(), stereopairIDs), f.logger.JoinedLogs())
}

func (f *DB) AssertUserDevices(userID uint64, expectedDevices model.Devices) {
	actualDevices, err := f.client.SelectUserDevices(f.ctx, userID)
	require.NoError(f.t, err, f.logger.JoinedLogs())

	expectedDevicesMap := make(map[string]model.Device, len(expectedDevices))
	for i := range expectedDevices {
		device := expectedDevices[i].Clone()
		device.ID = ""
		expectedDevicesMap[device.ExternalKey()] = device
	}
	actualDevicesMap := make(map[string]model.Device, len(actualDevices))
	for i := range actualDevices {
		device := actualDevices[i].Clone()
		device.ID = ""
		actualDevicesMap[device.ExternalKey()] = device
	}

	for key, expectedDevice := range expectedDevicesMap {
		actualDevice, found := actualDevicesMap[key]
		assert.Truef(f.t, found, "expected device %s not found in actual devices %v. %s", key, actualDevices.ExternalKeys(), f.logger.JoinedLogs())
		assert.Equalf(f.t, expectedDevice, actualDevice, "devices mismatch. %s", f.logger.JoinedLogs())
	}
	for key := range actualDevicesMap {
		_, found := expectedDevicesMap[key]
		assert.Truef(f.t, found, "actual device %s not found in expected devices %v. Logs:\n%s", key, expectedDevices.ExternalKeys(), f.logger.JoinedLogs())
	}
}
