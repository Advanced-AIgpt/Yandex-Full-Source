package db

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *DBClientSuite) Test_RoomNameNormalization() {
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
	_, err = s.dbClient.CreateUserRoom(s.context, user, model.Room{Name: "  сом рум 123 "})
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	_, err = s.dbClient.CreateUserRoom(s.context, user, model.Room{Name: "сом рум 123"})
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	_, err = s.dbClient.CreateUserRoom(s.context, user, model.Room{Name: "сом рум 123    "})
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	_, err = s.dbClient.CreateUserRoom(s.context, user, model.Room{Name: "    сом рум 123    "})
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	// 2
	_, err = s.dbClient.CreateUserRoom(s.context, user, model.Room{Name: ""})
	s.True(xerrors.Is(err, &model.NameEmptyError{}))

	_, err = s.dbClient.CreateUserRoom(s.context, user, model.Room{Name: "   "})
	s.True(xerrors.Is(err, &model.NameEmptyError{}))

	// 3
	id, err = s.dbClient.CreateUserRoom(s.context, user, model.Room{Name: "анозер рум"})
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	err = s.dbClient.UpdateUserRoomName(s.context, user, id, "анозер рум")
	s.NoError(err)

	err = s.dbClient.UpdateUserRoomName(s.context, user, id, "   анозер рум   ")
	s.NoError(err)

	err = s.dbClient.UpdateUserRoomName(s.context, user, id, "сомрум 123")
	s.NoError(err)

	// 4
	err = s.dbClient.UpdateUserRoomName(s.context, user, id, "сом рум 123")
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	err = s.dbClient.UpdateUserRoomName(s.context, user, id, "сом рум 123      ")
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	err = s.dbClient.UpdateUserRoomName(s.context, user, id, "    сом рум 123      ")
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	// 5
	err = s.dbClient.UpdateUserRoomName(s.context, user, id, "")
	s.True(xerrors.Is(err, &model.NameEmptyError{}))

	err = s.dbClient.UpdateUserRoomName(s.context, user, id, "     ")
	s.True(xerrors.Is(err, &model.NameEmptyError{}))
}

