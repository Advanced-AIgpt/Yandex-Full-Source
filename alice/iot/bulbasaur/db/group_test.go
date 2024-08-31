package db

import (
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/random"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *DBClientSuite) Test_GroupNameNormalization() {
	var (
		id  string
		err error
	)

	user := data.GenerateUser()
	err = s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	// 1
	_, err = s.dbClient.CreateUserGroup(s.context, user, model.Group{Name: "  some    group        123 "})
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	_, err = s.dbClient.CreateUserGroup(s.context, user, model.Group{Name: "some group 123"})
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	_, err = s.dbClient.CreateUserGroup(s.context, user, model.Group{Name: "some group 123      "})
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	_, err = s.dbClient.CreateUserGroup(s.context, user, model.Group{Name: "    Some group 123      "})
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	_, err = s.dbClient.CreateUserGroup(s.context, user, model.Group{Name: "soMe  grOuP  123"})
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	// 2
	_, err = s.dbClient.CreateUserGroup(s.context, user, model.Group{Name: ""})
	s.True(xerrors.Is(err, &model.InvalidValueError{}))

	_, err = s.dbClient.CreateUserGroup(s.context, user, model.Group{Name: "   "})
	s.True(xerrors.Is(err, &model.InvalidValueError{}))

	// 3
	id, err = s.dbClient.CreateUserGroup(s.context, user, model.Group{Name: "another group"})
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	err = s.dbClient.UpdateUserGroupName(s.context, user, id, "another group")
	s.NoError(err)

	err = s.dbClient.UpdateUserGroupName(s.context, user, id, "another    group")
	s.NoError(err)

	err = s.dbClient.UpdateUserGroupName(s.context, user, id, "   anOtHer grOup   ")
	s.NoError(err)

	err = s.dbClient.UpdateUserGroupName(s.context, user, id, "some group123")
	s.NoError(err)

	err = s.dbClient.UpdateUserGroupName(s.context, user, id, "somegroup123")
	s.NoError(err)

	// 4
	err = s.dbClient.UpdateUserGroupName(s.context, user, id, "sOme grOup 123")
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	err = s.dbClient.UpdateUserGroupName(s.context, user, id, "some group 123      ")
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	err = s.dbClient.UpdateUserGroupName(s.context, user, id, "    some group 123      ")
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	err = s.dbClient.UpdateUserGroupName(s.context, user, id, "some  group  123")
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	// 5
	err = s.dbClient.UpdateUserGroupName(s.context, user, id, "")
	s.True(xerrors.Is(err, &model.InvalidValueError{}))

	err = s.dbClient.UpdateUserGroupName(s.context, user, id, "     ")
	s.True(xerrors.Is(err, &model.InvalidValueError{}))
}

