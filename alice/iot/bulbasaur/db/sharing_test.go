package db

import (
	"fmt"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
	"github.com/gofrs/uuid"
)

func (s *DBClientSuite) TestSharingInfo() {
	owner := data.GenerateUser()

	// check empty sharing
	sharingInfos, err := s.dbClient.SelectGuestSharingInfos(s.context, owner.ID)
	s.NoError(err)
	s.Len(sharingInfos, 0)

	// create two households
	household1 := data.GenerateHousehold("Дача")
	household1.ID, err = s.dbClient.CreateUserHousehold(s.context, owner.ID, household1)
	s.NoError(err)

	household2 := data.GenerateHousehold("Работа")
	household2.ID, err = s.dbClient.CreateUserHousehold(s.context, owner.ID, household2)
	s.NoError(err)

	firstSharingInfo := model.SharingInfo{
		OwnerID:       owner.ID,
		HouseholdID:   household1.ID,
		HouseholdName: "Чужой дом 1",
	}

	// share household
	guest := data.GenerateUser()
	err = s.dbClient.StoreSharedHousehold(s.context, guest.ID, firstSharingInfo)
	s.NoError(err)

	// check one record
	s.checkSharingInfos(guest.ID, model.SharingInfos{firstSharingInfo})

	secondSharingInfo := model.SharingInfo{
		OwnerID:       owner.ID,
		HouseholdID:   household2.ID,
		HouseholdName: "Чужой дом 2",
	}

	// share another one
	err = s.dbClient.StoreSharedHousehold(s.context, guest.ID, secondSharingInfo)
	s.NoError(err)

	s.checkSharingInfos(guest.ID, model.SharingInfos{
		firstSharingInfo, secondSharingInfo,
	})

	// delete record on first one
	err = s.dbClient.DeleteSharedHousehold(s.context, owner.ID, guest.ID, household1.ID)
	s.NoError(err)
	s.checkSharingInfos(guest.ID, model.SharingInfos{secondSharingInfo})
}

func (s *DBClientSuite) TestUserInfoWithShared() {
	s.Run("basic one shared household", func() {
		owner := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, owner)
		s.NoError(err)

		guest := data.GenerateUser()
		err = s.dbClient.StoreUser(s.context, guest)
		s.NoError(err)

		guestHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, guest.ID)
		s.NoError(err)

		ownerHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, owner.ID)
		s.NoError(err)

		sharingInfo := model.SharingInfo{
			OwnerID:       owner.ID,
			HouseholdID:   ownerHousehold.ID,
			HouseholdName: "Дача",
		}
		ownerHousehold.SharingInfo = &sharingInfo

		devices := make(model.Devices, 3)
		for i := range devices {
			devices[i] = data.GenerateDevice()
			storedDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, owner, devices[i])
			if err != nil {
				s.dataPreparationFailed(err)
				return
			}
			s.Equal(model.StoreResultNew, storeResult)
			devices[i] = formatDeviceSelectUserInfo(s.context, storedDevice)
		}
		devices.SetSharingInfo(model.SharingInfos{sharingInfo})
		// make them favorite for owner
		// explicitly not setting favorite flag as true for guest
		err = s.dbClient.StoreFavoriteDevices(s.context, owner, devices)
		s.NoError(err)

		err = s.dbClient.StoreSharedHousehold(s.context, guest.ID, sharingInfo)
		s.NoError(err)

		ownerHousehold.Name = sharingInfo.HouseholdName

		s.checkUserInfo(guest.ID, model.UserInfo{
			Devices: devices,
			Households: model.Households{
				guestHousehold,
				ownerHousehold,
			},
			CurrentHouseholdID: guestHousehold.ID,
		}, nil)
	})
	s.Run("three users shared households to fourth one", func() {
		guest := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, guest)
		s.NoError(err)

		guestHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, guest.ID)
		s.NoError(err)

		households := model.Households{
			guestHousehold,
		}

		for i := 0; i < 3; i++ {
			owner := data.GenerateUser()
			err = s.dbClient.StoreUser(s.context, owner)
			s.NoError(err)
			ownerHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, owner.ID)
			s.NoError(err)
			householdName := fmt.Sprintf("Дом %d", i)
			sharingInfo := model.SharingInfo{
				OwnerID:       owner.ID,
				HouseholdID:   ownerHousehold.ID,
				HouseholdName: householdName,
			}
			ownerHousehold.SharingInfo = &sharingInfo
			ownerHousehold.Name = householdName
			err = s.dbClient.StoreSharedHousehold(s.context, guest.ID, sharingInfo)
			s.NoError(err)
			households = append(households, ownerHousehold)
		}

		s.checkUserInfo(guest.ID, model.UserInfo{
			Households:         households,
			CurrentHouseholdID: guestHousehold.ID,
		}, nil)
	})
}