func (s *DBClientSuite) Test_Room() {
	var (
		actualRoom model.Room
		err        error
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

	// 1. Should proper save and read rooms
	// 1.1
	_, err = s.dbClient.SelectUserRoom(s.context, data.GenerateUser().ID, "room-id")
	s.True(xerrors.Is(err, &model.RoomNotFoundError{}))

	s.checkUserRooms(data.GenerateUser().ID, nil, nil)

	// 1.2
	_, err = s.dbClient.SelectUserRoom(s.context, user.ID, "unknown-room-id")
	s.True(xerrors.Is(err, &model.RoomNotFoundError{}))

	s.checkUserRooms(user.ID, nil, nil)

	// 1.3
	room1 := data.GenerateRoom()
	room1.ID, err = s.dbClient.CreateUserRoom(s.context, user, room1)
	s.NoError(err)
	room1.Name = tools.StandardizeSpaces(room1.Name)
	room1.HouseholdID = userCurrentHousehold.ID
	room1.Devices = []string{}

	s.checkUserRooms(user.ID, []model.Room{room1}, nil)

	// 1.4
	_, err = s.dbClient.CreateUserRoom(s.context, user, room1)
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	s.checkUserRooms(user.ID, []model.Room{room1}, nil)

	// 1.5
	room2 := data.GenerateRoom()
	room2.ID, err = s.dbClient.CreateUserRoom(s.context, user, room2)
	s.NoError(err)
	room2.Name = tools.StandardizeSpaces(room2.Name)
	room2.HouseholdID = userCurrentHousehold.ID
	room2.Devices = []string{}

	s.checkUserRooms(user.ID, []model.Room{room1, room2}, nil)

	// 2. Should proper delete rooms
	// 2.1
	err = s.dbClient.DeleteUserRoom(s.context, user.ID, room2.ID)
	s.NoError(err)

	_, err = s.dbClient.SelectUserRoom(s.context, user.ID, room2.ID)
	s.True(xerrors.Is(err, &model.RoomNotFoundError{}))

	s.checkUserRooms(user.ID, []model.Room{room1}, nil)

	// 2.2
	err = s.dbClient.DeleteUserRoom(s.context, user.ID, room2.ID)
	s.NoError(err)

	s.checkUserRooms(user.ID, []model.Room{room1}, nil)

	// 2.3
	err = s.dbClient.DeleteUserRoom(s.context, user.ID, "unknown-room-id")
	s.NoError(err)

	s.checkUserRooms(user.ID, []model.Room{room1}, nil)

	// 3. Should proper rename rooms
	// 3.1
	room1.Name = tools.StandardizeSpaces(data.GenerateRoom().Name)

	err = s.dbClient.UpdateUserRoomName(s.context, user, room1.ID, room1.Name)
	s.NoError(err)

	s.checkUserRooms(user.ID, []model.Room{room1}, nil)

	// 3.2 UpdateUserRoomName should not fail if name did not changed
	err = s.dbClient.UpdateUserRoomName(s.context, user, room1.ID, room1.Name)
	s.NoError(err)

	s.checkUserRooms(user.ID, []model.Room{room1}, nil)

	// 3.3
	err = s.dbClient.UpdateUserRoomName(s.context, user, room2.ID, room1.Name)
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	s.checkUserRooms(user.ID, []model.Room{room1}, nil)

	// 3.4
	room3 := data.GenerateRoom()
	room3.ID, err = s.dbClient.CreateUserRoom(s.context, user, room3)
	s.NoError(err)
	room3.Name = tools.StandardizeSpaces(room3.Name)
	room3.HouseholdID = userCurrentHousehold.ID
	room3.Devices = []string{}

	err = s.dbClient.UpdateUserRoomName(s.context, user, room1.ID, room3.Name)
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	s.checkUserRooms(user.ID, []model.Room{room1, room3}, nil)

	// 4. Should proper control grants
	actualRoom, err = s.dbClient.SelectUserRoom(s.context, evilUser.ID, room1.ID)
	s.True(xerrors.Is(err, &model.RoomNotFoundError{}))
	s.Equal(model.Room{}, actualRoom)

	err = s.dbClient.UpdateUserRoomName(s.context, evilUser, room1.ID, room3.Name)
	s.True(xerrors.Is(err, &model.RoomNotFoundError{}))

	err = s.dbClient.DeleteUserRoom(s.context, evilUser.ID, room1.ID)
	s.NoError(err)

	err = s.dbClient.DeleteUserRoom(s.context, evilUser.ID, room1.ID)
	s.NoError(err)

	actualRoom, err = s.dbClient.SelectUserRoom(s.context, evilUser.ID, room1.ID)
	s.True(xerrors.Is(err, &model.RoomNotFoundError{}))
	s.Equal(model.Room{}, actualRoom)

	s.checkUserRooms(user.ID, []model.Room{room1, room3}, nil)
	s.checkUserRooms(evilUser.ID, nil, nil)

	// corner: evilUser with rooms !== evilUser w/o rooms
	evilRoom := data.GenerateRoom()
	evilRoom.ID, err = s.dbClient.CreateUserRoom(s.context, evilUser, evilRoom)
	s.NoError(err)
	evilRoom.Name = tools.StandardizeSpaces(evilRoom.Name)
	evilRoom.HouseholdID = evilCurrentHousehold.ID
	evilRoom.Devices = []string{}

	actualRoom, err = s.dbClient.SelectUserRoom(s.context, evilUser.ID, room1.ID)
	s.True(xerrors.Is(err, &model.RoomNotFoundError{}))
	s.Equal(model.Room{}, actualRoom)

	err = s.dbClient.UpdateUserRoomName(s.context, evilUser, room1.ID, room3.Name)
	s.True(xerrors.Is(err, &model.RoomNotFoundError{}))

	err = s.dbClient.DeleteUserRoom(s.context, evilUser.ID, room1.ID)
	s.NoError(err)

	err = s.dbClient.DeleteUserRoom(s.context, evilUser.ID, room1.ID)
	s.NoError(err)

	actualRoom, err = s.dbClient.SelectUserRoom(s.context, evilUser.ID, room1.ID)
	s.True(xerrors.Is(err, &model.RoomNotFoundError{}))
	s.Equal(model.Room{}, actualRoom)

	s.checkUserRooms(user.ID, []model.Room{room1, room3}, nil)
	s.checkUserRooms(evilUser.ID, []model.Room{evilRoom}, nil)
}

func (s *DBClientSuite) Test_RoomsAndDevices() {
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

	id, err = s.dbClient.CreateUserRoom(s.context, user, data.GenerateRoom())
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	room1, err := s.dbClient.SelectUserRoom(s.context, user.ID, id)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	room1.Devices = []string{}
	room1.HouseholdID = currentHousehold.ID

	id, err = s.dbClient.CreateUserRoom(s.context, user, data.GenerateRoom())
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	room2, err := s.dbClient.SelectUserRoom(s.context, user.ID, id)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	room2.Devices = []string{}
	room2.HouseholdID = currentHousehold.ID

	storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, data.GenerateDevice())
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	s.Equal(model.StoreResultNew, storeResult)
	device1, err := s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	device1.HouseholdID = currentHousehold.ID
	device1.Room = nil

	storedDevice, storeResult, err = s.dbClient.StoreUserDevice(s.context, user, data.GenerateDevice())
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	s.Equal(model.StoreResultNew, storeResult)

	device2, err := s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	device2.HouseholdID = currentHousehold.ID
	device2.Room = nil

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

	storedDevice, storeResult, err = s.dbClient.StoreUserDevice(s.context, evilUser, data.GenerateDevice())
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	s.Equal(model.StoreResultNew, storeResult)

	evilDevice, err := s.dbClient.SelectUserDevice(s.context, evilUser.ID, storedDevice.ID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	s.Equal(model.StoreResultNew, storeResult)
	evilDevice.HouseholdID = evilCurrentHousehold.ID
	evilDevice.Room = nil

	// 0. Pre-check
	// 0.1
	s.checkUserRooms(user.ID, []model.Room{room1, room2}, nil)
	s.checkUserDevices(user.ID, []model.Device{device1, device2})

	// 0.2
	s.checkUserRooms(evilUser.ID, nil, nil)
	s.checkUserDevices(evilUser.ID, []model.Device{evilDevice})

	// 1. Should proper add device to room
	// 1.1
	err = s.dbClient.UpdateUserDeviceRoom(s.context, user.ID, device1.ID, room1.ID)
	s.NoError(err)
	room1.Devices = []string{device1.ID}
	device1.Room = formatRoomSelectUserDevice(room1)
	device1.HouseholdID = currentHousehold.ID

	s.checkUserRooms(user.ID, []model.Room{room1, room2}, []model.Room{room1})
	s.checkUserDevices(user.ID, []model.Device{device1, device2})

	// 1.2
	err = s.dbClient.UpdateUserDeviceRoom(s.context, user.ID, device2.ID, room1.ID)
	s.NoError(err)
	room1.Devices = []string{device1.ID, device2.ID}
	device2.Room = formatRoomSelectUserDevice(room1)

	s.checkUserRooms(user.ID, []model.Room{room1, room2}, []model.Room{room1})
	s.checkUserDevices(user.ID, []model.Device{device1, device2})

	// 1.2 user w/o rooms
	err = s.dbClient.UpdateUserDeviceRoom(s.context, evilUser.ID, evilDevice.ID, "not-exist")
	s.True(xerrors.Is(err, &model.RoomNotFoundError{}))

	s.checkUserRooms(evilUser.ID, nil, nil)
	s.checkUserDevices(evilUser.ID, []model.Device{evilDevice})

	// 1.3 room does not exist
	err = s.dbClient.UpdateUserDeviceRoom(s.context, user.ID, device1.ID, "not-exist")
	s.True(xerrors.Is(err, &model.RoomNotFoundError{}))

	s.checkUserRooms(user.ID, []model.Room{room1, room2}, []model.Room{room1})
	s.checkUserDevices(user.ID, []model.Device{device1, device2})

	// 1.4 unknown device
	err = s.dbClient.UpdateUserDeviceRoom(s.context, user.ID, "not-exist", room1.ID)
	s.True(xerrors.Is(err, &model.DeviceNotFoundError{}))

	s.checkUserRooms(user.ID, []model.Room{room1, room2}, []model.Room{room1})
	s.checkUserDevices(user.ID, []model.Device{device1, device2})

	// 2. Should proper change device room
	err = s.dbClient.UpdateUserDeviceRoom(s.context, user.ID, device1.ID, room2.ID)
	s.NoError(err)
	room1.Devices = []string{device2.ID}
	room2.Devices = []string{device1.ID}
	device1.Room = formatRoomSelectUserDevice(room2)

	s.checkUserRooms(user.ID, []model.Room{room1, room2}, []model.Room{room1, room2})
	s.checkUserDevices(user.ID, []model.Device{device1, device2})

	// 3. Should proper delete rooms
	err = s.dbClient.DeleteUserRoom(s.context, user.ID, room2.ID)
	s.NoError(err)
	device1.Room = nil

	_, err = s.dbClient.SelectUserRoom(s.context, user.ID, room2.ID)
	s.True(xerrors.Is(err, &model.RoomNotFoundError{}))
	s.checkUserRooms(user.ID, []model.Room{room1}, []model.Room{room1})
	s.checkUserDevices(user.ID, []model.Device{device1, device2})

	// 4. Should proper delete device
	err = s.dbClient.DeleteUserDevice(s.context, user.ID, device2.ID)
	s.NoError(err)
	room1.Devices = []string{}

	s.checkUserRooms(user.ID, []model.Room{room1}, nil)
	s.checkUserDevices(user.ID, []model.Device{device1})

	// 5. Should proper control grants
	// 5.1
	err = s.dbClient.UpdateUserDeviceRoom(s.context, evilUser.ID, evilDevice.ID, room1.ID)
	s.True(xerrors.Is(err, &model.RoomNotFoundError{}))

	s.checkUserRooms(user.ID, []model.Room{room1}, nil)
	s.checkUserDevices(user.ID, []model.Device{device1})

	s.checkUserRooms(evilUser.ID, nil, nil)
	s.checkUserDevices(evilUser.ID, []model.Device{evilDevice})

	// corner: evilUser with rooms !== evilUser w/o rooms
	id, err = s.dbClient.CreateUserRoom(s.context, evilUser, data.GenerateRoom())
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	evilRoom, err := s.dbClient.SelectUserRoom(s.context, evilUser.ID, id)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	evilRoom.Devices = []string{}

	// 5.2
	err = s.dbClient.UpdateUserDeviceRoom(s.context, evilUser.ID, evilDevice.ID, room1.ID)
	s.True(xerrors.Is(err, &model.RoomNotFoundError{}))

	s.checkUserRooms(user.ID, []model.Room{room1}, nil)
	s.checkUserDevices(user.ID, []model.Device{device1})

	s.checkUserRooms(evilUser.ID, []model.Room{evilRoom}, nil)
	s.checkUserDevices(evilUser.ID, []model.Device{evilDevice})

	// 5.3
	err = s.dbClient.UpdateUserDeviceRoom(s.context, evilUser.ID, device1.ID, evilRoom.ID)
	s.True(xerrors.Is(err, &model.DeviceNotFoundError{}))

	s.checkUserRooms(user.ID, []model.Room{room1}, nil)
	s.checkUserDevices(user.ID, []model.Device{device1})

	s.checkUserRooms(evilUser.ID, []model.Room{evilRoom}, nil)
	s.checkUserDevices(evilUser.ID, []model.Device{evilDevice})
}

func (s *DBClientSuite) Test_RoomAutoCreation() {
	var (
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

	// 0. Pre-check
	s.checkUserRooms(user.ID, nil, nil)
	s.checkUserDevices(user.ID, nil)

	// 1. Add device without room
	deviceWORoom, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, data.GenerateDevice())
	s.NoError(err)
	s.Equal(model.StoreResultNew, storeResult)
	deviceWORoom = formatDeviceStoreUserDevice(s.context, deviceWORoom)

	s.checkUserRooms(user.ID, nil, nil)
	s.checkUserDevices(user.ID, []model.Device{deviceWORoom})

	// 2. Add device with new room
	deviceWithRoom1 := data.GenerateDevice()
	deviceWithRoom1.Room = &model.Room{Name: "new   room"}
	deviceWithRoom1.HouseholdID = currentHousehold.ID
	deviceWithRoom1, _, err = s.dbClient.StoreUserDevice(s.context, user, deviceWithRoom1)
	s.NoError(err)
	deviceWithRoom1 = formatDeviceStoreUserDevice(s.context, deviceWithRoom1)

	if deviceWithRoom1.Room == nil {
		s.Equal(model.Room{Name: "new room"}, deviceWithRoom1.Room)
		s.FailNow("test case failed")
		return
	}

	s.Equal("new room", deviceWithRoom1.Room.Name)
	s.NotZero(deviceWithRoom1.Room.ID)

	room := *deviceWithRoom1.Room
	room.Devices = []string{deviceWithRoom1.ID}
	room.HouseholdID = currentHousehold.ID
	deviceWithRoom1.Room = formatRoomSelectUserDevice(room)

	s.checkUserRooms(user.ID, []model.Room{room}, []model.Room{room})
	s.checkUserDevices(user.ID, []model.Device{deviceWORoom, deviceWithRoom1})

	// 3. Add device with known room
	deviceWithRoom2 := data.GenerateDevice()
	deviceWithRoom2.Room = &model.Room{Name: "    new room    "}
	deviceWithRoom2, _, err = s.dbClient.StoreUserDevice(s.context, user, deviceWithRoom2)
	s.NoError(err)
	deviceWithRoom2, err = s.dbClient.SelectUserDevice(s.context, user.ID, deviceWithRoom2.ID)
	s.NoError(err)

	room.Devices = []string{deviceWithRoom1.ID, deviceWithRoom2.ID}

	s.checkUserRooms(user.ID, []model.Room{room}, []model.Room{room})
	s.checkUserDevices(user.ID, []model.Device{deviceWORoom, deviceWithRoom1, deviceWithRoom2})
}

func (s *DBClientSuite) Test_CreateUpdateUserRoomWithDevices() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	// check room creation in non-existent household
	homelessRoom := data.GenerateRoom()
	homelessRoom.HouseholdID = "[object Object]"
	homelessRoom.ID, err = s.dbClient.CreateUserRoom(s.context, user, homelessRoom)
	s.Error(err)
	s.True(xerrors.Is(err, &model.UserHouseholdNotFoundError{}))

	// 1. Add device without room in household 1
	storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, data.GenerateDevice())
	s.NoError(err)
	s.Equal(model.StoreResultNew, storeResult)
	device1, err := s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
	s.NoError(err)

	// 2. create household 2
	householdID, err := s.dbClient.CreateUserHousehold(s.context, user.ID, data.GenerateHousehold("Другой дом"))
	s.NoError(err)
	household2, err := s.dbClient.SelectUserHousehold(s.context, user.ID, householdID)
	s.NoError(err)

	// 3. add device 1 to group in household 1
	groupInHousehold1 := data.GenerateGroup()
	groupInHousehold1.Devices = []string{device1.ID}
	groupID, err := s.dbClient.CreateUserGroup(s.context, user, groupInHousehold1)
	s.NoError(err)
	groupInHousehold1, err = s.dbClient.SelectUserGroup(s.context, user.ID, groupID)
	s.NoError(err)
	s.Equal(1, len(groupInHousehold1.Devices))
	s.Equal(device1.ID, groupInHousehold1.Devices[0])

	// 4. Add device without room in household 1
	device2 := data.GenerateDevice()
	device2.HouseholdID = household2.ID
	storedDevice, storeResult, err = s.dbClient.StoreUserDevice(s.context, user, device2)
	s.NoError(err)
	s.Equal(model.StoreResultNew, storeResult)
	device2, err = s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
	s.NoError(err)

	// 5. Add device 2 to group in household 2
	groupInHousehold2 := data.GenerateGroup()
	groupInHousehold2.Devices = []string{device2.ID}
	groupInHousehold2.HouseholdID = household2.ID
	groupID, err = s.dbClient.CreateUserGroup(s.context, user, groupInHousehold2)
	s.NoError(err)
	groupInHousehold2, err = s.dbClient.SelectUserGroup(s.context, user.ID, groupID)
	s.NoError(err)
	s.Equal(1, len(groupInHousehold2.Devices))
	s.Equal(device2.ID, groupInHousehold2.Devices[0])

	// 6. create room in household 2 and add device 1 and 2 to household 2
	// device from household 1 should lose all groups
	// device from household 2 should remain its groups as its already in household 2
	roomInHousehold2 := data.GenerateRoom()
	roomInHousehold2.HouseholdID = household2.ID
	roomInHousehold2.Devices = []string{device1.ID, device2.ID}
	roomID, err := s.dbClient.CreateUserRoom(s.context, user, roomInHousehold2)
	s.NoError(err)
	roomInHousehold2, err = s.dbClient.SelectUserRoom(s.context, user.ID, roomID)
	s.NoError(err)

	// check device1
	device1, err = s.dbClient.SelectUserDevice(s.context, user.ID, device1.ID)
	s.NoError(err)
	s.Equal(0, len(device1.Groups))
	s.Equal(roomID, device1.RoomID())

	// check device2
	device2, err = s.dbClient.SelectUserDevice(s.context, user.ID, device2.ID)
	s.NoError(err)
	s.Equal(1, len(device2.Groups))
	s.Equal(groupInHousehold2.ID, device2.Groups[0].ID)
	s.Equal(roomID, device2.RoomID())

	// 7. Create device3
	storedDevice, storeResult, err = s.dbClient.StoreUserDevice(s.context, user, data.GenerateDevice())
	s.NoError(err)
	s.Equal(model.StoreResultNew, storeResult)
	device3, err := s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
	s.NoError(err)

	// 8. Update room: now it contains only device 3
	roomInHousehold2.Devices = []string{device3.ID}
	err = s.dbClient.UpdateUserRoomNameAndDevices(s.context, user, roomInHousehold2)
	s.NoError(err)

	// check device1
	device1, err = s.dbClient.SelectUserDevice(s.context, user.ID, device1.ID)
	s.NoError(err)
	s.Nil(device1.Room)

	// check device2
	device2, err = s.dbClient.SelectUserDevice(s.context, user.ID, device2.ID)
	s.NoError(err)
	s.Nil(device2.Room)

	// check device3
	device3, err = s.dbClient.SelectUserDevice(s.context, user.ID, device3.ID)
	s.NoError(err)
	s.Equal(roomID, device3.RoomID())
}

