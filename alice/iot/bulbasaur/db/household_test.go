package db

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *DBClientSuite) TestUserHouseholds() {
	user := data.GenerateUser()
	household := data.GenerateHousehold("Мой домик")
	secondHousehold := data.GenerateHousehold("Второй дом")
	thirdHousehold := data.GenerateHousehold("Третий дом")

	_, err := s.dbClient.SelectUserHousehold(s.context, user.ID, "hehe")
	s.True(xerrors.Is(err, &model.UserHouseholdNotFoundError{}))

	err = s.dbClient.UpdateUserHousehold(s.context, user.ID, household)
	s.True(xerrors.Is(err, &model.UserHouseholdNotFoundError{}))

	// create first household in db
	err = s.dbClient.StoreUser(s.context, user)
	s.NoError(err)

	currentHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, user.ID)
	s.NoError(err)

	household.ID = currentHousehold.ID
	// replace default current household with our household
	err = s.dbClient.UpdateUserHousehold(s.context, user.ID, household)
	s.NoError(err)
	currentHousehold, err = s.dbClient.SelectCurrentHousehold(s.context, user.ID)
	s.NoError(err)
	s.Equal(household, currentHousehold)

	// update household in db
	household.Name = "Мой лучший домик"
	err = s.dbClient.UpdateUserHousehold(s.context, user.ID, household)
	s.NoError(err)
	selectedHousehold, err := s.dbClient.SelectUserHousehold(s.context, user.ID, household.ID)
	s.NoError(err)
	s.Equal(household, selectedHousehold)

	// create second household in db
	secondHousehold.ID, err = s.dbClient.CreateUserHousehold(s.context, user.ID, secondHousehold)
	s.NoError(err)
	// create third household in db
	thirdHousehold.ID, err = s.dbClient.CreateUserHousehold(s.context, user.ID, thirdHousehold)
	s.NoError(err)

	// check valid select work
	selectedHousehold, err = s.dbClient.SelectUserHousehold(s.context, user.ID, household.ID)
	s.NoError(err)
	s.Equal(household, selectedHousehold)

	// check all user household selection
	s.checkHouseholds(user.ID, []model.Household{household, secondHousehold, thirdHousehold})

	// check invalid creation
	invalidHousehold := data.GenerateHousehold("мой лучший домик                ")
	invalidHousehold.ID, err = s.dbClient.CreateUserHousehold(s.context, user.ID, invalidHousehold)
	s.True(xerrors.Is(err, &model.NameIsAlreadyTakenError{}))

	// check deletion
	err = s.dbClient.DeleteUserHousehold(s.context, user.ID, thirdHousehold.ID)
	s.NoError(err)
	s.checkHouseholds(user.ID, []model.Household{household, secondHousehold})

	// delete the current active household - next household should be active now
	err = s.dbClient.DeleteUserHousehold(s.context, user.ID, household.ID)
	s.NoError(err)
	s.checkHouseholds(user.ID, []model.Household{secondHousehold})

	// check current household
	currentHousehold, err = s.dbClient.SelectCurrentHousehold(s.context, user.ID)
	s.NoError(err)
	s.Equal(secondHousehold.ID, currentHousehold.ID)

	// cannot delete the last household
	err = s.dbClient.DeleteUserHousehold(s.context, user.ID, secondHousehold.ID)
	s.True(xerrors.Is(err, &model.UserHouseholdLastDeletionError{}))
}

func (s *DBClientSuite) TestMoveUserDevicesToHousehold() {
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

	id, err = s.dbClient.CreateUserRoom(s.context, user, model.Room{Name: "Комнатка"})
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	room, err := s.dbClient.SelectUserRoom(s.context, user.ID, id)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	generatedDevice := data.GenerateDevice()
	generatedDevice.Room = &room

	storedDevice, _, err := s.dbClient.StoreUserDevice(s.context, user, generatedDevice)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	device, err := s.dbClient.SelectUserDevice(s.context, user.ID, storedDevice.ID)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	id, err = s.dbClient.CreateUserHousehold(s.context, user.ID, data.GenerateHousehold("Домишко"))
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}
	household, err := s.dbClient.SelectUserHousehold(s.context, user.ID, id)
	if err != nil {
		s.dataPreparationFailed(err)
		return
	}

	// add to group
	err = s.dbClient.UpdateUserDeviceGroups(s.context, user, device.ID, []string{group1.ID, group2.ID})
	s.NoError(err)

	group1.Devices = []string{device.ID}
	group1.Type = device.Type
	group2.Devices = []string{device.ID}
	group2.Type = device.Type

	room.Devices = []string{device.ID}

	device.Groups = []model.Group{formatGroupSelectUserDevice(group1), formatGroupSelectUserDevice(group2)}

	s.checkUserGroups(user.ID, []model.Group{group1, group2}, nil)
	s.checkUserRooms(user.ID, []model.Room{room}, nil)
	s.checkUserDevices(user.ID, []model.Device{device})

	// move to other household
	err = s.dbClient.MoveUserDevicesToHousehold(s.context, user, []string{device.ID}, household.ID)
	s.NoError(err)

	// check that it disappeared from groups and rooms
	devices, err := s.dbClient.SelectUserGroupDevices(s.context, user.ID, group1.ID)
	s.NoError(err)
	s.Equal(0, len(devices))
	devices, err = s.dbClient.SelectUserGroupDevices(s.context, user.ID, group2.ID)
	s.NoError(err)
	s.Equal(0, len(devices))
	device, err = s.dbClient.SelectUserDevice(s.context, user.ID, device.ID)
	s.NoError(err)
	s.Nil(device.Room)
}
