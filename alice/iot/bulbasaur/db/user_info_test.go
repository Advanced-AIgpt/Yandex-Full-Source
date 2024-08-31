package db

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
)

func (s *DBClientSuite) TestSelectUserInfo() {
	s.Run("Unknown user", func() {
		user := data.GenerateUser()
		s.checkUserInfo(user.ID, model.UserInfo{}, &model.UnknownUserError{})
	})
	s.Run("Empty user", func() {
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
		s.checkUserInfo(user.ID, model.UserInfo{
			Households:         model.Households{currentHousehold},
			CurrentHouseholdID: currentHousehold.ID,
		}, nil)
	})
	s.Run("Devices", func() {
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

			devices := make([]model.Device, 3)
			for i := range devices {
				devices[i] = data.GenerateDevice()
				storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, devices[i])
				if err != nil {
					s.dataPreparationFailed(err)
					return
				}
				s.Equal(model.StoreResultNew, storeResult)
				devices[i] = formatDeviceSelectUserInfo(s.context, storedDevice)
			}

			s.checkUserInfo(user.ID, model.UserInfo{
				Devices:            devices,
				Households:         model.Households{currentHousehold},
				CurrentHouseholdID: currentHousehold.ID,
			}, nil)
		})
		s.Run("Deleted", func() {
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

			device1 := data.GenerateDevice()
			storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, device1)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			device1 = formatDeviceSelectUserInfo(s.context, storedDevice)

			device2 := data.GenerateDevice()
			storedDevice, _, err = s.dbClient.StoreUserDevice(s.context, user, device2)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			device2 = formatDeviceSelectUserInfo(s.context, storedDevice)

			err = s.dbClient.DeleteUserDevice(s.context, user.ID, device1.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserInfo(user.ID, model.UserInfo{
				Devices:            []model.Device{device2},
				Households:         model.Households{currentHousehold},
				CurrentHouseholdID: currentHousehold.ID,
			}, nil)
		})
	})
	s.Run("Groups", func() {
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

			device := data.GenerateDevice()
			storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, device)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			device = formatDeviceSelectUserInfo(s.context, storedDevice)

			group1 := data.GenerateGroup()
			group1.Name = tools.StandardizeSpaces(group1.Name)
			group1.HouseholdID = currentHousehold.ID
			group1.ID, err = s.dbClient.CreateUserGroup(s.context, user, group1)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			group2 := data.GenerateGroup()
			group2.Name = tools.StandardizeSpaces(group2.Name)
			group2.HouseholdID = currentHousehold.ID
			group2.ID, err = s.dbClient.CreateUserGroup(s.context, user, group2)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			err = s.dbClient.UpdateUserDeviceGroups(s.context, user, device.ID, []string{group1.ID, group2.ID})
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			group1.Type = device.Type
			group1.Devices = []string{device.ID}
			group2.Type = device.Type
			group2.Devices = []string{device.ID}

			device.Groups = []model.Group{
				group1, group2,
			}
			s.checkUserInfo(user.ID, model.UserInfo{
				Devices:            []model.Device{device},
				Groups:             []model.Group{group1, group2},
				Households:         model.Households{currentHousehold},
				CurrentHouseholdID: currentHousehold.ID,
			}, nil)
		})
		s.Run("Not linked", func() {
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
			storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, device)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			device = formatDeviceSelectUserInfo(s.context, storedDevice)

			group1 := data.GenerateGroup()
			group1.Name = tools.StandardizeSpaces(group1.Name)
			group1.HouseholdID = currentHousehold.ID
			group1.ID, err = s.dbClient.CreateUserGroup(s.context, user, group1)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			group2 := data.GenerateGroup()
			group2.Name = tools.StandardizeSpaces(group2.Name)
			group2.HouseholdID = currentHousehold.ID
			group2.ID, err = s.dbClient.CreateUserGroup(s.context, user, group2)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			err = s.dbClient.UpdateUserDeviceGroups(s.context, user, device.ID, []string{group1.ID})
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			group1.Type = device.Type
			group1.Devices = []string{device.ID}
			device.Groups = []model.Group{
				group1,
			}

			s.checkUserInfo(user.ID, model.UserInfo{
				Devices:            []model.Device{device},
				Groups:             []model.Group{group1},
				Households:         model.Households{currentHousehold},
				CurrentHouseholdID: currentHousehold.ID,
			}, nil)
		})
		s.Run("Deleted device", func() {
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
			storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, device)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			device = formatDeviceSelectUserInfo(s.context, storedDevice)

			group := data.GenerateGroup()
			group.Name = tools.StandardizeSpaces(group.Name)
			group.HouseholdID = currentHousehold.ID
			group.ID, err = s.dbClient.CreateUserGroup(s.context, user, group)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			err = s.dbClient.UpdateUserDeviceGroups(s.context, user, device.ID, []string{group.ID})
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			group.Type = device.Type

			err = s.dbClient.DeleteUserDevice(s.context, user.ID, device.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserInfo(user.ID, model.UserInfo{
				Households:         model.Households{currentHousehold},
				CurrentHouseholdID: currentHousehold.ID,
			}, nil)
		})
		s.Run("Deleted group", func() {
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
			storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, device)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			device = formatDeviceSelectUserInfo(s.context, storedDevice)

			group := data.GenerateGroup()
			group.Name = tools.StandardizeSpaces(group.Name)
			group.HouseholdID = currentHousehold.ID
			group.ID, err = s.dbClient.CreateUserGroup(s.context, user, group)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			err = s.dbClient.UpdateUserDeviceGroups(s.context, user, device.ID, []string{group.ID})
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			group.Type = device.Type

			err = s.dbClient.DeleteUserGroup(s.context, user.ID, group.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserInfo(user.ID, model.UserInfo{
				Devices:            []model.Device{device},
				Households:         model.Households{currentHousehold},
				CurrentHouseholdID: currentHousehold.ID,
			}, nil)
		})
		s.Run("MultipleDevices", func() {
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

			device1 := data.GenerateDevice()
			storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, device1)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			device1 = formatDeviceSelectUserInfo(s.context, storedDevice)

			device2 := data.GenerateDevice()
			device2.Type = device1.Type
			storedDevice, _, err = s.dbClient.StoreUserDevice(s.context, user, device2)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			device2 = formatDeviceSelectUserInfo(s.context, storedDevice)

			group := data.GenerateGroup()
			group.Name = tools.StandardizeSpaces(group.Name)
			group.HouseholdID = currentHousehold.ID
			group.ID, err = s.dbClient.CreateUserGroup(s.context, user, group)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			for _, deviceID := range []string{device1.ID, device2.ID} {
				err = s.dbClient.UpdateUserDeviceGroups(s.context, user, deviceID, []string{group.ID})
				if err != nil {
					s.dataPreparationFailed(err)
					return
				}
			}
			group.Type = device1.Type
			group.Devices = []string{device1.ID, device2.ID}
			device1.Groups = []model.Group{
				group,
			}
			device2.Groups = []model.Group{
				group,
			}
			s.checkUserInfo(user.ID, model.UserInfo{
				Devices:            []model.Device{device1, device2},
				Groups:             []model.Group{group},
				Households:         model.Households{currentHousehold},
				CurrentHouseholdID: currentHousehold.ID,
			}, nil)
		})
	})
	s.Run("Rooms", func() {
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

			device := data.GenerateDevice()
			storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, device)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			device = formatDeviceSelectUserInfo(s.context, storedDevice)

			room := data.GenerateRoom()
			room.Name = tools.StandardizeSpaces(room.Name)
			room.HouseholdID = currentHousehold.ID
			room.ID, err = s.dbClient.CreateUserRoom(s.context, user, room)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			err = s.dbClient.UpdateUserDeviceRoom(s.context, user.ID, device.ID, room.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			room.Devices = []string{device.ID}
			device.Room = &room
			s.checkUserInfo(user.ID, model.UserInfo{
				CurrentHouseholdID: currentHousehold.ID,
				Devices:            []model.Device{device},
				Rooms:              []model.Room{room},
				Households:         model.Households{currentHousehold},
			}, nil)
		})
		s.Run("Not linked", func() {
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
			storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, device)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			device = formatDeviceSelectUserInfo(s.context, storedDevice)

			room := data.GenerateRoom()
			room.Name = tools.StandardizeSpaces(room.Name)
			room.HouseholdID = currentHousehold.ID
			room.ID, err = s.dbClient.CreateUserRoom(s.context, user, room)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserInfo(user.ID, model.UserInfo{
				Devices:            []model.Device{device},
				Households:         model.Households{currentHousehold},
				CurrentHouseholdID: currentHousehold.ID,
			}, nil)
		})
		s.Run("Deleted device", func() {
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
			storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, device)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			device = formatDeviceSelectUserInfo(s.context, storedDevice)

			room := data.GenerateRoom()
			room.Name = tools.StandardizeSpaces(room.Name)
			room.HouseholdID = currentHousehold.ID
			room.ID, err = s.dbClient.CreateUserRoom(s.context, user, room)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			err = s.dbClient.UpdateUserDeviceRoom(s.context, user.ID, device.ID, room.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			err = s.dbClient.DeleteUserDevice(s.context, user.ID, device.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserInfo(user.ID, model.UserInfo{
				Households:         model.Households{currentHousehold},
				CurrentHouseholdID: currentHousehold.ID,
			}, nil)
		})
		s.Run("Deleted room", func() {
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
			storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, device)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			device = formatDeviceSelectUserInfo(s.context, storedDevice)

			room := data.GenerateRoom()
			room.Name = tools.StandardizeSpaces(room.Name)
			room.HouseholdID = currentHousehold.ID
			room.ID, err = s.dbClient.CreateUserRoom(s.context, user, room)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			err = s.dbClient.UpdateUserDeviceRoom(s.context, user.ID, device.ID, room.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			err = s.dbClient.DeleteUserRoom(s.context, user.ID, room.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserInfo(user.ID, model.UserInfo{
				Devices:            []model.Device{device},
				Households:         model.Households{currentHousehold},
				CurrentHouseholdID: currentHousehold.ID,
			}, nil)
		})
	})
	s.Run("Scenarios", func() {
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

			scenarios := make([]model.Scenario, 3)
			for i := range scenarios {
				scenarios[i] = data.GenerateScenario("", nil)
				scenarios[i].ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenarios[i])
				scenarios[i].Name = model.ScenarioName(tools.StandardizeSpaces(string(scenarios[i].Name)))
				if err != nil {
					s.dataPreparationFailed(err)
					return
				}
			}

			s.checkUserInfo(user.ID, model.UserInfo{
				Scenarios:          scenarios,
				Households:         model.Households{currentHousehold},
				CurrentHouseholdID: currentHousehold.ID,
			}, nil)
		})
		s.Run("Deleted", func() {
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

			scenario1 := data.GenerateScenario("", nil)
			scenario1.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario1)
			scenario1.Name = model.ScenarioName(tools.StandardizeSpaces(string(scenario1.Name)))
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			scenario2 := data.GenerateScenario("", nil)
			scenario2.ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenario2)
			scenario2.Name = model.ScenarioName(tools.StandardizeSpaces(string(scenario2.Name)))
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			err = s.dbClient.DeleteScenario(s.context, user.ID, scenario1.ID)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserInfo(user.ID, model.UserInfo{
				Scenarios:          []model.Scenario{scenario2},
				Households:         model.Households{currentHousehold},
				CurrentHouseholdID: currentHousehold.ID,
			}, nil)
		})
		s.Run("Deactivated scenarios are returned from db", func() {
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
			scenarios := make([]model.Scenario, 3)
			for i := range scenarios {
				scenarios[i] = data.GenerateScenario("", nil)
				scenarios[i].ID, err = s.dbClient.CreateScenario(s.context, user.ID, scenarios[i])
				scenarios[i].Name = model.ScenarioName(tools.StandardizeSpaces(string(scenarios[i].Name)))
				if err != nil {
					s.dataPreparationFailed(err)
					return
				}
			}

			s.checkUserInfo(user.ID, model.UserInfo{
				Scenarios:          scenarios,
				Households:         model.Households{currentHousehold},
				CurrentHouseholdID: currentHousehold.ID,
			}, nil)

			// deactivate
			scenarios[0].IsActive = false
			if err = s.dbClient.UpdateScenario(s.context, user.ID, scenarios[0]); err != nil {
				s.dataPreparationFailed(err)
				return
			}
			s.checkUserInfo(user.ID, model.UserInfo{
				Scenarios:          scenarios,
				Households:         model.Households{currentHousehold},
				CurrentHouseholdID: currentHousehold.ID,
			}, nil)
		})
		s.Run("Stereopairs", func() {
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

			deviceLeader := data.GenerateDevice()
			storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, deviceLeader)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			deviceLeader = formatDeviceSelectUserInfo(s.context, storedDevice)

			deviceFollower := data.GenerateDevice()
			storedDevice, _, err = s.dbClient.StoreUserDevice(s.context, user, deviceFollower)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			deviceFollower = formatDeviceSelectUserInfo(s.context, storedDevice)

			sp := data.GenerateStereopair(deviceLeader, deviceFollower)
			err = s.dbClient.StoreStereopair(s.context, user.ID, sp)
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}

			s.checkUserInfo(user.ID, model.UserInfo{
				Devices:            []model.Device{deviceLeader, deviceFollower},
				Stereopairs:        model.Stereopairs{sp},
				Households:         model.Households{currentHousehold},
				CurrentHouseholdID: currentHousehold.ID,
			}, nil)
		})
	})
}