func (s *DBClientSuite) Test_Group() {
	var (
		actualGroup model.Group
		err         error
	)

	user := data.GenerateUser()
	err = s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	userCurrentHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, user.ID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	evilUser := data.GenerateUser()
	err = s.dbClient.StoreUser(s.context, evilUser)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	evilCurrentHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, evilUser.ID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	// 1. Should proper save and read groups
	// 1.1
	actualGroup, err = s.dbClient.SelectUserGroup(s.context, data.GenerateUser().ID, "group-id")
	s.True(xerrors.Is(err, &model.GroupNotFoundError{}))
	s.Equal(model.Group{}, actualGroup)

	// TODO: SelectUserRooms and SelectNonEmptyUserGroups returns nil; SelectUserGroups returns []model.Group{}
	s.checkUserGroups(data.GenerateUser().ID, []model.Group{}, nil)

	// 1.2
	actualGroup, err = s.dbClient.SelectUserGroup(s.context, user.ID, "unknown-group-id")
	s.True(xerrors.Is(err, &model.GroupNotFoundError{}))
	s.Equal(model.Group{}, actualGroup)

	s.checkUserGroups(user.ID, []model.Group{}, nil)

	// 1.3 create group1
	group1 := data.GenerateGroup()
	group1.Name = tools.StandardizeSpaces(group1.Name)
	group1.Devices = []string{}
	group1.ID, err = s.dbClient.CreateUserGroup(s.context, user, group1)
	group1.HouseholdID = userCurrentHousehold.ID
	s.NoError(err)

	s.checkUserGroups(user.ID, []model.Group{group1}, nil)

	// 1.4 use name of group1
	_, err = s.dbClient.CreateUserGroup(s.context, user, group1)
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	s.checkUserGroups(user.ID, []model.Group{group1}, nil)

	// 1.5 create group2
	group2 := data.GenerateGroup()
	group2.Name = tools.StandardizeSpaces(group2.Name)
	group2.Devices = []string{}
	group2.ID, err = s.dbClient.CreateUserGroup(s.context, user, group2)
	group2.HouseholdID = userCurrentHousehold.ID
	s.NoError(err)

	s.checkUserGroups(user.ID, []model.Group{group1, group2}, nil)

	// 2. Should proper delete groups
	// 2.1
	err = s.dbClient.DeleteUserGroup(s.context, user.ID, group2.ID)
	s.NoError(err)

	actualGroup, err = s.dbClient.SelectUserGroup(s.context, user.ID, group2.ID)
	s.True(xerrors.Is(err, &model.GroupNotFoundError{}))
	s.Equal(model.Group{}, actualGroup)

	s.checkUserGroups(user.ID, []model.Group{group1}, nil)

	// 2.2
	err = s.dbClient.DeleteUserGroup(s.context, user.ID, group2.ID)
	s.NoError(err)

	s.checkUserGroups(user.ID, []model.Group{group1}, nil)

	actualGroup, err = s.dbClient.SelectUserGroup(s.context, user.ID, group2.ID)
	s.True(xerrors.Is(err, &model.GroupNotFoundError{}))
	s.Equal(model.Group{}, actualGroup)

	// 2.3
	err = s.dbClient.DeleteUserGroup(s.context, user.ID, "unknown-group-id")
	s.NoError(err)

	s.checkUserGroups(user.ID, []model.Group{group1}, nil)

	// 3. Should proper rename groups
	// 3.1
	newName := data.GenerateGroup().Name
	group1.Name = tools.StandardizeSpaces(newName)

	err = s.dbClient.UpdateUserGroupName(s.context, user, group1.ID, newName)
	s.NoError(err)

	s.checkUserGroups(user.ID, []model.Group{group1}, nil)

	// yes, it's second call. UpdateUserGroupName should not fail if name did not changed
	err = s.dbClient.UpdateUserGroupName(s.context, user, group1.ID, newName)
	s.NoError(err)

	s.checkUserGroups(user.ID, []model.Group{group1}, nil)

	// 3.2
	err = s.dbClient.UpdateUserGroupName(s.context, user, group2.ID, group1.Name)
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	// 3.3
	err = s.dbClient.UpdateUserGroupName(s.context, user, "unknown-group", "some-mane")
	s.True(xerrors.Is(err, &model.GroupNotFoundError{}))

	// 3.4
	group3 := data.GenerateGroup()
	group3.Name = tools.StandardizeSpaces(group3.Name)
	group3.Devices = []string{}
	group3.ID, err = s.dbClient.CreateUserGroup(s.context, user, group3)
	group3.HouseholdID = userCurrentHousehold.ID
	s.NoError(err)

	err = s.dbClient.UpdateUserGroupName(s.context, user, group1.ID, group3.Name)
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	s.checkUserGroups(user.ID, []model.Group{group1, group3}, nil)

	// 4. Should proper control grants
	actualGroup, err = s.dbClient.SelectUserGroup(s.context, evilUser.ID, group1.ID)
	s.True(xerrors.Is(err, &model.GroupNotFoundError{}))
	s.Equal(model.Group{}, actualGroup)

	err = s.dbClient.UpdateUserGroupName(s.context, evilUser, group1.ID, group3.Name)
	s.True(xerrors.Is(err, &model.GroupNotFoundError{}))

	err = s.dbClient.DeleteUserGroup(s.context, evilUser.ID, group1.ID)
	s.NoError(err)

	err = s.dbClient.DeleteUserGroup(s.context, evilUser.ID, group1.ID)
	s.NoError(err)

	s.checkUserGroups(user.ID, []model.Group{group1, group3}, nil)
	s.checkUserGroups(evilUser.ID, []model.Group{}, nil)

	// corner: evilUser with groups !== evilUser w/o groups
	evilGroup := data.GenerateGroup()
	evilGroup.Name = tools.StandardizeSpaces(evilGroup.Name)
	evilGroup.Devices = []string{}
	evilGroup.ID, err = s.dbClient.CreateUserGroup(s.context, evilUser, evilGroup)
	evilGroup.HouseholdID = evilCurrentHousehold.ID
	s.NoError(err)

	actualGroup, err = s.dbClient.SelectUserGroup(s.context, evilUser.ID, group1.ID)
	s.True(xerrors.Is(err, &model.GroupNotFoundError{}))
	s.Equal(model.Group{}, actualGroup)

	err = s.dbClient.UpdateUserGroupName(s.context, evilUser, group1.ID, group3.Name)
	s.True(xerrors.Is(err, &model.GroupNotFoundError{}))

	err = s.dbClient.DeleteUserGroup(s.context, evilUser.ID, group1.ID)
	s.NoError(err)

	err = s.dbClient.DeleteUserGroup(s.context, evilUser.ID, group1.ID)
	s.NoError(err)

	s.checkUserGroups(user.ID, []model.Group{group1, group3}, nil)
	s.checkUserGroups(evilUser.ID, []model.Group{evilGroup}, nil)
}

