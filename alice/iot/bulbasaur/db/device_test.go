package db

import (
	"context"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *DBClientSuite) TestSelectUserDevice_s_Simple() {
	// TODO mock selectUserDevicesSimple to test SelectUserDevicesSimple corner cases?
	// TODO mock selectUserDevicesSimple to test SelectUserDeviceSimple corner cases?
	// TODO mock selectUserDevices to test SelectUserDevices corner cases?
	// TODO mock selectUserDevices to test SelectUserDevice corner cases?

	s.Run("Basic", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		devices := make([]model.Device, 3)
		for i := range devices {
			devices[i] = data.GenerateDevice()
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			devices[i] = storedDevice
			s.Equal(model.StoreResultNew, storeResult)
			devices[i] = formatDeviceStoreUserDevice(s.context, devices[i])
		}

		device, err := s.dbClient.SelectUserDevice(s.context, user.ID, "unknown-device-id")
		s.True(xerrors.Is(err, &model.DeviceNotFoundError{}))
		s.Zero(device)

		device, err = s.dbClient.SelectUserDeviceSimple(s.context, user.ID, "unknown-device-id")
		s.True(xerrors.Is(err, &model.DeviceNotFoundError{}))
		s.Zero(device)

		s.checkUserDevices(user.ID, devices)
	})
	s.Run("UnknownUser", func() {
		user := data.GenerateUser()

		device, err := s.dbClient.SelectUserDevice(s.context, user.ID, "device-id")
		s.True(xerrors.Is(err, &model.DeviceNotFoundError{}))
		s.Zero(device)

		device, err = s.dbClient.SelectUserDeviceSimple(s.context, user.ID, "device-id")
		s.True(xerrors.Is(err, &model.DeviceNotFoundError{}))
		s.Zero(device)

		s.checkUserDevices(user.ID, nil)
	})
	s.Run("EmptyUser", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device, err := s.dbClient.SelectUserDevice(s.context, user.ID, "device-id")
		s.True(xerrors.Is(err, &model.DeviceNotFoundError{}))
		s.Zero(device)

		device, err = s.dbClient.SelectUserDeviceSimple(s.context, user.ID, "device-id")
		s.True(xerrors.Is(err, &model.DeviceNotFoundError{}))
		s.Zero(device)

		s.checkUserDevices(user.ID, nil)
	})
	s.Run("DeletedDevices", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		device1 := data.GenerateDevice()
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device1)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		device1 = storedDevice
		s.Equal(storeResult, model.StoreResultNew)
		device1 = formatDeviceStoreUserDevice(s.context, device1)

		device2 := data.GenerateDevice()
		storedDevice, storeResult, err = s.dbClient.StoreUserDevice(s.context, user, device2)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		s.Equal(storeResult, model.StoreResultNew)
		device2 = storedDevice
		device2 = formatDeviceStoreUserDevice(s.context, device2)

		s.checkUserDevices(user.ID, []model.Device{device1, device2})

		err = s.dbClient.DeleteUserDevice(s.context, user.ID, device1.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserDevices(user.ID, []model.Device{device2})
	})
	s.Run("EvilUser", func() {
		alice := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, alice)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		aliceDevice := data.GenerateDevice()
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, alice, aliceDevice)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		s.Equal(storeResult, model.StoreResultNew)
		aliceDevice = storedDevice
		aliceDevice = formatDeviceStoreUserDevice(s.context, aliceDevice)

		bob := data.GenerateUser()
		err = s.dbClient.StoreUser(s.context, bob)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device, err := s.dbClient.SelectUserDevice(s.context, bob.ID, aliceDevice.ID)
		s.True(xerrors.Is(err, &model.DeviceNotFoundError{}))
		s.Zero(device)

		device, err = s.dbClient.SelectUserDeviceSimple(s.context, bob.ID, aliceDevice.ID)
		s.True(xerrors.Is(err, &model.DeviceNotFoundError{}))
		s.Zero(device)

		s.checkUserDevices(bob.ID, nil)

		s.checkUserDevices(alice.ID, []model.Device{aliceDevice})
	})
}

func (s *DBClientSuite) TestSelectUserProviderDevices() {
	// TODO mock selectUserDevices to test SelectUserProviderDevices corner cases?

	s.Run("Basic", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		devicesA := make([]model.Device, 3)
		for i := range devicesA {
			devicesA[i] = data.GenerateDevice()
			devicesA[i].SkillID = "A"
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, devicesA[i])
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			devicesA[i] = formatDeviceStoreUserDevice(s.context, storedDevice)
		}

		devicesB := make([]model.Device, 3)
		for i := range devicesB {
			devicesB[i] = data.GenerateDevice()
			devicesB[i].SkillID = "B"
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, devicesB[i])
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			devicesB[i] = formatDeviceStoreUserDevice(s.context, storedDevice)
		}

		s.checkUserProviderDevices(user.ID, "unknown-skill-id", nil)
		s.checkUserProviderDevices(user.ID, "A", devicesA)
		s.checkUserProviderDevices(user.ID, "B", devicesB)
	})
	s.Run("UnknownUser", func() {
		user := data.GenerateUser()
		s.checkUserProviderDevices(user.ID, "skill-id", nil)
	})
	s.Run("EmptyUser", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserDevices(user.ID, nil)
		s.checkUserProviderDevices(user.ID, "skill-id", nil)
	})
	s.Run("DeletedDevices", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		devices := make([]model.Device, 3)
		for i := range devices {
			devices[i] = data.GenerateDevice()
			devices[i].SkillID = "A"
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			devices[i] = formatDeviceStoreUserDevice(s.context, storedDevice)
		}

		s.checkUserProviderDevices(user.ID, "A", devices)

		err = s.dbClient.DeleteUserDevices(s.context, user.ID, []string{devices[0].ID, devices[2].ID})
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserProviderDevices(user.ID, "A", []model.Device{devices[1]})
	})
	s.Run("EvilUser", func() {
		alice := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, alice)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		aliceDevices := make([]model.Device, 3)
		for i := range aliceDevices {
			aliceDevices[i] = data.GenerateDevice()
			aliceDevices[i].SkillID = "A"
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, alice, aliceDevices[i])
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			aliceDevices[i] = formatDeviceStoreUserDevice(s.context, storedDevice)
		}

		bob := data.GenerateUser()
		err = s.dbClient.StoreUser(s.context, bob)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserProviderDevices(bob.ID, "A", nil)
	})
	s.Run("UnknownProvider", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserProviderDevices(user.ID, "unknown-skill", nil)

		// corner case: add some devices
		devices := make([]model.Device, 3)
		for i := range devices {
			devices[i] = data.GenerateDevice()
			devices[i].SkillID = "A"
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			devices[i] = formatDeviceStoreUserDevice(s.context, storedDevice)
		}

		s.checkUserProviderDevices(user.ID, "unknown-skill", nil)
	})
}