func (s *DBClientSuite) TestStoreSharedHousehold() {
	s.Run("devices", func() {
		owner := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, owner)
		s.NoError(err)

		guest := data.GenerateUser()
		err = s.dbClient.StoreUser(s.context, guest)
		s.NoError(err)

		ownerHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, owner.ID)
		s.NoError(err)

		sharingInfo := model.SharingInfo{
			OwnerID:       owner.ID,
			HouseholdID:   ownerHousehold.ID,
			HouseholdName: "Дача",
		}

		// generate owner device
		device := data.GenerateDevice()
		ownerDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, owner, device)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		s.Equal(model.StoreResultNew, storeResult)

		// generate owner room
		ownerRoom := data.GenerateRoom()
		ownerRoom.Devices = []string{ownerDevice.ID}
		ownerRoom.ID, err = s.dbClient.CreateUserRoom(s.context, owner, ownerRoom)
		s.NoError(err)
		ownerRoom.SharingInfo = &sharingInfo

		// generate guest device
		device = data.GenerateDevice()
		guestDevice, storeResult, err := s.dbClient.StoreUserDevice(s.context, guest, device)
		if err != nil {
			s.dataPreparationFailed(err)
			return
		}
		s.Equal(model.StoreResultNew, storeResult)
		guestDevice = formatDeviceStoreUserDevice(s.context, guestDevice)

		err = s.dbClient.StoreSharedHousehold(s.context, guest.ID, sharingInfo)
		s.NoError(err)

		ownerDevice, err = s.dbClient.SelectUserDevice(s.context, guest.ID, ownerDevice.ID)
		s.NoError(err)
		s.NotNil(ownerDevice.Room)
		s.Equal(sharingInfo, *ownerDevice.Room.SharingInfo)
		ownerDevice = formatDeviceStoreUserDevice(s.context, ownerDevice)

		// make device favorite for owner
		// explicitly not setting favorite flag as true for guest
		err = s.dbClient.StoreFavoriteDevices(s.context, owner, model.Devices{device})
		s.NoError(err)
		ownerDevice.Favorite = false

		s.checkUserDevices(guest.ID, model.Devices{ownerDevice, guestDevice})
	})
	s.Run("groups", func() {
		owner := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, owner)
		s.NoError(err)

		guest := data.GenerateUser()
		err = s.dbClient.StoreUser(s.context, guest)
		s.NoError(err)

		guestHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, guest.ID)
		s.NoError(err)

		ownerHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, owner.ID)
		s.NoError(err)

		sharingInfo := model.SharingInfo{
			OwnerID:       owner.ID,
			HouseholdID:   ownerHousehold.ID,
			HouseholdName: "Дача",
		}

		// generate owner group
		ownerGroup := data.GenerateGroup()
		ownerGroup.ID, err = s.dbClient.CreateUserGroup(s.context, owner, ownerGroup)
		s.NoError(err)
		ownerGroup.SharingInfo = &sharingInfo
		ownerGroup.Devices = []string{}
		ownerGroup.HouseholdID = ownerHousehold.ID

		// generate guest group
		guestGroup := data.GenerateGroup()
		guestGroup.ID, err = s.dbClient.CreateUserGroup(s.context, guest, guestGroup)
		s.NoError(err)
		guestGroup.HouseholdID = guestHousehold.ID
		guestGroup.Devices = []string{}

		// make group favorite for owner
		// explicitly not setting favorite flag as true for guest
		err = s.dbClient.StoreFavoriteGroups(s.context, owner, model.Groups{ownerGroup})
		s.NoError(err)

		err = s.dbClient.StoreSharedHousehold(s.context, guest.ID, sharingInfo)
		s.NoError(err)

		s.checkUserGroups(guest.ID, model.Groups{ownerGroup, guestGroup}, nil)
	})
	s.Run("rooms", func() {
		owner := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, owner)
		s.NoError(err)

		guest := data.GenerateUser()
		err = s.dbClient.StoreUser(s.context, guest)
		s.NoError(err)

		guestHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, guest.ID)
		s.NoError(err)

		ownerHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, owner.ID)
		s.NoError(err)

		sharingInfo := model.SharingInfo{
			OwnerID:       owner.ID,
			HouseholdID:   ownerHousehold.ID,
			HouseholdName: "Дача",
		}

		// generate owner room
		ownerRoom := data.GenerateRoom()
		ownerRoom.ID, err = s.dbClient.CreateUserRoom(s.context, owner, ownerRoom)
		s.NoError(err)
		ownerRoom.SharingInfo = &sharingInfo
		ownerRoom.HouseholdID = ownerHousehold.ID
		ownerRoom.Devices = []string{}

		// generate guest room
		guestRoom := data.GenerateRoom()
		guestRoom.ID, err = s.dbClient.CreateUserRoom(s.context, guest, guestRoom)
		s.NoError(err)
		guestRoom.HouseholdID = guestHousehold.ID
		guestRoom.Devices = []string{}

		err = s.dbClient.StoreSharedHousehold(s.context, guest.ID, sharingInfo)
		s.NoError(err)

		s.checkUserRooms(guest.ID, model.Rooms{ownerRoom, guestRoom}, nil)
	})
	s.Run("households", func() {
		owner := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, owner)
		s.NoError(err)

		guest := data.GenerateUser()
		err = s.dbClient.StoreUser(s.context, guest)
		s.NoError(err)

		guestHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, guest.ID)
		s.NoError(err)

		ownerHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, owner.ID)
		s.NoError(err)

		sharingInfo := model.SharingInfo{
			OwnerID:       owner.ID,
			HouseholdID:   ownerHousehold.ID,
			HouseholdName: "Дача",
		}
		ownerHousehold.SharingInfo = &sharingInfo
		ownerHousehold.Name = sharingInfo.HouseholdName

		err = s.dbClient.StoreSharedHousehold(s.context, guest.ID, sharingInfo)
		s.NoError(err)

		s.checkHouseholds(guest.ID, model.Households{guestHousehold, ownerHousehold})
	})
	s.Run("current household", func() {
		owner := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, owner)
		s.NoError(err)

		guest := data.GenerateUser()
		err = s.dbClient.StoreUser(s.context, guest)
		s.NoError(err)

		ownerHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, owner.ID)
		s.NoError(err)
		sharingInfo := model.SharingInfo{
			OwnerID:       owner.ID,
			HouseholdID:   ownerHousehold.ID,
			HouseholdName: "Дача",
		}
		ownerHousehold.SharingInfo = &sharingInfo
		ownerHousehold.Name = "Дача"

		err = s.dbClient.StoreSharedHousehold(s.context, guest.ID, sharingInfo)
		s.NoError(err)

		err = s.dbClient.SetCurrentHouseholdForUser(s.context, guest.ID, ownerHousehold.ID)
		s.NoError(err)

		currentGuestHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, guest.ID)
		s.NoError(err)
		s.Equal(ownerHousehold, currentGuestHousehold)
	})
	s.Run("name validation for shared household", func() {
		owner := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, owner)
		s.NoError(err)

		guest := data.GenerateUser()
		err = s.dbClient.StoreUser(s.context, guest)
		s.NoError(err)

		guestHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, guest.ID)
		s.NoError(err)

		ownerHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, owner.ID)
		s.NoError(err)
		sharingInfo := model.SharingInfo{
			OwnerID:       owner.ID,
			HouseholdID:   ownerHousehold.ID,
			HouseholdName: guestHousehold.Name,
		}

		err = s.dbClient.StoreSharedHousehold(s.context, guest.ID, sharingInfo)
		s.Error(err)
		s.ErrorIs(err, &model.NameIsAlreadyTakenError{})
		sharingInfo.HouseholdName = "Дача"
		ownerHousehold.SharingInfo = &sharingInfo

		err = s.dbClient.StoreSharedHousehold(s.context, guest.ID, sharingInfo)
		s.NoError(err)
		// now try to rename guest current household to shared household name to see error
		guestHouseholdCurrentName := guestHousehold.Name
		guestHousehold.Name = "Дача"
		err = s.dbClient.UpdateUserHousehold(s.context, guest.ID, guestHousehold)
		s.Error(err)
		s.ErrorIs(err, &model.NameIsAlreadyTakenError{})

		// rename shared household for guest
		err = s.dbClient.RenameSharedHousehold(s.context, guest.ID, ownerHousehold.ID, guestHouseholdCurrentName)
		s.Error(err)
		s.ErrorIs(err, &model.NameIsAlreadyTakenError{})
		err = s.dbClient.RenameSharedHousehold(s.context, guest.ID, ownerHousehold.ID, "Офис")
		s.NoError(err)
		// check shared household new name
		sharingInfos, err := s.dbClient.SelectGuestSharingInfos(s.context, guest.ID)
		s.NoError(err)
		s.Len(sharingInfos, 1)
		s.Equal("Офис", sharingInfos[0].HouseholdName)
	})
}