func (s *DBClientSuite) Test_GroupsAndDevices() {
	var (
		id  string
		err error
	)

	user := data.GenerateUser()
	err = s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	currentHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, user.ID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	id, err = s.dbClient.CreateUserGroup(s.context, user, data.GenerateGroup())
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	group1, err := s.dbClient.SelectUserGroup(s.context, user.ID, id)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	id, err = s.dbClient.CreateUserGroup(s.context, user, data.GenerateGroup())
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	group2, err := s.dbClient.SelectUserGroup(s.context, user.ID, id)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, data.GenerateDevice())
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	deviceA1, err := s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	deviceA2 := data.GenerateDevice()
	deviceA2.Type = deviceA1.Type // hack
	deviceA2.HouseholdID = currentHousehold.ID
	storedDevice, _, err = s.dbClient.StoreUserDevice(s.context, user, deviceA2)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	deviceA2.ID = storedDevice.ID
	deviceA2, err = s.dbClient.SelectUserDevice(s.context, user.ID, deviceA2.ID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	deviceB1 := data.GenerateDevice()
	for deviceB1.Type == deviceA1.Type { // hack
		deviceB1 = data.GenerateDevice()
	}
	storedDevice, _, err = s.dbClient.StoreUserDevice(s.context, user, deviceB1)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	deviceB1.ID = storedDevice.ID
	deviceB1.HouseholdID = storedDevice.HouseholdID
	deviceB1, err = s.dbClient.SelectUserDevice(s.context, user.ID, deviceB1.ID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	evilUser := data.GenerateUser()
	err = s.dbClient.StoreUser(s.context, evilUser)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	storedDevice, _, err = s.dbClient.StoreUserDevice(s.context, evilUser, data.GenerateDevice())
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	evilDevice, err := s.dbClient.SelectUserDevice(s.context, evilUser.ID, storedDevice.ID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	// 0. Pre-check
	// 0.1
	group1.Devices = []string{}
	group2.Devices = []string{}

	deviceA1.Groups = nil
	deviceA2.Groups = nil
	deviceB1.Groups = nil

	s.checkUserGroups(user.ID, []model.Group{group1, group2}, nil)
	s.checkUserDevices(user.ID, []model.Device{deviceA1, deviceA2, deviceB1})

	// 0.2
	evilDevice.Groups = nil

	s.checkUserGroups(evilUser.ID, []model.Group{}, nil)
	s.checkUserDevices(evilUser.ID, []model.Device{evilDevice})

	// 1. Should proper add device to group
	// 1.1
	err = s.dbClient.UpdateUserDeviceGroups(s.context, user, deviceA1.ID, []string{group1.ID})
	s.NoError(err)

	group1.Devices = []string{deviceA1.ID}
	group1.Type = deviceA1.Type
	group2.Devices = []string{}
	group2.Type = ""

	deviceA1.Groups = []model.Group{formatGroupSelectUserDevice(group1)}
	deviceA2.Groups = nil
	deviceB1.Groups = nil

	s.checkUserGroups(user.ID, []model.Group{group1, group2}, []model.Group{group1})
	s.checkUserDevices(user.ID, []model.Device{deviceA1, deviceA2, deviceB1})

	// 1.2
	err = s.dbClient.UpdateUserDeviceGroups(s.context, user, deviceA2.ID, []string{group1.ID, group2.ID})
	s.NoError(err)

	group1.Devices = []string{deviceA1.ID, deviceA2.ID}
	group1.Type = deviceA1.Type
	group2.Devices = []string{deviceA2.ID}
	group2.Type = deviceA1.Type

	deviceA1.Groups = []model.Group{formatGroupSelectUserDevice(group1)}
	deviceA2.Groups = []model.Group{formatGroupSelectUserDevice(group1), formatGroupSelectUserDevice(group2)}
	deviceB1.Groups = nil

	s.checkUserGroups(user.ID, []model.Group{group1, group2}, []model.Group{group1, group2})
	s.checkUserDevices(user.ID, []model.Device{deviceA1, deviceA2, deviceB1})

	// 1.3 group.Type != device.Type
	err = s.dbClient.UpdateUserDeviceGroups(s.context, user, deviceB1.ID, []string{group1.ID})
	s.True(xerrors.Is(err, &model.IncompatibleDeviceTypeError{}))

	s.checkUserGroups(user.ID, []model.Group{group1, group2}, []model.Group{group1, group2})
	s.checkUserDevices(user.ID, []model.Device{deviceA1, deviceA2, deviceB1})

	// 1.4 user w/o groups
	err = s.dbClient.UpdateUserDeviceGroups(s.context, evilUser, evilDevice.ID, []string{"not-exist"})
	s.True(xerrors.Is(err, &model.GroupNotFoundError{}))

	s.checkUserGroups(evilUser.ID, []model.Group{}, nil)
	s.checkUserDevices(evilUser.ID, []model.Device{evilDevice})

	// 1.5 user with groups, group does not exist
	err = s.dbClient.UpdateUserDeviceGroups(s.context, user, deviceA2.ID, []string{"not-exist"})
	s.True(xerrors.Is(err, &model.GroupNotFoundError{}))

	s.checkUserGroups(user.ID, []model.Group{group1, group2}, []model.Group{group1, group2})
	s.checkUserDevices(user.ID, []model.Device{deviceA1, deviceA2, deviceB1})

	// 1.6 unknown device
	err = s.dbClient.UpdateUserDeviceGroups(s.context, user, "unknown-device-id", []string{group1.ID})
	s.True(xerrors.Is(err, &model.DeviceNotFoundError{}))

	s.checkUserGroups(user.ID, []model.Group{group1, group2}, []model.Group{group1, group2})
	s.checkUserDevices(user.ID, []model.Device{deviceA1, deviceA2, deviceB1})

	// 2. Should proper change device group
	// 2.1 no group type updates
	err = s.dbClient.UpdateUserDeviceGroups(s.context, user, deviceA1.ID, []string{group2.ID})
	s.NoError(err)

	group1.Devices = []string{deviceA2.ID}
	group1.Type = deviceA1.Type
	group2.Devices = []string{deviceA1.ID, deviceA2.ID}
	group2.Type = deviceA1.Type

	deviceA1.Groups = []model.Group{formatGroupSelectUserDevice(group2)}
	deviceA2.Groups = []model.Group{formatGroupSelectUserDevice(group1), formatGroupSelectUserDevice(group2)}
	deviceB1.Groups = nil

	s.checkUserGroups(user.ID, []model.Group{group1, group2}, []model.Group{group1, group2})
	s.checkUserDevices(user.ID, []model.Device{deviceA1, deviceA2, deviceB1})

	// 2.2 remove device from all groups
	err = s.dbClient.UpdateUserDeviceGroups(s.context, user, deviceA2.ID, []string{})
	s.NoError(err)

	group1.Devices = []string{}
	group1.Type = ""
	group2.Devices = []string{deviceA1.ID}
	group2.Type = deviceA1.Type

	deviceA1.Groups = []model.Group{formatGroupSelectUserDevice(group2)}
	deviceA2.Groups = nil
	deviceB1.Groups = nil

	s.checkUserGroups(user.ID, []model.Group{group1, group2}, []model.Group{group2})
	s.checkUserDevices(user.ID, []model.Device{deviceA1, deviceA2, deviceB1})

	// 2.3 move device between groups
	err = s.dbClient.UpdateUserDeviceGroups(s.context, user, deviceA1.ID, []string{group1.ID})
	s.NoError(err)

	group1.Devices = []string{deviceA1.ID}
	group1.Type = deviceA1.Type
	group2.Devices = []string{}
	group2.Type = ""

	deviceA1.Groups = []model.Group{formatGroupSelectUserDevice(group1)}
	deviceA2.Groups = nil
	deviceB1.Groups = nil

	s.checkUserGroups(user.ID, []model.Group{group1, group2}, []model.Group{group1})
	s.checkUserDevices(user.ID, []model.Device{deviceA1, deviceA2, deviceB1})

	// 3. Should proper delete devices and groups
	// 3.0 prepare
	err = s.dbClient.UpdateUserDeviceGroups(s.context, user, deviceA1.ID, []string{group1.ID})
	s.NoError(err)
	err = s.dbClient.UpdateUserDeviceGroups(s.context, user, deviceA2.ID, []string{group1.ID})
	s.NoError(err)
	err = s.dbClient.UpdateUserDeviceGroups(s.context, user, deviceB1.ID, []string{group2.ID})
	s.NoError(err)

	group1.Devices = []string{deviceA1.ID, deviceA2.ID}
	group1.Type = deviceA1.Type
	group2.Devices = []string{deviceB1.ID}
	group2.Type = deviceB1.Type

	deviceA1.Groups = []model.Group{formatGroupSelectUserDevice(group1)}
	deviceA2.Groups = []model.Group{formatGroupSelectUserDevice(group1)}
	deviceB1.Groups = []model.Group{formatGroupSelectUserDevice(group2)}

	s.checkUserGroups(user.ID, []model.Group{group1, group2}, []model.Group{group1, group2})
	s.checkUserDevices(user.ID, []model.Device{deviceA1, deviceA2, deviceB1})

	// 3.1.1 delete group
	err = s.dbClient.DeleteUserGroup(s.context, user.ID, group1.ID)
	s.NoError(err)

	deviceA1.Groups = nil
	deviceA2.Groups = nil

	s.checkUserGroups(user.ID, []model.Group{group2}, []model.Group{group2})
	s.checkUserDevices(user.ID, []model.Device{deviceA1, deviceA2, deviceB1})

	// 3.1.2 group was already deleted
	err = s.dbClient.DeleteUserGroup(s.context, user.ID, group1.ID)
	s.NoError(err)

	s.checkUserGroups(user.ID, []model.Group{group2}, []model.Group{group2})
	s.checkUserDevices(user.ID, []model.Device{deviceA1, deviceA2, deviceB1})

	// 3.1.3 unknown group
	err = s.dbClient.DeleteUserGroup(s.context, user.ID, "not-exists")
	s.NoError(err)

	s.checkUserGroups(user.ID, []model.Group{group2}, []model.Group{group2})
	s.checkUserDevices(user.ID, []model.Device{deviceA1, deviceA2, deviceB1})

	// 3.1.4 unknown user
	err = s.dbClient.DeleteUserGroup(s.context, data.GenerateUser().ID, "not-exists")
	s.NoError(err)

	// 3.2.1 delete device
	err = s.dbClient.DeleteUserDevice(s.context, user.ID, deviceB1.ID)
	s.NoError(err)

	group2.Devices = []string{}
	group2.Type = ""

	// TODO: QUASAR-4275: IoT: метод SelectNonEmptyUserGroups не учитывает archived=true у устройств
	// должно быть так:
	// s.checkUserGroups(user.ID, []model.Group{group2}, nil)
	s.checkUserGroups(user.ID, []model.Group{group2}, []model.Group{group2})
	s.checkUserDevices(user.ID, []model.Device{deviceA1, deviceA2})
}

func (s *DBClientSuite) Test_MultiroomGroup() {
	s.Run("several multiroom speakers can be grouped", func() {
		// prepare data
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		id, err := s.dbClient.CreateUserGroup(s.context, user, data.GenerateGroup())
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		group, err := s.dbClient.SelectUserGroup(s.context, user.ID, id)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		multiroomSpeaker1 := data.GenerateDevice()
		multiroomSpeaker1.Type = model.YandexStationMini2DeviceType
		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, multiroomSpeaker1)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		multiroomSpeaker1 = storedDevice
		multiroomSpeaker1, err = s.dbClient.SelectUserDevice(s.context, user.ID, multiroomSpeaker1.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		multiroomSpeaker2 := data.GenerateDevice()
		multiroomSpeaker2.Type = model.YandexStation2DeviceType
		storedDevice, _, err = s.dbClient.StoreUserDevice(s.context, user, multiroomSpeaker2)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		multiroomSpeaker2 = storedDevice
		multiroomSpeaker2, err = s.dbClient.SelectUserDevice(s.context, user.ID, multiroomSpeaker2.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.Subtest("sanity check", func() {
			group.Devices = []string{}
			multiroomSpeaker1.Groups = nil
			multiroomSpeaker2.Groups = nil
			s.checkUserGroups(user.ID, []model.Group{group}, nil)
			s.checkUserDevices(user.ID, []model.Device{multiroomSpeaker1, multiroomSpeaker2})
		})

		s.Subtest("create group with single multiroom speaker", func() {
			err = s.dbClient.UpdateUserDeviceGroups(s.context, user, multiroomSpeaker1.ID, []string{group.ID})
			s.NoError(err)

			group.Devices = []string{multiroomSpeaker1.ID}
			group.Type = model.SmartSpeakerDeviceType
			multiroomSpeaker1.Groups = []model.Group{formatGroupSelectUserDevice(group)}

			s.checkUserGroups(user.ID, []model.Group{group}, nil) // second parameter is not used
			s.checkUserDevices(user.ID, []model.Device{multiroomSpeaker1, multiroomSpeaker2})
		})

		s.Subtest("add another multiroom speaker", func() {
			err = s.dbClient.UpdateUserDeviceGroups(s.context, user, multiroomSpeaker2.ID, []string{group.ID})
			s.NoError(err)

			group.Devices = []string{multiroomSpeaker1.ID, multiroomSpeaker2.ID}
			multiroomSpeaker2.Groups = []model.Group{formatGroupSelectUserDevice(group)}

			s.checkUserGroups(user.ID, []model.Group{group}, nil) // second parameter is not used
			s.checkUserDevices(user.ID, []model.Device{multiroomSpeaker1, multiroomSpeaker2})
		})
	})

	s.Run("non-multiroom devices can't be added to multiroom groups", func() {
		// prepare data
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
		id, err := s.dbClient.CreateUserGroup(s.context, user, data.GenerateGroup())
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		group, err := s.dbClient.SelectUserGroup(s.context, user.ID, id)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		multiroomSpeaker1 := data.GenerateDevice()
		multiroomSpeaker1.Type = model.YandexStationMini2DeviceType
		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, multiroomSpeaker1)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		multiroomSpeaker1 = storedDevice
		multiroomSpeaker1, err = s.dbClient.SelectUserDevice(s.context, user.ID, multiroomSpeaker1.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		nonMultiroomSpeaker1 := data.GenerateDevice()
		nonMultiroomSpeaker1.Type = model.DexpSmartBoxDeviceType
		storedDevice, _, err = s.dbClient.StoreUserDevice(s.context, user, nonMultiroomSpeaker1)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		nonMultiroomSpeaker1 = storedDevice
		nonMultiroomSpeaker1, err = s.dbClient.SelectUserDevice(s.context, user.ID, nonMultiroomSpeaker1.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		lamp := data.GenerateDevice()
		lamp.Type = model.LightDeviceType
		lamp.HouseholdID = currentHousehold.ID
		storedDevice, _, err = s.dbClient.StoreUserDevice(s.context, user, lamp)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		lamp = storedDevice
		lamp, err = s.dbClient.SelectUserDevice(s.context, user.ID, lamp.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.Subtest("sanity check", func() {
			group.Devices = []string{}
			multiroomSpeaker1.Groups = nil
			nonMultiroomSpeaker1.Groups = nil
			lamp.Groups = nil
			s.checkUserGroups(user.ID, []model.Group{group}, nil)
			s.checkUserDevices(user.ID, []model.Device{multiroomSpeaker1, nonMultiroomSpeaker1, lamp})
		})

		s.Subtest("create group with non-multiroom speaker fails", func() {
			err = s.dbClient.UpdateUserDeviceGroups(s.context, user, nonMultiroomSpeaker1.ID, []string{group.ID})
			if !xerrors.Is(err, &model.IncompatibleDeviceTypeError{}) {
				s.FailNow("error should be model.IncompatibleDeviceTypeError{}")
				return
			}

			s.checkUserGroups(user.ID, []model.Group{group}, nil) // second parameter is not used
			s.checkUserDevices(user.ID, []model.Device{multiroomSpeaker1, nonMultiroomSpeaker1, lamp})
		})

		s.Subtest("create group with single multiroom speaker", func() {
			err = s.dbClient.UpdateUserDeviceGroups(s.context, user, multiroomSpeaker1.ID, []string{group.ID})
			s.NoError(err)

			group.Devices = []string{multiroomSpeaker1.ID}
			group.Type = model.SmartSpeakerDeviceType
			multiroomSpeaker1.Groups = []model.Group{formatGroupSelectUserDevice(group)}

			s.checkUserGroups(user.ID, []model.Group{group}, nil) // second parameter is not used
			s.checkUserDevices(user.ID, []model.Device{multiroomSpeaker1, nonMultiroomSpeaker1, lamp})
		})

		s.Subtest("add non-multiroom speaker to group and fail", func() {
			err = s.dbClient.UpdateUserDeviceGroups(s.context, user, nonMultiroomSpeaker1.ID, []string{group.ID})
			if !xerrors.Is(err, &model.IncompatibleDeviceTypeError{}) {
				s.FailNow("error should be model.IncompatibleDeviceTypeError{}")
				return
			}

			s.checkUserGroups(user.ID, []model.Group{group}, nil) // second parameter is not used
			s.checkUserDevices(user.ID, []model.Device{multiroomSpeaker1, nonMultiroomSpeaker1, lamp})
		})

		s.Subtest("add lamp to group and fail", func() {
			err = s.dbClient.UpdateUserDeviceGroups(s.context, user, lamp.ID, []string{group.ID})
			if !xerrors.Is(err, &model.IncompatibleDeviceTypeError{}) {
				s.FailNow("error should be model.IncompatibleDeviceTypeError{}")
				return
			}

			s.checkUserGroups(user.ID, []model.Group{group}, nil) // second parameter is not used
			s.checkUserDevices(user.ID, []model.Device{multiroomSpeaker1, nonMultiroomSpeaker1, lamp})
		})
	})

	s.Run("speakers can't be added to normal device groups", func() {
		// prepare data
		user := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, user)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		id, err := s.dbClient.CreateUserGroup(s.context, user, data.GenerateGroup())
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		group, err := s.dbClient.SelectUserGroup(s.context, user.ID, id)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		multiroomSpeaker1 := data.GenerateDevice()
		multiroomSpeaker1.Type = model.YandexStationMini2DeviceType
		storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, multiroomSpeaker1)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		multiroomSpeaker1 = storedDevice
		multiroomSpeaker1, err = s.dbClient.SelectUserDevice(s.context, user.ID, multiroomSpeaker1.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		lamp := data.GenerateDevice()
		lamp.Type = model.LightDeviceType
		storedDevice, _, err = s.dbClient.StoreUserDevice(s.context, user, lamp)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		lamp = storedDevice
		lamp, err = s.dbClient.SelectUserDevice(s.context, user.ID, lamp.ID)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}

		s.Subtest("sanity check", func() {
			group.Devices = []string{}
			multiroomSpeaker1.Groups = nil
			lamp.Groups = nil
			s.checkUserGroups(user.ID, []model.Group{group}, nil)
			s.checkUserDevices(user.ID, []model.Device{multiroomSpeaker1, lamp})
		})

		s.Subtest("create lamp group", func() {
			err = s.dbClient.UpdateUserDeviceGroups(s.context, user, lamp.ID, []string{group.ID})
			s.NoError(err)

			group.Devices = []string{lamp.ID}
			group.Type = model.LightDeviceType
			lamp.Groups = []model.Group{formatGroupSelectUserDevice(group)}

			s.checkUserGroups(user.ID, []model.Group{group}, nil) // second parameter is not used
			s.checkUserDevices(user.ID, []model.Device{multiroomSpeaker1, lamp})
		})

		s.Subtest("multiroom speaker can't be added to light group", func() {
			err = s.dbClient.UpdateUserDeviceGroups(s.context, user, multiroomSpeaker1.ID, []string{group.ID})
			if !xerrors.Is(err, &model.IncompatibleDeviceTypeError{}) {
				s.FailNow("error should be model.IncompatibleDeviceTypeError{}")
				return
			}

			s.checkUserGroups(user.ID, []model.Group{group}, nil) // second parameter is not used
			s.checkUserDevices(user.ID, []model.Device{multiroomSpeaker1, lamp})
		})
	})
}

func (s *DBClientSuite) Test_DeviceGroupsCleanupAfterGroupRemoval() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	groupID, err := s.dbClient.CreateUserGroup(s.context, user, data.GenerateGroup())
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, data.GenerateDevice())
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	if err := s.dbClient.UpdateUserDeviceGroups(s.context, user, storedDevice.ID, []string{groupID}); err != nil {
		s.dataPreparationFailed(err)
		return
	}

	deviceGroups, err := s.dbClient.selectDeviceGroups(s.context, DeviceGroupsQueryCriteria{UserID: user.ID})
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	s.Len(deviceGroups, 1)

	if err = s.dbClient.DeleteUserGroup(s.context, user.ID, groupID); err != nil {
		s.Fail(err.Error(), "Failed to delete group")
	}

	deviceGroups, err = s.dbClient.selectDeviceGroups(s.context, DeviceGroupsQueryCriteria{UserID: user.ID})
	if err != nil {
		s.Fail(err.Error())
		return
	}
	s.Len(deviceGroups, 0, "Expected 0 device groups after group deletion")
}

func (s *DBClientSuite) Test_GroupAliasUpdate() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	group := data.GenerateGroup()

	groupID, err := s.dbClient.CreateUserGroup(s.context, user, group)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	aliases := []string{
		"группа",
		"группочка",
		"группонька",
	}

	err = s.dbClient.UpdateUserGroupNameAndAliases(s.context, user, groupID, group.Name, aliases)
	s.Require().NoError(err)

	selectedGroup, err := s.dbClient.SelectUserGroup(s.context, user.ID, groupID)
	s.Require().NoError(err)

	s.Equal(group.Name, selectedGroup.Name)
	s.Equal(aliases, selectedGroup.Aliases)

	newGroup := data.GenerateGroup()
	newGroup.Name = "ГруппА"
	_, err = s.dbClient.CreateUserGroup(s.context, user, newGroup)
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	generatedName := data.GenerateGroup().Name
	newGroup.Name = generatedName
	newGroup.ID, err = s.dbClient.CreateUserGroup(s.context, user, newGroup)
	s.Require().NoError(err)
	newAliases := []string{
		"  группА",
		"алиасочка",
		"алиасонька",
	}
	err = s.dbClient.UpdateUserGroupNameAndAliases(s.context, user, newGroup.ID, newGroup.Name, newAliases)
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))
	newAliases = []string{
		"алиасочка",
		"алиасонька",
	}
	err = s.dbClient.UpdateUserGroupNameAndAliases(s.context, user, newGroup.ID, newGroup.Name, newAliases)
	s.Require().NoError(err)

	selectedGroup, err = s.dbClient.SelectUserGroup(s.context, user.ID, newGroup.ID)
	s.Require().NoError(err)
	s.Equal(newGroup.Name, selectedGroup.Name)
	s.Equal(newAliases, selectedGroup.Aliases)
}