func (s *DBClientSuite) TestSelectUserGroupDevices_Simple() {
	// TODO mock selectUserDevices to test SelectUserGroupDevices corner cases?
	// TODO mock selectUserDevicesSimple to test SelectUserDevicesSimple corner cases?

	s.Run("Basic", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device1 := data.GenerateDevice()
		device1.Type = model.OtherDeviceType
		device1.OriginalType = model.OtherDeviceType
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device1)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device1 = formatDeviceStoreUserDevice(s.context, storedDevice)

		device2 := data.GenerateDevice()
		device2.Type = model.OtherDeviceType
		device2.OriginalType = model.OtherDeviceType
		storedDevice, storeResult, err = s.dbClient.StoreUserDevice(s.context, user, device2)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device2 = formatDeviceStoreUserDevice(s.context, storedDevice)

		group1 := data.GenerateGroup()

		group1.ID, err = s.dbClient.CreateUserGroup(s.context, user, group1)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		group1.Name = tools.StandardizeSpaces(group1.Name)
		group1.Type = model.OtherDeviceType

		group2 := data.GenerateGroup()
		group2.ID, err = s.dbClient.CreateUserGroup(s.context, user, group2)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		group2.Name = tools.StandardizeSpaces(group2.Name)
		group2.Type = model.OtherDeviceType

		device1.Groups = []model.Group{group1}
		err = s.dbClient.UpdateUserDeviceGroups(s.context, user, device1.ID, []string{group1.ID})
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device2.Groups = []model.Group{group1, group2}
		err = s.dbClient.UpdateUserDeviceGroups(s.context, user, device2.ID, []string{group1.ID, group2.ID})
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserDevices(user.ID, []model.Device{device1, device2})
		s.checkUserGroupDevices(user.ID, group1.ID, []model.Device{device1, device2})
		s.checkUserGroupDevices(user.ID, group2.ID, []model.Device{device2})
	})
	s.Run("UnknownUser", func() {
		user := data.GenerateUser()

		s.checkUserGroupDevices(user.ID, "group-id", nil)
	})
	s.Run("EmptyUser", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		currentHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, user.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		group := data.GenerateGroup()
		group.ID, err = s.dbClient.CreateUserGroup(s.context, user, group)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		group.Name = tools.StandardizeSpaces(group.Name)
		group.HouseholdID = currentHousehold.ID

		s.checkUserDevices(user.ID, nil)
		s.checkUserGroupDevices(user.ID, group.ID, nil)
	})
	s.Run("DeletedDevices", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		group := data.GenerateGroup()
		group.ID, err = s.dbClient.CreateUserGroup(s.context, user, group)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		group.Name = tools.StandardizeSpaces(group.Name)
		group.Type = model.OtherDeviceType

		devices := make([]model.Device, 3)
		for i := range devices {
			devices[i] = data.GenerateDevice()
			devices[i].Type = model.OtherDeviceType
			devices[i].OriginalType = model.OtherDeviceType
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			devices[i] = formatDeviceStoreUserDevice(s.context, storedDevice)

			devices[i].Groups = []model.Group{group}
			err = s.dbClient.UpdateUserDeviceGroups(s.context, user, devices[i].ID, []string{group.ID})
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
		}

		s.checkUserGroupDevices(user.ID, group.ID, devices)

		err = s.dbClient.DeleteUserDevices(s.context, user.ID, []string{devices[0].ID, devices[2].ID})
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserGroupDevices(user.ID, group.ID, []model.Device{devices[1]})
	})
	s.Run("EvilUser", func() {
		alice := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, alice)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		aliceGroup := data.GenerateGroup()
		aliceGroup.ID, err = s.dbClient.CreateUserGroup(s.context, alice, aliceGroup)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		aliceGroup.Name = tools.StandardizeSpaces(aliceGroup.Name)
		aliceGroup.Type = model.OtherDeviceType

		aliceDevices := make([]model.Device, 3)
		for i := range aliceDevices {
			aliceDevices[i] = data.GenerateDevice()
			aliceDevices[i].Type = model.OtherDeviceType
			aliceDevices[i].OriginalType = model.OtherDeviceType
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, alice, aliceDevices[i])
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			aliceDevices[i] = formatDeviceStoreUserDevice(s.context, storedDevice)

			aliceDevices[i].Groups = []model.Group{aliceGroup}
			err = s.dbClient.UpdateUserDeviceGroups(s.context, alice, aliceDevices[i].ID, []string{aliceGroup.ID})
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
		}

		bob := data.GenerateUser()
		err = s.dbClient.StoreUser(s.context, bob)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserGroupDevices(alice.ID, aliceGroup.ID, aliceDevices)
		s.checkUserGroupDevices(bob.ID, aliceGroup.ID, nil)

		// corner case
		s.checkUserGroupDevices(alice.ID, aliceGroup.ID, aliceDevices)
	})
	s.Run("UnknownGroup", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		devices := make([]model.Device, 3)
		for i := range devices {
			devices[i] = data.GenerateDevice()
			devices[i].Type = model.OtherDeviceType
			devices[i].OriginalType = model.OtherDeviceType
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			devices[i] = formatDeviceStoreUserDevice(s.context, storedDevice)
		}

		s.checkUserDevices(user.ID, devices)
		s.checkUserGroupDevices(user.ID, "unknown-group", nil)

		// corner case: add some groups
		group := data.GenerateGroup()
		group.ID, err = s.dbClient.CreateUserGroup(s.context, user, group)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		group.Name = tools.StandardizeSpaces(group.Name)
		group.Type = model.OtherDeviceType

		s.checkUserDevices(user.ID, devices)
		s.checkUserGroupDevices(user.ID, "unknown-group", nil)

		// corner case: link device and group

		devices[0].Groups = []model.Group{group}
		err = s.dbClient.UpdateUserDeviceGroups(s.context, user, devices[0].ID, []string{group.ID})
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserDevices(user.ID, devices)
		s.checkUserGroupDevices(user.ID, group.ID, []model.Device{devices[0]})
		s.checkUserGroupDevices(user.ID, "unknown-group", nil)
	})
	s.Run("DeletedGroup", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device1 := data.GenerateDevice()
		device1.Type = model.OtherDeviceType
		device1.OriginalType = model.OtherDeviceType
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device1)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device1 = formatDeviceStoreUserDevice(s.context, storedDevice)
		/* impossible condition: nil != nil
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}*/

		device2 := data.GenerateDevice()
		device2.Type = model.OtherDeviceType
		device2.OriginalType = model.OtherDeviceType
		storedDevice, storeResult, err = s.dbClient.StoreUserDevice(s.context, user, device2)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device2 = formatDeviceStoreUserDevice(s.context, storedDevice)

		group1 := data.GenerateGroup()
		group1.ID, err = s.dbClient.CreateUserGroup(s.context, user, group1)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		group1.Name = tools.StandardizeSpaces(group1.Name)
		group1.Type = model.OtherDeviceType

		group2 := data.GenerateGroup()
		group2.ID, err = s.dbClient.CreateUserGroup(s.context, user, group2)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		group2.Name = tools.StandardizeSpaces(group2.Name)
		group2.Type = model.OtherDeviceType

		device1.Groups = []model.Group{group1}
		err = s.dbClient.UpdateUserDeviceGroups(s.context, user, device1.ID, []string{group1.ID})
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device2.Groups = []model.Group{group1, group2}
		err = s.dbClient.UpdateUserDeviceGroups(s.context, user, device2.ID, []string{group1.ID, group2.ID})
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserDevices(user.ID, []model.Device{device1, device2})
		s.checkUserGroupDevices(user.ID, group1.ID, []model.Device{device1, device2})
		s.checkUserGroupDevices(user.ID, group2.ID, []model.Device{device2})

		err = s.dbClient.DeleteUserGroup(s.context, user.ID, group1.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		device1.Groups = nil
		device2.Groups = []model.Group{group2}

		s.checkUserDevices(user.ID, []model.Device{device1, device2})
		s.checkUserGroupDevices(user.ID, group1.ID, nil)
		s.checkUserGroupDevices(user.ID, group2.ID, []model.Device{device2})
	})
}