func (s *DBClientSuite) TestSelectHouseholdResidents() {
	s.Run("guest and owner querying got same result", func() {
		owner := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, owner)
		s.NoError(err)

		guest := data.GenerateUser()
		err = s.dbClient.StoreUser(s.context, guest)
		s.NoError(err)

		ownerHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, owner.ID)
		s.NoError(err)
		sharingInfo := model.SharingInfo{
			OwnerID:       owner.ID,
			HouseholdID:   ownerHousehold.ID,
			HouseholdName: "Дача",
		}
		ownerHousehold.SharingInfo = &sharingInfo

		err = s.dbClient.StoreSharedHousehold(s.context, guest.ID, sharingInfo)
		s.NoError(err)

		// owner selecting
		residents, err := s.dbClient.SelectHouseholdResidents(s.context, owner.ID, ownerHousehold)
		s.NoError(err)

		expected := model.HouseholdResidents{
			{
				ID:   guest.ID,
				Role: model.GuestHouseholdRole,
			},
			{
				ID:   owner.ID,
				Role: model.OwnerHouseholdRole,
			},
		}
		s.Equal(expected, residents)

		// guest selecting
		residents, err = s.dbClient.SelectHouseholdResidents(s.context, guest.ID, ownerHousehold)
		s.NoError(err)

		expected = model.HouseholdResidents{
			{
				ID:   guest.ID,
				Role: model.GuestHouseholdRole,
			},
			{
				ID:   owner.ID,
				Role: model.OwnerHouseholdRole,
			},
		}
		s.Equal(expected, residents)
	})

	s.Run("pending invitations guests", func() {
		owner := data.GenerateUser()
		err := s.dbClient.StoreUser(s.context, owner)
		s.NoError(err)

		guest := data.GenerateUser()
		err = s.dbClient.StoreUser(s.context, guest)
		s.NoError(err)

		ownerHousehold, err := s.dbClient.SelectCurrentHousehold(s.context, owner.ID)
		s.NoError(err)
		sharingInfo := model.SharingInfo{
			OwnerID:       owner.ID,
			HouseholdID:   ownerHousehold.ID,
			HouseholdName: "Дача",
		}
		ownerHousehold.SharingInfo = &sharingInfo

		invitation := model.HouseholdInvitation{
			ID:          uuid.Must(uuid.NewV4()).String(),
			SenderID:    owner.ID,
			GuestID:     guest.ID,
			HouseholdID: ownerHousehold.ID,
		}
		err = s.dbClient.StoreHouseholdInvitation(s.context, invitation)
		s.NoError(err)

		// owner selecting
		residents, err := s.dbClient.SelectHouseholdResidents(s.context, owner.ID, ownerHousehold)
		s.NoError(err)

		expected := model.HouseholdResidents{
			{
				ID:   guest.ID,
				Role: model.PendingInvitationHouseholdRole,
			},
			{
				ID:   owner.ID,
				Role: model.OwnerHouseholdRole,
			},
		}
		s.Equal(expected, residents)

		// guest selecting
		residents, err = s.dbClient.SelectHouseholdResidents(s.context, guest.ID, ownerHousehold)
		s.NoError(err)

		expected = model.HouseholdResidents{
			{
				ID:   guest.ID,
				Role: model.PendingInvitationHouseholdRole,
			},
			{
				ID:   owner.ID,
				Role: model.OwnerHouseholdRole,
			},
		}
		s.Equal(expected, residents)
	})
}