func (s *DBClientSuite) Test_CreateUserGroupWithDevices() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	// check group creation in non-existent household
	homelessGroup := data.GenerateGroup()
	homelessGroup.HouseholdID = "[object Object]"
	homelessGroup.ID, err = s.dbClient.CreateUserGroup(s.context, user, homelessGroup)
	s.Error(err)
	s.True(xerrors.Is(err, &model.UserHouseholdNotFoundError{}))

	// generate 3 devices, A and B has same type
	deviceA := data.GenerateDevice()
	deviceB := data.GenerateDevice()
	deviceC := data.GenerateDevice()
	deviceB.OriginalType = deviceA.OriginalType
	deviceB.Type = deviceA.Type

	deviceCType := deviceC.Type
	for deviceCType == deviceA.Type && model.MultiroomSpeakers[deviceCType] {
		deviceCType = model.DeviceType(random.Choice(model.KnownDeviceTypes))
	}
	deviceC.Type = deviceCType
	deviceC.OriginalType = deviceCType
	storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, deviceA)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	deviceA.ID = storedDevice.ID
	storedDevice, _, err = s.dbClient.StoreUserDevice(s.context, user, deviceB)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	deviceB.ID = storedDevice.ID
	storedDevice, _, err = s.dbClient.StoreUserDevice(s.context, user, deviceC)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	deviceC.ID = storedDevice.ID

	group := data.GenerateGroup()
	group.Devices = []string{deviceA.ID, deviceB.ID, deviceC.ID}

	_, err = s.dbClient.CreateUserGroup(s.context, user, group)
	s.True(xerrors.Is(err, &model.IncompatibleDeviceTypeError{}))

	group.Devices = []string{deviceA.ID, deviceB.ID}

	groupID, err := s.dbClient.CreateUserGroup(s.context, user, group)
	s.NoError(err)

	group, err = s.dbClient.SelectUserGroup(s.context, user.ID, groupID)
	s.Require().NoError(err)
	expected := []string{deviceA.ID, deviceB.ID}
	sort.Strings(group.Devices)
	sort.Strings(expected)
	s.Equal(expected, group.Devices)
	s.Equal(group.Type, deviceA.Type)
}