func (s *DBClientSuite) TestStoreUserDevice() {
	s.Run("NewDevice", func() {
		s.Run("Basic", func() {
			user := data.GenerateUser()
			err := s.dbClient.StoreUser(s.context, user)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserDevices(user.ID, nil)

			device := data.GenerateDevice()
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			device = formatDeviceStoreUserDevice(s.context, storedDevice)

			s.checkUserDevices(user.ID, []model.Device{device})
		})
		s.Run("IgnoreID", func() {
			user := data.GenerateUser()
			err := s.dbClient.StoreUser(s.context, user)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			s.checkUserDevices(user.ID, nil)

			device := data.GenerateDevice()
			device.ID = "some-device-id"
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			device = formatDeviceStoreUserDevice(s.context, storedDevice)

			s.checkUserDevices(user.ID, []model.Device{device})
		})
		s.Run("IgnoreGroups", func() {
			user := data.GenerateUser()
			err := s.dbClient.StoreUser(s.context, user)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserDevices(user.ID, nil)
			s.checkUserGroups(user.ID, []model.Group{}, nil)

			device := data.GenerateDevice()
			device.Groups = []model.Group{data.GenerateGroup()}
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			device = formatDeviceStoreUserDevice(s.context, storedDevice)
			device.Groups = nil

			s.checkUserDevices(user.ID, []model.Device{device})
			s.checkUserGroups(user.ID, []model.Group{}, nil)
		})
		s.Run("NameNormalization", func() {
			for i, testCase := range []struct {
				NewDeviceName string
				ExpectedName  string
			}{
				{
					NewDeviceName: "some device 123",
					ExpectedName:  "some device 123",
				},
				{
					NewDeviceName: "some    device  123",
					ExpectedName:  "some device 123",
				},
				{
					NewDeviceName: "  some device 123",
					ExpectedName:  "some device 123",
				},
				{
					NewDeviceName: "some device 123    ",
					ExpectedName:  "some device 123",
				},
				{
					NewDeviceName: "  some device 123         ",
					ExpectedName:  "some device 123",
				},
				{
					NewDeviceName: "  some    device        123    ",
					ExpectedName:  "some device 123",
				},
				{
					NewDeviceName: "  some    device123    ",
					ExpectedName:  "some device123",
				},
				{
					NewDeviceName: "  somedevice123    ",
					ExpectedName:  "somedevice123",
				},
			} {
				s.Run(fmt.Sprintf("%v.%s", i, testCase.NewDeviceName), func() {
					user := data.GenerateUser()
					err := s.dbClient.StoreUser(s.context, user)
					if err != nil {
						s.dataPreparationFailed(err)
						return
					}

					s.checkUserDevices(user.ID, nil)

					device := data.GenerateDevice()
					device.ExternalName = testCase.NewDeviceName
					storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
					s.NoError(err)
					s.Equal(model.StoreResultNew, storeResult)
					device = formatDeviceStoreUserDevice(s.context, storedDevice)
					device.Name = testCase.ExpectedName
					device.ExternalName = testCase.NewDeviceName

					s.checkUserDevices(user.ID, []model.Device{device})
				})
			}
		})
		s.Run("DuplicatedName", func() {
			user := data.GenerateUser()
			err := s.dbClient.StoreUser(s.context, user)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserDevices(user.ID, nil)

			device1 := data.GenerateDevice()
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device1)
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			device1 = formatDeviceStoreUserDevice(s.context, storedDevice)

			s.checkUserDevices(user.ID, []model.Device{device1})

			device2 := data.GenerateDevice()
			device2.Name = device1.Name
			device2, _, err = s.dbClient.StoreUserDevice(s.context, user, device2)
			s.NoError(err)
			device2 = formatDeviceStoreUserDevice(s.context, device2)

			s.checkUserDevices(user.ID, []model.Device{device1, device2})
		})
		s.Run("DeviceInfo", func() {
			user := data.GenerateUser()
			err := s.dbClient.StoreUser(s.context, user)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserDevices(user.ID, nil)

			device := data.GenerateDevice()
			device.DeviceInfo = data.GenerateDeviceInfo()
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			device = formatDeviceStoreUserDevice(s.context, storedDevice)

			s.checkUserDevices(user.ID, []model.Device{device})
		})
		s.Run("CustomData", func() {
			user := data.GenerateUser()
			err := s.dbClient.StoreUser(s.context, user)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			s.checkUserDevices(user.ID, nil)

			device := data.GenerateDevice()
			// data from example https://yandex.ru/dev/dialogs/alice/doc/smart-home/reference/get-devices-docpage/#output-structure__spec-output
			device.CustomData = map[string]interface{}{
				"foo": 1.0,
				"bar": "two",
				"baz": false,
				"qux": []interface{}{1.0, "two", false},
				"quux": map[string]interface{}{
					"quuz": map[string]interface{}{
						"corge": []interface{}{},
					},
				},
			}
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			device = formatDeviceStoreUserDevice(s.context, storedDevice)

			s.checkUserDevices(user.ID, []model.Device{device})
		})
	})
	s.Run("DeviceLimit", func() {
		defer func(limit uint64) {
			model.ConstDeviceLimit = limit
		}(model.ConstDeviceLimit)
		// change this const only here only to check that all is correct
		model.ConstDeviceLimit = 2

		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		generateLamp := func() (model.Device, model.StoreResult, error) {
			lamp := data.GenerateDevice()
			onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
			onOff.SetRetrievable(true)
			onOff.SetState(model.OnOffCapabilityState{Value: true})
			lamp.Capabilities = []model.ICapability{
				onOff,
			}
			return s.dbClient.StoreUserDevice(s.context, user, lamp)
		}

		lamp1, storeResult, err := generateLamp()

		if err != nil {
			s.dataPreparationFailed(err)
		}
		lamp1 = formatDeviceStoreUserDevice(s.context, lamp1)
		s.Equal(model.StoreResultNew, storeResult)

		lamp2, storeResult, err := generateLamp()
		if err != nil {
			s.dataPreparationFailed(err)
		}
		lamp2 = formatDeviceStoreUserDevice(s.context, lamp2)
		s.Equal(model.StoreResultNew, storeResult)

		_, storeResult, err = generateLamp()
		if err != nil {
			s.dataPreparationFailed(err)
		}
		s.Equal(model.StoreResultLimitReached, storeResult)

		s.checkUserDevices(user.ID, []model.Device{lamp1, lamp2})

		lamp2.ExternalName = "amazing epic name"
		_, storeResult, err = s.dbClient.StoreUserDevice(s.context, user, lamp2)
		s.NoError(err)
		s.Equal(model.StoreResultUpdated, storeResult)
		s.checkUserDevices(user.ID, []model.Device{lamp1, lamp2})
	})
	s.Run("NewDeviceWithNewRoom", func() {
		s.Run("Basic", func() {
			user := data.GenerateUser()
			err := s.dbClient.StoreUser(s.context, user)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			s.checkUserDevices(user.ID, nil)
			s.checkUserRooms(user.ID, nil, nil)

			device := data.GenerateDevice()
			device.Room = &model.Room{Name: data.GenerateRoom().Name}
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			device = formatDeviceStoreUserDevice(s.context, storedDevice)

			rooms, err := s.dbClient.SelectUserRooms(s.context, user.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			if len(rooms) != 1 {
				s.NotNil(rooms)
				s.Equal(1, len(rooms))
				return
			}
			device.Room = formatRoomSelectUserDevice(rooms[0])

			s.checkUserDevices(user.ID, []model.Device{device})
		})
		s.Run("IgnoreRoomID", func() {
			const AttackedRoomID = "some-room-id"

			user := data.GenerateUser()
			err := s.dbClient.StoreUser(s.context, user)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserDevices(user.ID, nil)
			s.checkUserRooms(user.ID, nil, nil)

			device := data.GenerateDevice()
			device.Room = &model.Room{Name: data.GenerateRoom().Name, ID: AttackedRoomID}
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			device = formatDeviceStoreUserDevice(s.context, storedDevice)

			rooms, err := s.dbClient.SelectUserRooms(s.context, user.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			if rooms == nil || len(rooms) != 1 {
				s.NotNil(rooms)
				s.Equal(1, len(rooms))
				return
			}
			s.NotEqual(AttackedRoomID, rooms[0].ID)
		})
		s.Run("NameNormalization", func() {
			for i, testCase := range []struct {
				NewDeviceRoomName string
				ExpectedRoomName  string
			}{
				{
					NewDeviceRoomName: "сом рум 123",
					ExpectedRoomName:  "сом рум 123",
				},
				{
					NewDeviceRoomName: "сом    рум  123",
					ExpectedRoomName:  "сом рум 123",
				},
				{
					NewDeviceRoomName: "  сом рум 123",
					ExpectedRoomName:  "сом рум 123",
				},
				{
					NewDeviceRoomName: "сом рум 123    ",
					ExpectedRoomName:  "сом рум 123",
				},
				{
					NewDeviceRoomName: "  сом рум 123         ",
					ExpectedRoomName:  "сом рум 123",
				},
				{
					NewDeviceRoomName: "  сом    рум        123    ",
					ExpectedRoomName:  "сом рум 123",
				},
				{
					NewDeviceRoomName: "  сомрум 123    ",
					ExpectedRoomName:  "сомрум 123",
				},
			} {
				s.Run(fmt.Sprintf("%v.%s", i, testCase.NewDeviceRoomName), func() {
					user := data.GenerateUser()
					err := s.dbClient.StoreUser(s.context, user)
					if err != nil {
						s.dataPreparationFailed(err)
						return
					}

					s.checkUserDevices(user.ID, nil)
					s.checkUserRooms(user.ID, nil, nil)

					device := data.GenerateDevice()
					newRoom := model.Room{Name: testCase.NewDeviceRoomName}
					device.Room = &newRoom
					device.Groups = nil
					storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
					if err != nil {
						s.dataPreparationFailed(err)
						return
					}
					s.Equal(model.StoreResultNew, storeResult)
					device = formatDeviceStoreUserDevice(s.context, storedDevice)

					rooms, err := s.dbClient.SelectUserRooms(s.context, user.ID)
					if err != nil {
						s.dataPreparationFailed(err)
						return
					}
					if rooms == nil || len(rooms) != 1 {
						s.NotNil(rooms)
						s.Equal(1, len(rooms))
						return
					}

					s.Equal(testCase.ExpectedRoomName, rooms[0].Name)
				})
			}
		})
	})
	s.Run("NewDeviceWithExistentRoom", func() {
		s.Run("Basic", func() {
			user := data.GenerateUser()
			err := s.dbClient.StoreUser(s.context, user)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			id, err := s.dbClient.CreateUserRoom(s.context, user, data.GenerateRoom())
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			room, err := s.dbClient.SelectUserRoom(s.context, user.ID, id)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserDevices(user.ID, nil)
			s.checkUserRooms(user.ID, []model.Room{room}, nil)

			device := data.GenerateDevice()
			device.Room = &model.Room{Name: room.Name} // provider does not use room ID for create
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			device = formatDeviceStoreUserDevice(s.context, storedDevice)

			room.Devices = []string{device.ID}
			device.Room = formatRoomSelectUserDevice(room)

			s.checkUserDevices(user.ID, []model.Device{device})
			s.checkUserRooms(user.ID, []model.Room{room}, []model.Room{room})
		})
		s.Run("IgnoreRoomID", func() {
			user := data.GenerateUser()
			err := s.dbClient.StoreUser(s.context, user)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			id, err := s.dbClient.CreateUserRoom(s.context, user, data.GenerateRoom())
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			attackedRoom, err := s.dbClient.SelectUserRoom(s.context, user.ID, id)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			id, err = s.dbClient.CreateUserRoom(s.context, user, data.GenerateRoom())
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			room, err := s.dbClient.SelectUserRoom(s.context, user.ID, id)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserDevices(user.ID, nil)
			s.checkUserRooms(user.ID, []model.Room{attackedRoom, room}, nil)

			device := data.GenerateDevice()
			device.Room = &model.Room{Name: room.Name, ID: attackedRoom.ID}
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			device = formatDeviceStoreUserDevice(s.context, storedDevice)

			room.Devices = []string{device.ID}
			device.Room = formatRoomSelectUserDevice(room)

			s.checkUserDevices(user.ID, []model.Device{device})
			s.checkUserRooms(user.ID, []model.Room{attackedRoom, room}, []model.Room{room})
		})
		s.Run("NameNormalization", func() {
			for i, testCase := range []struct {
				ExistentRoomName  string
				NewDeviceRoomName string
			}{
				{
					ExistentRoomName:  "сом рум 123",
					NewDeviceRoomName: "сом рум 123",
				},
				{
					ExistentRoomName:  "сом рум 123",
					NewDeviceRoomName: "сом    рум  123",
				},
				{
					ExistentRoomName:  "сом рум 123",
					NewDeviceRoomName: "  сом рум 123",
				},
				{
					ExistentRoomName:  "сом рум 123",
					NewDeviceRoomName: "сом рум 123    ",
				},
				{
					ExistentRoomName:  "сом рум 123",
					NewDeviceRoomName: "  сом рум 123         ",
				},
				{
					ExistentRoomName:  "сом рум 123",
					NewDeviceRoomName: "  сом    рум        123    ",
				},
				{
					ExistentRoomName:  "сом рум 123",
					NewDeviceRoomName: "  сом    рум 123    ",
				},
				{
					ExistentRoomName:  "сомрум 123",
					NewDeviceRoomName: "  сомрум 123    ",
				},
				{
					ExistentRoomName:  "сом рум 123",
					NewDeviceRoomName: "сом Рум 123",
				},
				{
					ExistentRoomName:  "сомрум 123",
					NewDeviceRoomName: " СоМРуМ 123",
				},
			} {
				s.Run(fmt.Sprintf("%v.%s", i, testCase.NewDeviceRoomName), func() {
					user := data.GenerateUser()
					err := s.dbClient.StoreUser(s.context, user)
					if err != nil {
						s.dataPreparationFailed(err)
						return
					}
					id, err := s.dbClient.CreateUserRoom(s.context, user, model.Room{Name: testCase.ExistentRoomName})
					if err != nil {
						s.dataPreparationFailed(err)
						return
					}
					room, err := s.dbClient.SelectUserRoom(s.context, user.ID, id)
					if err != nil {
						s.dataPreparationFailed(err)
						return
					}

					s.checkUserDevices(user.ID, nil)
					s.checkUserRooms(user.ID, []model.Room{room}, nil)

					device := data.GenerateDevice()
					device.Room = &model.Room{Name: testCase.NewDeviceRoomName}
					storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
					s.NoError(err)
					s.Equal(model.StoreResultNew, storeResult)
					device = formatDeviceStoreUserDevice(s.context, storedDevice)

					room.Devices = []string{device.ID}
					device.Room = formatRoomSelectUserDevice(room)

					s.checkUserDevices(user.ID, []model.Device{device})
					s.checkUserRooms(user.ID, []model.Room{room}, []model.Room{room})
				})
			}
		})
		s.Run("EvilUser", func() {
			alice := data.GenerateUser()
			err := s.dbClient.StoreUser(s.context, alice)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			bob := data.GenerateUser()
			err = s.dbClient.StoreUser(s.context, bob)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			id, err := s.dbClient.CreateUserRoom(s.context, alice, data.GenerateRoom())
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			aliceRoom, err := s.dbClient.SelectUserRoom(s.context, alice.ID, id)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserDevices(alice.ID, nil)
			s.checkUserRooms(alice.ID, []model.Room{aliceRoom}, nil)

			bobDevice := data.GenerateDevice()
			bobDevice.Room = &model.Room{Name: aliceRoom.Name, ID: aliceRoom.ID}
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, bob, bobDevice)
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			bobDevice = formatDeviceStoreUserDevice(s.context, storedDevice)

			bobRooms, err := s.dbClient.SelectUserRooms(s.context, bob.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			if bobRooms == nil || len(bobRooms) != 1 {
				s.NotNil(bobRooms)
				s.Equal(1, len(bobRooms))
				return
			}
			bobDevice.Room = formatRoomSelectUserDevice(bobRooms[0])

			s.checkUserDevices(alice.ID, nil)
			s.checkUserRooms(alice.ID, []model.Room{aliceRoom}, nil)

			s.checkUserDevices(bob.ID, []model.Device{bobDevice})
			s.checkUserRooms(bob.ID, bobRooms, bobRooms)
		})
	})
	s.Run("ExistentDevice", func() {
		s.Run("Basic", func() {
			user := data.GenerateUser()
			err := s.dbClient.StoreUser(s.context, user)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			currentHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, user.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserDevices(user.ID, nil)

			originalDevice := data.GenerateDevice()
			originalDevice.Room = &model.Room{Name: data.GenerateRoom().Name}
			originalDevice.DeviceInfo = data.GenerateDeviceInfo()
			originalDevice.CustomData = map[string]interface{}{"foo": "bar"}
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, originalDevice)
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			originalDevice = formatDeviceStoreUserDevice(s.context, storedDevice)
			rooms, err := s.dbClient.SelectUserRooms(s.context, user.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			if rooms == nil || len(rooms) != 1 {
				s.NotNil(rooms)
				s.Equal(1, len(rooms))
				return
			}

			originalDevice = formatDeviceStoreUserDevice(s.context, originalDevice)
			originalDevice.Room = formatRoomSelectUserDevice(rooms[0])

			s.checkUserDevices(user.ID, []model.Device{originalDevice})
			s.checkUserRooms(user.ID, rooms, rooms)

			secondRoomID, err := s.dbClient.CreateUserRoom(s.context, user, data.GenerateRoom())
			s.NoError(err)
			_, err = s.dbClient.SelectUserRoom(s.context, user.ID, secondRoomID)
			s.NoError(err)

			updatedDevice := data.GenerateDevice()
			// <!--clone "primary key" properties
			updatedDevice.ExternalID = originalDevice.ExternalID
			updatedDevice.SkillID = originalDevice.SkillID
			// -->
			updatedDevice.DeviceInfo = data.GenerateDeviceInfo()
			updatedDevice.CustomData = map[string]interface{}{"baz": 2.0}
			updatedDevice.Updated = s.dbClient.CurrentTimestamp()
			storedDevice, storeResult, err = s.dbClient.StoreUserDevice(s.context, user, updatedDevice)
			s.NoError(err)
			s.Equal(model.StoreResultUpdated, storeResult)
			updatedDevice = formatDeviceStoreUserDevice(s.context, storedDevice)
			rooms, err = s.dbClient.SelectUserRooms(s.context, user.ID)
			s.NoError(err)

			expectedDevice := model.Device{
				ID:           originalDevice.ID,
				Name:         originalDevice.Name,
				Aliases:      []string{},
				Description:  originalDevice.Description,
				ExternalID:   originalDevice.ExternalID,
				SkillID:      originalDevice.SkillID,
				Type:         originalDevice.Type,
				OriginalType: originalDevice.OriginalType,
				Groups:       originalDevice.Groups,
				HouseholdID:  currentHousehold.ID,
				Room:         originalDevice.Room,
				ExternalName: updatedDevice.ExternalName,
				Capabilities: updatedDevice.Capabilities,
				Properties:   updatedDevice.Properties,
				DeviceInfo:   updatedDevice.DeviceInfo,
				CustomData:   updatedDevice.CustomData,
				Updated:      updatedDevice.Updated,
				Created:      updatedDevice.Updated,
				Status:       updatedDevice.Status,
			}
			s.checkUserDevices(user.ID, []model.Device{expectedDevice})
			s.checkUserRooms(user.ID, rooms, rooms)
		})
		s.Run("IgnoreID", func() {
			user := data.GenerateUser()
			err := s.dbClient.StoreUser(s.context, user)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			s.checkUserDevices(user.ID, nil)

			originalDevice := data.GenerateDevice()
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, originalDevice)
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			originalDevice = formatDeviceStoreUserDevice(s.context, storedDevice)

			s.checkUserDevices(user.ID, []model.Device{originalDevice})
			s.checkUserRooms(user.ID, nil, nil)

			updatedDevice := data.GenerateDevice()
			// <!--device.ID is not a key; so new device and room will be created
			updatedDevice.ID = originalDevice.ID
			// -->
			updatedDevice.Room = &model.Room{Name: data.GenerateRoom().Name}
			storedDevice, storeResult, err = s.dbClient.StoreUserDevice(s.context, user, updatedDevice)
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			updatedDevice = formatDeviceStoreUserDevice(s.context, storedDevice)

			rooms, err := s.dbClient.SelectUserRooms(s.context, user.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			if rooms == nil || len(rooms) != 1 {
				s.NotNil(rooms)
				s.Equal(1, len(rooms))
				return
			}

			updatedDevice = formatDeviceStoreUserDevice(s.context, updatedDevice)
			updatedDevice.Room = formatRoomSelectUserDevice(rooms[0])

			s.checkUserDevices(user.ID, []model.Device{originalDevice, updatedDevice})
			s.checkUserRooms(user.ID, rooms, rooms)
		})
		s.Run("NameNormalization", func() {
			for i, testCase := range []string{
				"some device 123",
				"some    device  123",
				"  some device 123",
				"some device 123    ",
				"  some device 123         ",
				"  some    device        123    ",
				"  some    device123    ",
				"  somedevice123    ",
			} {
				s.Run(fmt.Sprintf("%v.%s", i, testCase), func() {
					user := data.GenerateUser()
					err := s.dbClient.StoreUser(s.context, user)
					if err != nil {
						s.dataPreparationFailed(err)
						return
					}

					s.checkUserDevices(user.ID, nil)

					originalDevice := data.GenerateDevice()
					storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, originalDevice)
					s.NoError(err)
					s.Equal(model.StoreResultNew, storeResult)
					originalDevice = formatDeviceStoreUserDevice(s.context, storedDevice)

					s.checkUserDevices(user.ID, []model.Device{originalDevice})

					updatedDevice := originalDevice
					updatedDevice.ExternalName = testCase
					_, storeResult, err = s.dbClient.StoreUserDevice(s.context, user, updatedDevice)
					s.NoError(err)
					s.Equal(model.StoreResultUpdated, storeResult)
					s.Equal(originalDevice.ID, updatedDevice.ID)

					s.checkUserDevices(user.ID, []model.Device{updatedDevice})
				})
			}
		})
		s.Run("EvilUser", func() {
			alice := data.GenerateUser()
			err := s.dbClient.StoreUser(s.context, alice)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			bob := data.GenerateUser()
			err = s.dbClient.StoreUser(s.context, bob)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			aliceDevice := data.GenerateDevice()
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, alice, aliceDevice)
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			aliceDevice = formatDeviceStoreUserDevice(s.context, storedDevice)

			bobDevice := data.GenerateDevice()
			// <!--clone "primary key" properties from Alice's device
			bobDevice.ExternalID = aliceDevice.ExternalID
			bobDevice.SkillID = aliceDevice.SkillID
			// -->
			storedDevice, storeResult, err = s.dbClient.StoreUserDevice(s.context, bob, bobDevice)
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			bobDevice = formatDeviceStoreUserDevice(s.context, storedDevice)

			s.checkUserDevices(alice.ID, []model.Device{aliceDevice})
			s.checkUserDevices(bob.ID, []model.Device{bobDevice})
		})
	})
	s.Run("DeletedDevice", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserDevices(user.ID, nil)

		originalDevice := data.GenerateDevice()
		originalDevice.DeviceInfo = data.GenerateDeviceInfo()
		originalDevice.CustomData = map[string]interface{}{"foo": "bar"}
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, originalDevice)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		originalDevice = formatDeviceStoreUserDevice(s.context, storedDevice)

		s.checkUserDevices(user.ID, []model.Device{originalDevice})
		s.checkUserRooms(user.ID, nil, nil)

		err = s.dbClient.DeleteUserDevice(s.context, user.ID, originalDevice.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		updatedDevice := data.GenerateDevice()
		// <!--clone "primary key" properties
		updatedDevice.ExternalID = originalDevice.ExternalID
		updatedDevice.SkillID = originalDevice.SkillID
		// -->
		updatedDevice.Room = &model.Room{Name: data.GenerateRoom().Name} // new room will be created even at "update"
		updatedDevice.DeviceInfo = data.GenerateDeviceInfo()
		updatedDevice.CustomData = map[string]interface{}{"baz": 2.0}
		storedDevice, storeResult, err = s.dbClient.StoreUserDevice(s.context, user, updatedDevice)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		updatedDevice = formatDeviceStoreUserDevice(s.context, storedDevice)

		rooms, err := s.dbClient.SelectUserRooms(s.context, user.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		if rooms == nil || len(rooms) != 1 {
			s.NotNil(rooms)
			s.Equal(1, len(rooms))
			return
		}
		updatedDevice.Room = formatRoomSelectUserDevice(rooms[0])

		s.checkUserDevices(user.ID, []model.Device{updatedDevice})
		s.checkUserRooms(user.ID, rooms, rooms)
	})
	s.Run("non-existent household", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		// check device storing in non-existent household
		homelessDevice := data.GenerateDevice()
		homelessDevice.HouseholdID = "[object Object]"
		homelessDevice, _, err = s.dbClient.StoreUserDevice(s.context, user, homelessDevice)
		s.Error(err)
		s.True(xerrors.Is(err, &model.UserHouseholdNotFoundError{}))
	})
	s.Run("move between households for zigbee devices", func() {
		// store household with room
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		firstHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, user.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		// store device in current household and first room
		device := data.GenerateDevice()
		device.SkillID = model.YANDEXIO
		device.Room = &model.Room{Name: "Первая"}
		device, _, err = s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)

		// put it in some group
		deviceGroup := model.Group{
			Name:        "Группа с устройством",
			Type:        device.Type,
			Devices:     []string{device.ID},
			HouseholdID: device.HouseholdID,
		}
		groupID, err := s.dbClient.CreateUserGroup(s.context, user, deviceGroup)
		s.NoError(err)
		deviceGroup.ID = groupID

		// store another household with another room
		secondHousehold := data.GenerateHousehold("Еще один")
		secondHousehold.ID, err = s.dbClient.CreateUserHousehold(s.context, user.ID, secondHousehold)
		s.NoError(err)
		secondRoom := data.GenerateRoom()
		secondRoom.HouseholdID = secondHousehold.ID
		secondRoom.ID, err = s.dbClient.CreateUserRoom(s.context, user, secondRoom)
		s.NoError(err)

		// re-insert device in another household and room by room name
		device.Room = &model.Room{Name: secondRoom.Name}
		device.HouseholdID = secondHousehold.ID
		device, _, err = s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		// reselect to confirm move
		device, err = s.dbClient.SelectUserDevice(s.context, user.ID, device.ID)
		s.NoError(err)
		s.NotNil(device.HouseholdID)
		s.NotNil(device.Room)
		s.Equal(device.HouseholdID, secondHousehold.ID)
		s.Equal(device.RoomID(), secondRoom.ID)
		// reselect deviceGroups to confirm it has no devices now
		deviceGroups, err := s.dbClient.selectDeviceGroups(s.context, DeviceGroupsQueryCriteria{UserID: user.ID, GroupID: groupID})
		s.NoError(err)
		s.Len(deviceGroups, 0)

		// re-insert into same household with no room
		device.Room = nil
		_, _, err = s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		// reselect to confirm move to no room
		device, err = s.dbClient.SelectUserDevice(s.context, user.ID, device.ID)
		s.NoError(err)
		s.NotNil(device.HouseholdID)
		s.Nil(device.Room)
		s.Equal(device.HouseholdID, secondHousehold.ID)

		// re-insert without household - it will move to current household
		device.HouseholdID = ""
		device, _, err = s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		// reselect to confirm move to firstHousehold
		device, err = s.dbClient.SelectUserDevice(s.context, user.ID, device.ID)
		s.NoError(err)
		s.NotNil(device.HouseholdID)
		s.Nil(device.Room)
		s.Equal(device.HouseholdID, firstHousehold.ID)
	})
	s.Run("no move between households and rooms for already existing devices", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		// create household 2, other from current
		// create room in household2
		// put device in that room in household 2
		household2 := data.GenerateHousehold("Домишко 2")
		household2.ID, err = s.dbClient.CreateUserHousehold(s.context, user.ID, household2)
		s.NoError(err)
		room := data.GenerateRoom()
		room.HouseholdID = household2.ID
		room.ID, err = s.dbClient.CreateUserRoom(s.context, user, room)
		s.NoError(err)
		device := data.GenerateDevice()
		device.Room = &model.Room{Name: room.Name}
		device.HouseholdID = household2.ID
		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		device = storedDevice
		// recheck
		s.NotNil(device.HouseholdID)
		s.NotNil(device.Room)
		s.Equal(device.HouseholdID, household2.ID)
		s.Equal(device.RoomID(), room.ID)
		// try to store it one more time - as its already existed its room and household should not be changed
		// set household and room to nil as it comes from deviceInfoView without them
		device.HouseholdID = ""
		device.Room = nil
		_, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		s.Equal(model.StoreResultUpdated, storeResult)
		// reselect it and double check
		device, err = s.dbClient.SelectUserDevice(s.context, user.ID, device.ID)
		s.NoError(err)
		// recheck
		s.NotNil(device.HouseholdID)
		s.NotNil(device.Room)
		s.Equal(device.HouseholdID, household2.ID)
		s.Equal(device.RoomID(), room.ID)
	})
}