func (s *DBClientSuite) TestHouseholdInvitations() {
	owner := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, owner)
	s.NoError(err)

	// create household
	household1 := data.GenerateHousehold("Дача")
	household1.ID, err = s.dbClient.CreateUserHousehold(s.context, owner.ID, household1)
	s.NoError(err)

	// check empty invitations
	invitations, err := s.dbClient.SelectHouseholdInvitationsBySender(s.context, owner.ID)
	s.NoError(err)
	s.Len(invitations, 0)

	invitations, err = s.dbClient.SelectHouseholdInvitationsByGuest(s.context, owner.ID)
	s.NoError(err)
	s.Len(invitations, 0)

	guest := data.GenerateUser()
	err = s.dbClient.StoreUser(s.context, guest)
	s.NoError(err)

	// store invitation
	invitation := model.HouseholdInvitation{
		ID:          uuid.Must(uuid.NewV4()).String(),
		SenderID:    owner.ID,
		HouseholdID: household1.ID,
		GuestID:     guest.ID,
	}
	err = s.dbClient.StoreHouseholdInvitation(s.context, invitation)
	s.NoError(err)
	// check
	invitations, err = s.dbClient.SelectHouseholdInvitationsBySender(s.context, owner.ID)
	s.NoError(err)
	s.Equal(model.HouseholdInvitations{invitation}, invitations)

	invitations, err = s.dbClient.SelectHouseholdInvitationsByGuest(s.context, guest.ID)
	s.NoError(err)
	s.Equal(model.HouseholdInvitations{invitation}, invitations)

	selectedInvitation, err := s.dbClient.SelectHouseholdInvitationByID(s.context, invitation.ID)
	s.NoError(err)
	s.Equal(invitation, selectedInvitation)

	// delete invitation
	err = s.dbClient.DeleteHouseholdInvitation(s.context, invitation)
	s.NoError(err)

	// check empty invitations
	invitations, err = s.dbClient.SelectHouseholdInvitationsBySender(s.context, owner.ID)
	s.NoError(err)
	s.Len(invitations, 0)

	invitations, err = s.dbClient.SelectHouseholdInvitationsByGuest(s.context, owner.ID)
	s.NoError(err)
	s.Len(invitations, 0)
}