func (s *DBClientSuite) Test_UpdateUserGroup() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	// generate 3 devices, A and B has same type
	deviceA := data.GenerateDevice()
	deviceB := data.GenerateDevice()
	deviceC := data.GenerateDevice()
	deviceB.OriginalType = deviceA.OriginalType
	deviceB.Type = deviceA.Type

	deviceCType := deviceC.Type
	for deviceCType == deviceA.Type && model.MultiroomSpeakers[deviceCType] {
		deviceCType = model.DeviceType(random.Choice(model.KnownDeviceTypes))
	}
	deviceC.Type = deviceCType
	deviceC.OriginalType = deviceCType
	storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, deviceA)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	deviceA.ID = storedDevice.ID
	storedDevice, _, err = s.dbClient.StoreUserDevice(s.context, user, deviceB)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	deviceB.ID = storedDevice.ID
	storedDevice, _, err = s.dbClient.StoreUserDevice(s.context, user, deviceC)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	deviceC.ID = storedDevice.ID

	group := data.GenerateGroup()
	group.Devices = []string{deviceC.ID}

	group.ID, err = s.dbClient.CreateUserGroup(s.context, user, group)
	s.Require().NoError(err)

	selectedGroup, err := s.dbClient.SelectUserGroup(s.context, user.ID, group.ID)
	s.Require().NoError(err)
	expected := []string{deviceC.ID}
	s.Equal(expected, selectedGroup.Devices)
	s.Equal(deviceC.Type, selectedGroup.Type)

	differentName := "Другое имечко"
	devicesToGroup := []string{deviceA.ID, deviceB.ID}
	selectedGroup.Name = differentName
	selectedGroup.Devices = devicesToGroup
	err = s.dbClient.UpdateUserGroupNameAndDevices(s.context, user, selectedGroup)
	s.Require().NoError(err)

	selectedGroup, err = s.dbClient.SelectUserGroup(s.context, user.ID, group.ID)
	s.Require().NoError(err)
	sort.Strings(devicesToGroup)
	sort.Strings(selectedGroup.Devices)
	s.Equal(devicesToGroup, selectedGroup.Devices)
	s.Equal(deviceA.Type, selectedGroup.Type)
	s.Equal(differentName, selectedGroup.Name)
}