func (s *DBClientSuite) Test_UpdateDeviceRoomDeleteGroups() {
	user := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, user)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	// 1. Add device without room in household 1
	storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, user, data.GenerateDevice())
	s.NoError(err)
	s.Equal(model.StoreResultNew, storeResult)
	device, err := s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
	s.NoError(err)

	// 2. create household 2
	householdID, err := s.dbClient.CreateUserHousehold(s.context, user.ID, data.GenerateHousehold("Другой дом"))
	s.NoError(err)
	household2, err := s.dbClient.SelectUserHousehold(s.context, user.ID, householdID)
	s.NoError(err)

	// 3. add device to group in household 1
	groupInHousehold1 := data.GenerateGroup()
	groupInHousehold1.Devices = []string{device.ID}
	groupID, err := s.dbClient.CreateUserGroup(s.context, user, groupInHousehold1)
	s.NoError(err)
	groupInHousehold1, err = s.dbClient.SelectUserGroup(s.context, user.ID, groupID)
	s.NoError(err)
	s.Equal(1, len(groupInHousehold1.Devices))
	s.Equal(device.ID, groupInHousehold1.Devices[0])

	// 6. create room in household 2
	roomInHousehold2 := data.GenerateRoom()
	roomInHousehold2.HouseholdID = household2.ID
	roomInHousehold2.Devices = []string{}
	roomID, err := s.dbClient.CreateUserRoom(s.context, user, roomInHousehold2)
	s.NoError(err)
	roomInHousehold2, err = s.dbClient.SelectUserRoom(s.context, user.ID, roomID)
	s.NoError(err)

	// 7. update device room to room in household 2
	err = s.dbClient.UpdateUserDeviceRoom(s.context, user.ID, device.ID, roomInHousehold2.ID)
	s.NoError(err)

	// check device groups, household and room
	device, err = s.dbClient.SelectUserDevice(s.context, user.ID, device.ID)
	s.NoError(err)
	s.Equal(roomInHousehold2.ID, device.RoomID())
	s.Equal(0, len(device.Groups))
	s.Equal(household2.ID, device.HouseholdID)
}