func (s *DBClientSuite) TestStoreDeviceState() {
	s.Run("Basic", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device := data.GenerateDevice()
		deviceOnOffCap := model.MakeCapabilityByType(model.OnOffCapabilityType)
		deviceOnOffCap.SetRetrievable(true)
		deviceOnOffCap.SetState(model.OnOffCapabilityState{Value: true})

		deviceModeCap := model.MakeCapabilityByType(model.ModeCapabilityType)
		deviceModeCap.SetRetrievable(true)
		deviceModeCap.SetParameters(model.ModeCapabilityParameters{
			Instance: model.ThermostatModeInstance,
			Modes: []model.Mode{
				{Value: model.CoolMode, Name: tools.AOS("дубак")},
				{Value: model.HeatMode, Name: tools.AOS("пекло")},
			},
		})
		deviceModeCap.SetState(model.ModeCapabilityState{
			Instance: model.ThermostatModeInstance,
			Value:    model.CoolMode,
		})

		deviceColorCap := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
		deviceColorCap.SetRetrievable(false)
		deviceColorCap.SetParameters(model.ColorSettingCapabilityParameters{
			ColorModel: model.CM(model.HsvModelType),
		})

		device.Capabilities = []model.ICapability{
			deviceOnOffCap,
			deviceModeCap,
			deviceColorCap,
		}
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device = formatDeviceStoreUserDevice(s.context, storedDevice)

		updatedDeviceOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		updatedDeviceOnOff.SetState(model.OnOffCapabilityState{Value: false})

		foreignDeviceRange := model.MakeCapabilityByType(model.RangeCapabilityType)
		foreignDeviceRange.SetState(
			model.RangeCapabilityState{
				Instance: model.BrightnessRangeInstance,
				Value:    50,
			})
		updatedDeviceColor := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
		updatedDeviceColor.SetState(
			model.ColorSettingCapabilityState{
				Instance: model.HsvColorCapabilityInstance,
				Value:    model.HSV{H: 50, S: 50, V: 50},
			})
		updatedDeviceMode := model.MakeCapabilityByType(model.ModeCapabilityType)
		updatedDeviceMode.SetState(
			model.ModeCapabilityState{
				Instance: model.ThermostatModeInstance,
				Value:    model.HeatMode,
			})
		updatedDevice := device
		updatedDevice.Capabilities = []model.ICapability{
			updatedDeviceOnOff.WithLastUpdated(s.dbClient.CurrentTimestamp()),
			foreignDeviceRange.WithLastUpdated(s.dbClient.CurrentTimestamp()),
			updatedDeviceColor.WithLastUpdated(s.dbClient.CurrentTimestamp()),
			updatedDeviceMode.WithLastUpdated(s.dbClient.CurrentTimestamp()),
		}
		updatedDevice.Updated = s.dbClient.CurrentTimestamp()
		_, _, err = s.dbClient.StoreDeviceState(s.context, user.ID, updatedDevice)
		s.NoError(err)

		expectedDevice := device
		expectedDeviceFirstCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
		expectedDeviceFirstCapability.SetRetrievable(device.Capabilities[0].Retrievable())
		expectedDeviceFirstCapability.SetParameters(device.Capabilities[0].Parameters())
		expectedDeviceFirstCapability.SetState(updatedDevice.Capabilities[0].State())
		expectedDeviceFirstCapability.SetLastUpdated(s.dbClient.CurrentTimestamp())

		expectedDeviceSecondCapability := model.MakeCapabilityByType(model.ModeCapabilityType)
		expectedDeviceSecondCapability.SetRetrievable(device.Capabilities[1].Retrievable())
		expectedDeviceSecondCapability.SetParameters(device.Capabilities[1].Parameters())
		expectedDeviceSecondCapability.SetState(updatedDevice.Capabilities[3].State())
		expectedDeviceSecondCapability.SetLastUpdated(s.dbClient.CurrentTimestamp())

		expectedDevice.Capabilities = []model.ICapability{
			expectedDeviceFirstCapability,
			expectedDeviceSecondCapability,
			device.Capabilities[2], // unretrievable capability, not updated
		}

		s.checkUserDevices(user.ID, []model.Device{expectedDevice})
	})
	s.Run("UnknownDevice", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		currentHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, user.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device := data.GenerateDevice()
		device.ID = "unknown-device"
		device.HouseholdID = currentHousehold.ID
		_, _, err = s.dbClient.StoreDeviceState(s.context, user.ID, device)
		s.True(xerrors.Is(err, &model.DeviceNotFoundError{}))
	})
	s.Run("DeletedDevice", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device := data.GenerateDevice()
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device = formatDeviceStoreUserDevice(s.context, storedDevice)

		err = s.dbClient.DeleteUserDevice(s.context, user.ID, device.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device.Capabilities = []model.ICapability{}
		_, _, err = s.dbClient.StoreDeviceState(s.context, user.ID, device)

		s.True(xerrors.Is(err, &model.DeviceNotFoundError{}))
	})
}