func (s *DBClientSuite) Test_HouseholdGroups() {
	var err error
	user := data.GenerateUser()
	err = s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	currentHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, user.ID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	group1 := data.GenerateGroup()
	group1.ID, err = s.dbClient.CreateUserGroup(s.context, user, group1)
	s.NoError(err)
	group1.Name = tools.StandardizeSpaces(group1.Name)
	group1.HouseholdID = currentHousehold.ID
	group1.Devices = []string{}
	s.checkUserGroups(user.ID, []model.Group{group1}, nil)

	household2ID, err := s.dbClient.CreateUserHousehold(s.context, user.ID, data.GenerateHousehold("Домишко"))
	s.NoError(err)
	household2, err := s.dbClient.SelectUserHousehold(s.context, user.ID, household2ID)
	s.NoError(err)

	group2 := data.GenerateGroup()
	group2.HouseholdID = household2ID
	group2.ID, err = s.dbClient.CreateUserGroup(s.context, user, group2)
	s.NoError(err)

	// check groups per household
	household1Groups, err := s.dbClient.SelectUserHouseholdGroups(s.context, user.ID, currentHousehold.ID)
	s.NoError(err)
	s.Equal(1, len(household1Groups))
	s.Equal(group1.ID, household1Groups[0].ID)

	household2Groups, err := s.dbClient.SelectUserHouseholdGroups(s.context, user.ID, household2.ID)
	s.NoError(err)
	s.Equal(1, len(household2Groups))
	s.Equal(group2.ID, household2Groups[0].ID)
}