func (s *DBClientSuite) Test_HouseholdRooms() {
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
	room1 := data.GenerateRoom()
	room1.ID, err = s.dbClient.CreateUserRoom(s.context, user, room1)
	s.NoError(err)
	room1.Name = tools.StandardizeSpaces(room1.Name)
	room1.HouseholdID = currentHousehold.ID
	room1.Devices = []string{}
	s.checkUserRooms(user.ID, []model.Room{room1}, nil)

	household2ID, err := s.dbClient.CreateUserHousehold(s.context, user.ID, data.GenerateHousehold("Домишко"))
	s.NoError(err)
	household2, err := s.dbClient.SelectUserHousehold(s.context, user.ID, household2ID)
	s.NoError(err)

	room2 := data.GenerateRoom()
	room2.HouseholdID = household2ID
	room2.ID, err = s.dbClient.CreateUserRoom(s.context, user, room2)
	s.NoError(err)

	// check rooms per household
	household1Rooms, err := s.dbClient.SelectUserHouseholdRooms(s.context, user.ID, currentHousehold.ID)
	s.NoError(err)
	s.Equal(1, len(household1Rooms))
	s.Equal(room1.ID, household1Rooms[0].ID)

	household2Rooms, err := s.dbClient.SelectUserHouseholdRooms(s.context, user.ID, household2.ID)
	s.NoError(err)
	s.Equal(1, len(household2Rooms))
	s.Equal(room2.ID, household2Rooms[0].ID)
}