func (s *DBClientSuite) TestStoreDevicesStates() {
	// TODO mock StoreDevicesState to test StoreDevicesStates?

	s.Run("EmptyArgs", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		_, _, err = s.dbClient.StoreDevicesStates(s.context, user.ID, []model.Device{}, true)
		s.NoError(err)
	})

	s.Run("EmptyArgs-in-transaction", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		err = s.dbClient.Transaction(s.context, "TestStoreDevicesStates-EmptyArgs-transaction", func(ctx context.Context) error {
			_, _, err := s.dbClient.StoreDevicesStates(ctx, user.ID, []model.Device{}, false)
			return err
		})
		s.NoError(err)
	})
}

func (s *DBClientSuite) TestUpdateUserDeviceName() {
	s.Run("Basic", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device := data.GenerateDevice()
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device = formatDeviceStoreUserDevice(s.context, storedDevice)

		s.checkUserDevices(user.ID, []model.Device{device})

		device.Name = tools.StandardizeSpaces(data.GenerateDevice().ExternalName)
		err = s.dbClient.UpdateUserDeviceName(s.context, user.ID, device.ID, device.Name)
		s.NoError(err)

		s.checkUserDevices(user.ID, []model.Device{device})
	})
	s.Run("ToEmptyName", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device := data.GenerateDevice()
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device = formatDeviceStoreUserDevice(s.context, storedDevice)

		s.checkUserDevices(user.ID, []model.Device{device})

		for _, newName := range []string{"", "   "} {
			err = s.dbClient.UpdateUserDeviceName(s.context, user.ID, device.ID, newName)
			s.True(xerrors.Is(err, &model.InvalidValueError{}))
		}

		s.checkUserDevices(user.ID, []model.Device{device})
	})
	s.Run("NoChange", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device := data.GenerateDevice()
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device = formatDeviceStoreUserDevice(s.context, storedDevice)

		s.checkUserDevices(user.ID, []model.Device{device})

		err = s.dbClient.UpdateUserDeviceName(s.context, user.ID, device.ID, device.Name)
		s.NoError(err)

		s.checkUserDevices(user.ID, []model.Device{device})
	})
	s.Run("NameNormalization", func() {
		for i, testCase := range []struct {
			NewDeviceName      string
			ExpectedDeviceName string
		}{
			{
				NewDeviceName:      "some device 123",
				ExpectedDeviceName: "some device 123",
			},
			{
				NewDeviceName:      "some    device  123",
				ExpectedDeviceName: "some device 123",
			},
			{
				NewDeviceName:      "  some device 123",
				ExpectedDeviceName: "some device 123",
			},
			{
				NewDeviceName:      "some device 123    ",
				ExpectedDeviceName: "some device 123",
			},
			{
				NewDeviceName:      "  some device 123         ",
				ExpectedDeviceName: "some device 123",
			},
			{
				NewDeviceName:      "  some    device        123    ",
				ExpectedDeviceName: "some device 123",
			},
			{
				NewDeviceName:      "  some    device123    ",
				ExpectedDeviceName: "some device123",
			},
			{
				NewDeviceName:      "  somedevice123    ",
				ExpectedDeviceName: "somedevice123",
			},
		} {
			s.Run(fmt.Sprintf("%v.%s", i, testCase.NewDeviceName), func() {
				user := data.GenerateUser()
				err := s.dbClient.StoreUser(s.context, user)
				if err != nil {
					s.dataPreparationFailed(err)
					return
				}

				device := data.GenerateDevice()
				storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
				s.NoError(err)
				s.Equal(model.StoreResultNew, storeResult)
				device = formatDeviceStoreUserDevice(s.context, storedDevice)

				s.checkUserDevices(user.ID, []model.Device{device})

				err = s.dbClient.UpdateUserDeviceName(s.context, user.ID, device.ID, testCase.NewDeviceName)
				s.NoError(err)
				device.Name = testCase.ExpectedDeviceName

				s.checkUserDevices(user.ID, []model.Device{device})
			})
		}
	})
	s.Run("UnknownDevice", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserDevices(user.ID, nil)

		err = s.dbClient.UpdateUserDeviceName(s.context, user.ID, "unknown-device", "some name")
		s.True(xerrors.Is(err, &model.DeviceNotFoundError{}))

		s.checkUserDevices(user.ID, nil)
	})
	s.Run("DeletedDevice", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device := data.GenerateDevice()
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device = formatDeviceStoreUserDevice(s.context, storedDevice)

		s.checkUserDevices(user.ID, []model.Device{device})

		err = s.dbClient.DeleteUserDevice(s.context, user.ID, device.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		err = s.dbClient.UpdateUserDeviceName(s.context, user.ID, device.ID, data.GenerateDevice().ExternalName)
		s.True(xerrors.Is(err, &model.DeviceNotFoundError{}))

		s.checkUserDevices(user.ID, nil)
	})
	s.Run("EvilUser", func() {
		alice := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, alice)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		aliceDevice := data.GenerateDevice()
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, alice, aliceDevice)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		aliceDevice = formatDeviceStoreUserDevice(s.context, storedDevice)

		bob := data.GenerateUser()
		err = s.dbClient.StoreUser(s.context, bob)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserDevices(alice.ID, []model.Device{aliceDevice})
		s.checkUserDevices(bob.ID, nil)

		err = s.dbClient.UpdateUserDeviceName(s.context, bob.ID, aliceDevice.ID, data.GenerateDevice().ExternalName)
		s.True(xerrors.Is(err, &model.DeviceNotFoundError{}))

		s.checkUserDevices(alice.ID, []model.Device{aliceDevice})
		s.checkUserDevices(bob.ID, nil)
	})
}