func (s *DBClientSuite) TestHouseholdSharingLinks() {
	owner := data.GenerateUser()
	err := s.dbClient.StoreUser(s.context, owner)
	s.NoError(err)

	// create household
	household1 := data.GenerateHousehold("Дача")
	household1.ID, err = s.dbClient.CreateUserHousehold(s.context, owner.ID, household1)
	s.NoError(err)

	// store sharing link
	link := model.HouseholdSharingLink{
		SenderID:    owner.ID,
		HouseholdID: household1.ID,
		ID:          uuid.Must(uuid.NewV4()).String(),
		ExpireAt:    s.dbClient.CurrentTimestamp().Add(24 * time.Hour),
	}
	err = s.dbClient.StoreHouseholdSharingLink(s.context, link)
	s.NoError(err)

	actual, err := s.dbClient.SelectHouseholdSharingLinkByID(s.context, link.ID)
	s.NoError(err)
	s.Equal(link, actual)

	actual, err = s.dbClient.SelectHouseholdSharingLink(s.context, owner.ID, household1.ID)
	s.NoError(err)
	s.Equal(link, actual)

	err = s.dbClient.DeleteHouseholdSharingLinks(s.context, owner.ID, household1.ID)
	s.NoError(err)

	_, err = s.dbClient.SelectHouseholdSharingLink(s.context, owner.ID, household1.ID)
	s.Error(err)
	s.True(xerrors.Is(err, &model.SharingLinkDoesNotExistError{}))
}