func (s *DBClientSuite) TestDeleteUserDevice() {
	// TODO mock DeleteUserDevices to test DeleteUserDevice?
}

func (s *DBClientSuite) TestDeleteUserDevices() {
	s.Run("Basic", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		devices := make([]model.Device, 3)
		for i := range devices {
			devices[i] = data.GenerateDevice()
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			devices[i] = formatDeviceStoreUserDevice(s.context, storedDevice)
		}

		s.checkUserDevices(user.ID, devices)

		err = s.dbClient.DeleteUserDevices(s.context, user.ID, []string{devices[0].ID, devices[2].ID})
		s.NoError(err)

		s.checkUserDevices(user.ID, []model.Device{devices[1]})
	})
	s.Run("EmptyArgs", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		devices := make([]model.Device, 3)
		for i := range devices {
			devices[i] = data.GenerateDevice()
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			devices[i] = formatDeviceStoreUserDevice(s.context, storedDevice)
		}

		s.checkUserDevices(user.ID, devices)

		err = s.dbClient.DeleteUserDevices(s.context, user.ID, []string{})
		s.NoError(err)

		s.checkUserDevices(user.ID, devices)
	})
	s.Run("UnknownUser", func() {
		err := s.dbClient.DeleteUserDevices(s.context, data.GenerateUser().ID, []string{"some-device-id"})
		s.NoError(err)
	})
	s.Run("NoDevices", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserDevices(user.ID, nil)

		err = s.dbClient.DeleteUserDevices(s.context, user.ID, []string{"unknown-device-id", "other-unknown-device-id"})
		s.NoError(err)

		s.checkUserDevices(user.ID, nil)
	})
	s.Run("UnknownDevice", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device := data.GenerateDevice()
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device = formatDeviceStoreUserDevice(s.context, storedDevice)

		err = s.dbClient.DeleteUserDevices(s.context, user.ID, []string{"unknown-device-id", "other-unknown-device-id"})
		s.NoError(err)

		s.checkUserDevices(user.ID, []model.Device{device})
	})
	s.Run("DeletedDevice", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device := data.GenerateDevice()
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device = formatDeviceStoreUserDevice(s.context, storedDevice)

		s.checkUserDevices(user.ID, []model.Device{device})

		err = s.dbClient.DeleteUserDevices(s.context, user.ID, []string{device.ID})
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserDevices(user.ID, nil)

		err = s.dbClient.DeleteUserDevices(s.context, user.ID, []string{device.ID})
		s.NoError(err)

		s.checkUserDevices(user.ID, nil)
	})
	s.Run("EvilUser", func() {
		alice := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, alice)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		aliceDevice := data.GenerateDevice()
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, alice, aliceDevice)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		aliceDevice = formatDeviceStoreUserDevice(s.context, storedDevice)

		bob := data.GenerateUser()
		err = s.dbClient.StoreUser(s.context, bob)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserDevices(alice.ID, []model.Device{aliceDevice})
		s.checkUserDevices(bob.ID, nil)

		err = s.dbClient.DeleteUserDevice(s.context, bob.ID, aliceDevice.ID)
		s.NoError(err)

		s.checkUserDevices(alice.ID, []model.Device{aliceDevice})
		s.checkUserDevices(bob.ID, nil)

		// just in case
		err = s.dbClient.DeleteUserDevice(s.context, bob.ID, aliceDevice.ID)
		s.NoError(err)

		s.checkUserDevices(alice.ID, []model.Device{aliceDevice})
		s.checkUserDevices(bob.ID, nil)
	})
	s.Run("stereopairSingleDevice", func() {
		alice := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, alice)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		speakerDevice := data.GenerateDevice()
		speakerDevice.Type = model.YandexStationDeviceType
		speakerDevice.Capabilities = model.GenerateQuasarCapabilities(s.context, model.YandexStationDeviceType)
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, alice, speakerDevice)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		speakerDevice = formatDeviceStoreUserDevice(s.context, storedDevice)

		stereopair := model.Stereopair{
			ID:   "stereo-1-3",
			Name: "name-1-3",
			Config: model.StereopairConfig{
				Devices: model.StereopairDeviceConfigs{
					{
						ID:      speakerDevice.ID,
						Channel: model.RightChannel,
						Role:    model.FollowerRole,
					},
					{
						ID:      "some-id",
						Channel: model.LeftChannel,
						Role:    model.LeaderRole,
					},
				},
			},
		}
		err = s.dbClient.StoreStereopair(s.context, alice.ID, stereopair)
		s.NoError(err)

		err = s.dbClient.DeleteUserDevices(s.context, alice.ID, []string{speakerDevice.ID})
		s.NoError(err)
	})
}

func (s *DBClientSuite) TestSwitchDeviceType() {
	s.Run("Switch device type: socket->light [success]", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device := data.GenerateDevice()

		device.OriginalType = model.SocketDeviceType
		device.Type = model.SocketDeviceType

		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device = formatDeviceStoreUserDevice(s.context, storedDevice)

		s.checkUserDevices(user.ID, []model.Device{device})

		err = s.dbClient.UpdateUserDeviceType(s.context, user.ID, device.ID, model.LightDeviceType)
		s.NoError(err)

		device.Type = model.LightDeviceType
		s.checkUserDevices(user.ID, []model.Device{device})
	})

	s.Run("Switch device type: socket->socket [success]", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device := data.GenerateDevice()

		device.OriginalType = model.SocketDeviceType
		device.Type = model.SocketDeviceType

		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device = formatDeviceStoreUserDevice(s.context, storedDevice)

		s.checkUserDevices(user.ID, []model.Device{device})

		err = s.dbClient.UpdateUserDeviceType(s.context, user.ID, device.ID, model.SocketDeviceType)
		s.NoError(err)
		s.checkUserDevices(user.ID, []model.Device{device})
	})

	s.Run("Switch device type: socket->light, device in group [fail]", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device := data.GenerateDevice()

		device.OriginalType = model.SocketDeviceType
		device.Type = model.SocketDeviceType

		group := model.Group{Name: "kokoko"}
		groupID, _ := s.dbClient.CreateUserGroup(s.context, user, group)

		device.Groups = []model.Group{
			{
				ID:      groupID,
				Name:    group.Name,
				Aliases: []string{},
				Type:    model.SocketDeviceType,
			},
		}

		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device = storedDevice

		err = s.dbClient.UpdateUserDeviceGroups(s.context, user, device.ID, []string{groupID})
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device = formatDeviceStoreUserDevice(s.context, device)

		s.checkUserDevices(user.ID, []model.Device{device})

		err = s.dbClient.UpdateUserDeviceType(s.context, user.ID, device.ID, model.LightDeviceType)
		s.True(xerrors.Is(err, &model.GroupListIsNotEmptyError{}))

		s.checkUserDevices(user.ID, []model.Device{device})
	})

	s.Run("Switch device type: socket->light, unacceptable target type [fail]", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device := data.GenerateDevice()

		device.OriginalType = model.SocketDeviceType
		device.Type = model.SocketDeviceType

		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device = formatDeviceStoreUserDevice(s.context, storedDevice)

		s.checkUserDevices(user.ID, []model.Device{device})

		err = s.dbClient.UpdateUserDeviceType(s.context, user.ID, device.ID, model.SwitchDeviceType)
		s.True(xerrors.Is(err, &model.DeviceTypeSwitchError{}))

		s.checkUserDevices(user.ID, []model.Device{device})
	})

	s.Run("Switch device type: socket->light, unacceptable source type [fail]", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		device := data.GenerateDevice()

		device.OriginalType = model.KettleDeviceType
		device.Type = model.SocketDeviceType

		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device = formatDeviceStoreUserDevice(s.context, storedDevice)

		s.checkUserDevices(user.ID, []model.Device{device})

		err = s.dbClient.UpdateUserDeviceType(s.context, user.ID, device.ID, model.LightDeviceType)
		s.True(xerrors.Is(err, &model.DeviceTypeSwitchError{}))

		s.checkUserDevices(user.ID, []model.Device{device})
	})

	s.Run("select archived devices for user and skillID", func() {
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		devices := make([]model.Device, 3)
		for i := range devices {
			devices[i] = data.GenerateDevice()
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
			s.NoError(err)
			s.Equal(model.StoreResultNew, storeResult)
			devices[i] = formatDeviceStoreUserDevice(s.context, storedDevice)
		}

		s.checkUserDevices(user.ID, devices)

		err = s.dbClient.DeleteUserDevices(s.context, user.ID, []string{devices[0].ID})
		s.NoError(err)

		s.checkUserDevices(user.ID, []model.Device{devices[1], devices[2]})
		expectedDevicesMap := make(map[string]model.Device)
		expectedDevicesMap[devices[0].ExternalID] = devices[0]
		s.checkUserProviderArchivedDevices(user.ID, devices[0].SkillID, expectedDevicesMap)
	})
}

func (s *DBClientSuite) TestStoreUserDeviceConfigs() {
	s.Run("check configs update", func() {
		user := data.GenerateUser()
		if err := s.dbClient.StoreUser(s.context, user); err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.checkUserDevices(user.ID, nil)

		device := data.GenerateDevice()
		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, device)
		s.NoError(err)
		s.Equal(model.StoreResultNew, storeResult)
		device = formatDeviceStoreUserDevice(s.context, storedDevice)
		s.checkUserDevices(user.ID, []model.Device{device})
		device.InternalConfig = model.DeviceConfig{
			Tandem: &model.TandemDeviceConfig{
				Partner: model.TandemDeviceConfigPartner{ID: "hehe-id"},
			},
		}
		deviceConfigsMap := map[string]model.DeviceConfig{
			device.ID: device.InternalConfig,
		}
		err = s.dbClient.StoreUserDeviceConfigs(s.context, user.ID, deviceConfigsMap)
		s.NoError(err)
		device = formatDeviceStoreUserDevice(s.context, device)
		s.checkUserDevices(user.ID, []model.Device{device})
	})
}

func (s *DBClientSuite) TestStoreDeviceUpdatesState() {
	s.Run("check store device with same capabilities", func() {
		user := data.GenerateUser()
		if err := s.dbClient.StoreUser(s.context, user); err != nil {
			s.dataPreparationFailed(err)
			return
		}

		onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType).
			WithRetrievable(true).
			WithParameters(model.OnOffCapabilityParameters{Split: false}).
			WithState(model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true})

		modeCapability := model.MakeCapabilityByType(model.ModeCapabilityType).
			WithRetrievable(true).
			WithParameters(model.ModeCapabilityParameters{
				Instance: model.CoffeeModeInstance,
				Modes: []model.Mode{
					{Value: model.AmericanoMode},
					{Value: model.CappuccinoMode},
					{Value: model.LatteMode},
				},
			}).
			WithState(model.ModeCapabilityState{Instance: model.CoffeeModeInstance, Value: model.LatteMode})

		rangeCapability := model.MakeCapabilityByType(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.BrightnessRangeInstance,
				Unit:         model.UnitPercent,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min: 0,
					Max: 100,
				},
			}).
			WithState(model.RangeCapabilityState{Instance: model.BrightnessRangeInstance, Value: 80})

		device := model.NewDevice("Лампочка").
			WithDeviceType(model.LightDeviceType).
			WithSkillID(model.VIRTUAL).
			WithExternalID("id").
			WithCapabilities(
				onOffCapability,
				modeCapability,
				rangeCapability,
			)

		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, *device)
		s.NoError(err)
		device.ID = storedDevice.ID
		s.Equal(model.StoreResultNew, storeResult)

		device.Capabilities = model.Capabilities{
			onOffCapability,
			modeCapability.WithState(model.ModeCapabilityState{Instance: model.CoffeeModeInstance, Value: model.AmericanoMode}),
			rangeCapability.WithState((*model.RangeCapabilityState)(nil)),
		}

		storedDevice, storeResult, err = s.dbClient.StoreUserDevice(s.context, user, *device)
		s.NoError(err)
		s.Equal(model.StoreResultUpdated, storeResult)
		s.Equal(device.ID, storedDevice.ID)

		storedDevice, err = s.dbClient.SelectUserDevice(s.context, user.ID, device.ID)
		s.NoError(err)
		s.Equal(3, len(storedDevice.Capabilities))

		// The same
		storedOnOffCapability, ok := storedDevice.GetCapabilityByTypeAndInstance(onOffCapability.Type(), onOffCapability.Instance())
		s.Require().True(ok)
		s.Require().NotNil(storedOnOffCapability.State())
		s.Require().IsType(model.OnOffCapabilityState{}, storedOnOffCapability.State())
		s.Equal(model.OnOnOffCapabilityInstance, storedOnOffCapability.State().(model.OnOffCapabilityState).Instance)
		s.Equal(true, storedOnOffCapability.State().(model.OnOffCapabilityState).Value)

		// Updated to AmericanoMode
		storedModeCapability, ok := storedDevice.GetCapabilityByTypeAndInstance(modeCapability.Type(), modeCapability.Instance())
		s.Require().True(ok)
		s.Require().NotNil(storedModeCapability.State())
		s.Require().IsType(model.ModeCapabilityState{}, storedModeCapability.State())
		s.Equal(model.CoffeeModeInstance, storedModeCapability.State().(model.ModeCapabilityState).Instance)
		s.Equal(model.AmericanoMode, storedModeCapability.State().(model.ModeCapabilityState).Value)

		// Did not override old state with nil state
		storedRangeCapability, ok := storedDevice.GetCapabilityByTypeAndInstance(rangeCapability.Type(), rangeCapability.Instance())
		s.Require().True(ok)
		s.Require().NotNil(storedRangeCapability.State())
		s.Require().IsType(model.RangeCapabilityState{}, storedRangeCapability.State())
		s.Equal(model.BrightnessRangeInstance, storedRangeCapability.State().(model.RangeCapabilityState).Instance)
		s.Equal(80.0, storedRangeCapability.State().(model.RangeCapabilityState).Value)
	})

	s.Run("check store device with added capability", func() {
		user := data.GenerateUser()
		if err := s.dbClient.StoreUser(s.context, user); err != nil {
			s.dataPreparationFailed(err)
			return
		}

		onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType).
			WithRetrievable(true).
			WithParameters(model.OnOffCapabilityParameters{Split: true}).
			WithState(model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true})

		modeCapability := model.MakeCapabilityByType(model.ModeCapabilityType).
			WithRetrievable(true).
			WithParameters(model.ModeCapabilityParameters{
				Instance: model.TeaModeInstance,
				Modes: []model.Mode{
					{Value: model.BlackTeaMode},
					{Value: model.GreenTeaMode},
					{Value: model.PuerhTeaMode},
				},
			}).
			WithState(model.ModeCapabilityState{Instance: model.TeaModeInstance, Value: model.BlackTeaMode})

		rangeCapability := model.MakeCapabilityByType(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.VolumeRangeInstance,
				Unit:         model.UnitPercent,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min: 0,
					Max: 100,
				},
			}).
			WithState(model.RangeCapabilityState{Instance: model.BrightnessRangeInstance, Value: 10})

		colorCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType).
			WithRetrievable(true).
			WithParameters(model.ColorSettingCapabilityParameters{
				TemperatureK: &model.TemperatureKParameters{
					Min: 0,
					Max: 300,
				},
			}).
			WithState(model.ColorSettingCapabilityState{Instance: model.TemperatureKCapabilityInstance, Value: model.TemperatureK(150)})

		device := model.NewDevice("Лампочка").
			WithDeviceType(model.LightDeviceType).
			WithSkillID(model.VIRTUAL).
			WithExternalID("id").
			WithCapabilities(
				onOffCapability,
				modeCapability,
				rangeCapability,
			)

		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, *device)
		s.NoError(err)
		device.ID = storedDevice.ID
		s.Equal(model.StoreResultNew, storeResult)

		device.Capabilities = model.Capabilities{
			onOffCapability.WithState(model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: false}),
			modeCapability.WithState((*model.ModeCapabilityState)(nil)),
			rangeCapability.WithState(model.RangeCapabilityState{Instance: model.VolumeRangeInstance, Value: 20.0}),
			colorCapability,
		}

		storedDevice, storeResult, err = s.dbClient.StoreUserDevice(s.context, user, *device)
		s.NoError(err)
		s.Equal(model.StoreResultUpdated, storeResult)
		s.Equal(device.ID, storedDevice.ID)

		storedDevice, err = s.dbClient.SelectUserDevice(s.context, user.ID, device.ID)
		s.NoError(err)
		s.Equal(4, len(storedDevice.Capabilities))

		// Updated to false
		storedOnOffCapability, ok := storedDevice.GetCapabilityByTypeAndInstance(onOffCapability.Type(), onOffCapability.Instance())
		s.Require().True(ok)
		s.Require().NotNil(storedOnOffCapability.State())
		s.Require().IsType(model.OnOffCapabilityState{}, storedOnOffCapability.State())
		s.Equal(model.OnOnOffCapabilityInstance, storedOnOffCapability.State().(model.OnOffCapabilityState).Instance)
		s.Equal(false, storedOnOffCapability.State().(model.OnOffCapabilityState).Value)

		// Did not override old state with nil state
		storedModeCapability, ok := storedDevice.GetCapabilityByTypeAndInstance(modeCapability.Type(), modeCapability.Instance())
		s.Require().True(ok)
		s.Require().NotNil(storedModeCapability.State())
		s.Require().IsType(model.ModeCapabilityState{}, storedModeCapability.State())
		s.Equal(model.TeaModeInstance, storedModeCapability.State().(model.ModeCapabilityState).Instance)
		s.Equal(model.BlackTeaMode, storedModeCapability.State().(model.ModeCapabilityState).Value)

		// Updated to 20
		storedRangeCapability, ok := storedDevice.GetCapabilityByTypeAndInstance(rangeCapability.Type(), rangeCapability.Instance())
		s.Require().True(ok)
		s.Require().NotNil(storedRangeCapability.State())
		s.Require().IsType(model.RangeCapabilityState{}, storedRangeCapability.State())
		s.Equal(model.VolumeRangeInstance, storedRangeCapability.State().(model.RangeCapabilityState).Instance)
		s.Equal(20.0, storedRangeCapability.State().(model.RangeCapabilityState).Value)

		// Saved
		storedColorCapability, ok := storedDevice.GetCapabilityByTypeAndInstance(colorCapability.Type(), colorCapability.Instance())
		s.Require().True(ok)
		s.Require().NotNil(storedColorCapability.State())
		s.Require().IsType(model.ColorSettingCapabilityState{}, storedColorCapability.State())
		s.Equal(model.TemperatureKCapabilityInstance, storedColorCapability.State().(model.ColorSettingCapabilityState).Instance)
		s.Equal(model.TemperatureK(150), storedColorCapability.State().(model.ColorSettingCapabilityState).Value)
	})

	s.Run("check store device with new capability and removed capability", func() {
		user := data.GenerateUser()
		if err := s.dbClient.StoreUser(s.context, user); err != nil {
			s.dataPreparationFailed(err)
			return
		}

		onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType).
			WithRetrievable(true).
			WithParameters(model.OnOffCapabilityParameters{Split: true}).
			WithState(model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true})

		modeCapability := model.MakeCapabilityByType(model.ModeCapabilityType).
			WithRetrievable(true).
			WithParameters(model.ModeCapabilityParameters{
				Instance: model.TeaModeInstance,
				Modes: []model.Mode{
					{Value: model.BlackTeaMode},
					{Value: model.GreenTeaMode},
					{Value: model.PuerhTeaMode},
				},
			}).
			WithState(model.ModeCapabilityState{Instance: model.TeaModeInstance, Value: model.BlackTeaMode})

		rangeCapability := model.MakeCapabilityByType(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.VolumeRangeInstance,
				Unit:         model.UnitPercent,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min: 0,
					Max: 100,
				},
			}).
			WithState(model.RangeCapabilityState{Instance: model.BrightnessRangeInstance, Value: 10})

		colorCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType).
			WithRetrievable(true).
			WithParameters(model.ColorSettingCapabilityParameters{
				TemperatureK: &model.TemperatureKParameters{
					Min: 0,
					Max: 300,
				},
			}).
			WithState(model.ColorSettingCapabilityState{Instance: model.TemperatureKCapabilityInstance, Value: model.TemperatureK(150)})

		toggleCapability := model.MakeCapabilityByType(model.ToggleCapabilityType).
			WithRetrievable(true).
			WithParameters(model.ToggleCapabilityParameters{Instance: model.MuteToggleCapabilityInstance}).
			WithState(model.ToggleCapabilityState{Instance: model.MuteToggleCapabilityInstance, Value: false})

		device := model.NewDevice("Лампочка").
			WithDeviceType(model.LightDeviceType).
			WithSkillID(model.VIRTUAL).
			WithExternalID("id").
			WithCapabilities(
				onOffCapability,
				modeCapability,
				rangeCapability,
				colorCapability,
			)

		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, *device)
		s.NoError(err)
		device.ID = storedDevice.ID
		s.Equal(model.StoreResultNew, storeResult)

		device.Capabilities = model.Capabilities{
			modeCapability.WithState(model.ModeCapabilityState{Instance: model.TeaModeInstance, Value: model.GreenTeaMode}),
			rangeCapability.WithState(model.RangeCapabilityState{Instance: model.VolumeRangeInstance, Value: 20.0}),
			colorCapability.WithState((*model.ColorSettingCapabilityState)(nil)),
			toggleCapability,
		}

		storedDevice, storeResult, err = s.dbClient.StoreUserDevice(s.context, user, *device)
		s.NoError(err)
		s.Equal(model.StoreResultUpdated, storeResult)
		s.Equal(device.ID, storedDevice.ID)

		storedDevice, err = s.dbClient.SelectUserDevice(s.context, user.ID, device.ID)
		s.NoError(err)
		s.Equal(4, len(storedDevice.Capabilities))

		// Updated to GreenTeaMode
		storedModeCapability, ok := storedDevice.GetCapabilityByTypeAndInstance(modeCapability.Type(), modeCapability.Instance())
		s.Require().True(ok)
		s.Require().NotNil(storedModeCapability.State())
		s.Require().IsType(model.ModeCapabilityState{}, storedModeCapability.State())
		s.Equal(model.TeaModeInstance, storedModeCapability.State().(model.ModeCapabilityState).Instance)
		s.Equal(model.GreenTeaMode, storedModeCapability.State().(model.ModeCapabilityState).Value)

		// Updated to 20
		storedRangeCapability, ok := storedDevice.GetCapabilityByTypeAndInstance(rangeCapability.Type(), rangeCapability.Instance())
		s.Require().True(ok)
		s.Require().NotNil(storedRangeCapability.State())
		s.Require().IsType(model.RangeCapabilityState{}, storedRangeCapability.State())
		s.Equal(model.VolumeRangeInstance, storedRangeCapability.State().(model.RangeCapabilityState).Instance)
		s.Equal(20.0, storedRangeCapability.State().(model.RangeCapabilityState).Value)

		// Did not override old state with nil state
		storedColorCapability, ok := storedDevice.GetCapabilityByTypeAndInstance(colorCapability.Type(), colorCapability.Instance())
		s.Require().True(ok)
		s.Require().NotNil(storedColorCapability.State())
		s.Require().IsType(model.ColorSettingCapabilityState{}, storedColorCapability.State())
		s.Equal(model.TemperatureKCapabilityInstance, storedColorCapability.State().(model.ColorSettingCapabilityState).Instance)
		s.Equal(model.TemperatureK(150), storedColorCapability.State().(model.ColorSettingCapabilityState).Value)

		// Added
		storedToggleCapability, ok := storedDevice.GetCapabilityByTypeAndInstance(toggleCapability.Type(), toggleCapability.Instance())
		s.Require().True(ok)
		s.Require().NotNil(storedToggleCapability.State())
		s.Require().IsType(model.ToggleCapabilityState{}, storedToggleCapability.State())
		s.Equal(model.MuteToggleCapabilityInstance, storedToggleCapability.State().(model.ToggleCapabilityState).Instance)
		s.Equal(false, storedToggleCapability.State().(model.ToggleCapabilityState).Value)
	})

	s.Run("check store device with new parameters", func() {
		user := data.GenerateUser()
		if err := s.dbClient.StoreUser(s.context, user); err != nil {
			s.dataPreparationFailed(err)
			return
		}

		modeCapability := model.MakeCapabilityByType(model.ModeCapabilityType).
			WithRetrievable(true).
			WithParameters(model.ModeCapabilityParameters{
				Instance: model.TeaModeInstance,
				Modes: []model.Mode{
					{Value: model.BlackTeaMode},
					{Value: model.GreenTeaMode},
					{Value: model.PuerhTeaMode},
				},
			}).
			WithState(model.ModeCapabilityState{Instance: model.TeaModeInstance, Value: model.BlackTeaMode})

		rangeCapability := model.MakeCapabilityByType(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.VolumeRangeInstance,
				Unit:         model.UnitPercent,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min: 0,
					Max: 100,
				},
			}).
			WithState(model.RangeCapabilityState{Instance: model.VolumeRangeInstance, Value: 10})

		colorCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType).
			WithRetrievable(true).
			WithParameters(model.ColorSettingCapabilityParameters{
				TemperatureK: &model.TemperatureKParameters{
					Min: 0,
					Max: 300,
				},
			}).
			WithState(model.ColorSettingCapabilityState{Instance: model.TemperatureKCapabilityInstance, Value: model.TemperatureK(150)})

		toggleCapability := model.MakeCapabilityByType(model.ToggleCapabilityType).
			WithRetrievable(true).
			WithParameters(model.ToggleCapabilityParameters{Instance: model.MuteToggleCapabilityInstance}).
			WithState(model.ToggleCapabilityState{Instance: model.MuteToggleCapabilityInstance, Value: false})

		device := model.NewDevice("Лампочка").
			WithDeviceType(model.LightDeviceType).
			WithSkillID(model.VIRTUAL).
			WithExternalID("id").
			WithCapabilities(
				modeCapability,
				rangeCapability,
				colorCapability,
				toggleCapability,
			)

		storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, *device)
		s.NoError(err)
		device.ID = storedDevice.ID
		s.Equal(model.StoreResultNew, storeResult)

		device.Capabilities = model.Capabilities{
			modeCapability.
				WithParameters(model.ModeCapabilityParameters{
					Instance: model.TeaModeInstance,
					Modes: []model.Mode{
						{Value: model.BlackTeaMode},
						{Value: model.WhiteTeaMode},
						{Value: model.OolongTeaMode},
					},
				}).
				WithState((*model.ModeCapabilityState)(nil)),
			rangeCapability.
				WithParameters(model.RangeCapabilityParameters{
					Instance:     model.BrightnessRangeInstance,
					Unit:         model.UnitPercent,
					RandomAccess: true,
					Looped:       false,
					Range: &model.Range{
						Min: 0,
						Max: 100,
					},
				}).
				WithState((*model.RangeCapabilityState)(nil)),
			colorCapability.
				WithParameters(model.ColorSettingCapabilityParameters{
					TemperatureK: &model.TemperatureKParameters{
						Min: 500,
						Max: 1000,
					},
				}).
				WithState((*model.ColorSettingCapabilityState)(nil)),
			toggleCapability.
				WithParameters(model.ToggleCapabilityParameters{Instance: model.KeepWarmToggleCapabilityInstance}).
				WithState((*model.ToggleCapabilityState)(nil)),
		}

		storedDevice, storeResult, err = s.dbClient.StoreUserDevice(s.context, user, *device)
		s.NoError(err)
		s.Equal(model.StoreResultUpdated, storeResult)
		s.Equal(device.ID, storedDevice.ID)

		storedDevice, err = s.dbClient.SelectUserDevice(s.context, user.ID, device.ID)
		s.NoError(err)
		s.Equal(4, len(storedDevice.Capabilities))

		// State didn't change because parameters are compatible
		storedModeCapability, ok := storedDevice.GetCapabilityByTypeAndInstance(modeCapability.Type(), modeCapability.Instance())
		s.Require().True(ok)
		s.Require().NotNil(storedModeCapability.State())
		s.Require().IsType(model.ModeCapabilityState{}, storedModeCapability.State())
		s.Equal(model.TeaModeInstance, storedModeCapability.State().(model.ModeCapabilityState).Instance)
		s.Equal(model.BlackTeaMode, storedModeCapability.State().(model.ModeCapabilityState).Value)

		// State became nil because parameters are not compatible
		storedRangeCapability, ok := storedDevice.GetCapabilityByTypeAndInstance(rangeCapability.Type(), rangeCapability.Instance())
		s.Require().True(ok)
		s.Nil(storedRangeCapability.State())

		// State became nil because old state is not in range of new parameters
		storedColorCapability, ok := storedDevice.GetCapabilityByTypeAndInstance(colorCapability.Type(), colorCapability.Instance())
		s.Require().True(ok)
		s.Nil(storedColorCapability.State())

		// State became nil because parameters are not compatible
		storedToggleCapability, ok := storedDevice.GetCapabilityByTypeAndInstance(toggleCapability.Type(), toggleCapability.Instance())
		s.Require().True(ok)
		s.Nil(storedToggleCapability.State())
	})
}
