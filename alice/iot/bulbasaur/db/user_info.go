package db

import (
	"context"
	"sync"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (db *DBClient) selectUserInfo(ctx context.Context, userID uint64) (model.UserInfo, error) {
	simpleInfo, err := db.selectUserInfoSimple(ctx, userID)
	if err != nil {
		return model.UserInfo{}, err
	}

	return simpleInfo.toUserInfo()
}

type userInfoSimple struct {
	Devices                model.Devices
	Scenarios              model.Scenarios
	DeviceGroups           []deviceGroupsItem
	Households             model.Households
	CurrentHouseholdID     string
	GroupsBeforeFiltration model.Groups
	RoomsBeforeFiltration  model.Rooms
	StereopairsSimple      model.Stereopairs
	RawFavorites           rawFavorites
}

func (s userInfoSimple) toSharedUserInfo(ownerID uint64, sharingInfos model.SharingInfos) userInfoSimple {
	result := s.filterBySharingInfos(sharingInfos)
	result.markAsShared(sharingInfos)
	result.dropScenarios()
	result.dropFavorites()
	return result
}

func (s userInfoSimple) filterBySharingInfos(sharingInfos model.SharingInfos) userInfoSimple {
	var result userInfoSimple
	householdSharingMap := sharingInfos.ToHouseholdMap()
	for _, household := range s.Households {
		if _, ok := householdSharingMap[household.ID]; !ok {
			continue
		}
		result.Households = append(result.Households, household)
	}
	for _, room := range s.RoomsBeforeFiltration {
		if _, ok := householdSharingMap[room.HouseholdID]; !ok {
			continue
		}
		result.RoomsBeforeFiltration = append(result.RoomsBeforeFiltration, room)
	}
	for _, group := range s.GroupsBeforeFiltration {
		if _, ok := householdSharingMap[group.HouseholdID]; !ok {
			continue
		}
		result.GroupsBeforeFiltration = append(result.GroupsBeforeFiltration, group)
	}
	for _, device := range s.Devices {
		if _, ok := householdSharingMap[device.HouseholdID]; !ok {
			continue
		}
		result.Devices = append(result.Devices, device)
	}
	result.DeviceGroups = append(result.DeviceGroups, s.DeviceGroups...)
	result.StereopairsSimple = append(result.StereopairsSimple, s.StereopairsSimple...)
	result.RawFavorites = append(result.RawFavorites, s.RawFavorites...)
	result.Scenarios = append(result.Scenarios, s.Scenarios...)
	result.CurrentHouseholdID = s.CurrentHouseholdID
	return result
}

func (s *userInfoSimple) markAsShared(sharingInfos model.SharingInfos) {
	s.Devices.SetSharingInfo(sharingInfos)
	s.GroupsBeforeFiltration.SetSharingInfo(sharingInfos)
	s.RoomsBeforeFiltration.SetSharingInfo(sharingInfos)
	s.Households.SetSharingInfoAndName(sharingInfos)
}

func (s *userInfoSimple) dropScenarios() {
	s.Scenarios = make(model.Scenarios, 0)
}

func (s *userInfoSimple) dropFavorites() {
	s.RawFavorites = rawFavorites{}
}

func (s userInfoSimple) toUserInfo() (model.UserInfo, error) {
	var result model.UserInfo

	roomsMap := make(map[string]*model.Room)
	for i := range s.RoomsBeforeFiltration {
		roomsMap[s.RoomsBeforeFiltration[i].ID] = &s.RoomsBeforeFiltration[i]
	}
	groupsMap := make(map[string]*model.Group)
	for i := range s.GroupsBeforeFiltration {
		groupsMap[s.GroupsBeforeFiltration[i].ID] = &s.GroupsBeforeFiltration[i]
	}

	devicesMap := make(map[string]*model.Device)
	roomHasDevices := make(map[string]bool)

	for i, device := range s.Devices {
		devicesMap[device.ID] = &s.Devices[i]

		if device.Room != nil && device.Room.ID != "" && roomsMap[device.Room.ID] != nil {
			roomHasDevices[device.Room.ID] = true
			roomsMap[device.Room.ID].Devices = append(roomsMap[device.Room.ID].Devices, device.ID)
			devicesMap[device.ID].Room = roomsMap[device.Room.ID]
		} else {
			devicesMap[device.ID].Room = nil
		}

		devicesMap[device.ID].Groups = make([]model.Group, 0)
	}

	// filter rooms
	rooms := make([]model.Room, 0, len(s.RoomsBeforeFiltration))
	for roomID, room := range roomsMap {
		if roomHasDevices[roomID] {
			rooms = append(rooms, *room)
		}
	}
	result.Rooms = rooms

	// find groups to add, fill group device ids
	groupsToAdd := make(map[string]bool, len(s.DeviceGroups))
	for _, item := range s.DeviceGroups {
		if groupsMap[item.GroupID] != nil && devicesMap[item.DeviceID] != nil {
			groupsToAdd[item.GroupID] = true
			groupsMap[item.GroupID].Devices = append(groupsMap[item.GroupID].Devices, item.DeviceID)
		}
	}

	devicesMapByID := s.Devices.ToMap()

	// prepare favorites
	favoriteRelations := s.RawFavorites.toFavoriteRelationMaps()
	for devicePropertyKey := range favoriteRelations.FavoriteDevicePropertyKeys {
		device, found := devicesMapByID[devicePropertyKey.DeviceID]
		if !found {
			delete(favoriteRelations.FavoriteDevicePropertyKeys, devicePropertyKey)
			continue
		}
		if _, found := device.Properties.GetByKey(devicePropertyKey.PropertyKey); !found {
			delete(favoriteRelations.FavoriteDevicePropertyKeys, devicePropertyKey)
			continue
		}
	}
	result.FavoriteRelations = favoriteRelations
	// set all favorite markers
	for i := range s.Devices {
		s.Devices[i].Favorite = favoriteRelations.FavoriteDeviceIDs[s.Devices[i].ID]
	}
	for i := range s.Scenarios {
		s.Scenarios[i].Favorite = favoriteRelations.FavoriteScenarioIDs[s.Scenarios[i].ID]
	}
	for i := range s.GroupsBeforeFiltration {
		s.GroupsBeforeFiltration[i].Favorite = favoriteRelations.FavoriteGroupIDs[s.GroupsBeforeFiltration[i].ID]
	}

	// fill groups and device group ids
	groups := make([]model.Group, 0, len(s.DeviceGroups))
	for groupID := range groupsToAdd {
		group := *groupsMap[groupID]
		groups = append(groups, group)
		for _, deviceID := range group.Devices {
			devicesMap[deviceID].Groups = append(devicesMap[deviceID].Groups, group)
		}
	}
	result.Groups = groups

	// stereopairs
	if err := fillStereopairsFromDevices(s.StereopairsSimple, devicesMapByID); err != nil {
		return model.UserInfo{}, err
	}
	result.Stereopairs = s.StereopairsSimple

	// no filtering required
	result.Devices = s.Devices
	result.Scenarios = s.Scenarios
	result.Households = s.Households
	result.CurrentHouseholdID = s.CurrentHouseholdID

	return result, nil
}

func (db *DBClient) selectUserInfoSimple(ctx context.Context, userID uint64) (userInfoSimple, error) {
	// transaction doesn't allow parallel requests
	// both on the server and sdk (failed by race condition)
	if db.HasTransaction(ctx) {
		return userInfoSimple{}, xerrors.New("SelectUserInfo doesn't work in transaction")
	}
	ctx = db.ContextWithTransactionType(ctx, db.StaleReadTransactionType)

	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	var primitives userInfoSimple
	var selectGroup goroutines.Group

	selectGroup.Go(func() (err error) {
		user, err := db.getUser(ctx, userID)
		if err != nil {
			if xerrors.Is(err, new(model.UnknownUserError)) {
				cancel() // other data for this user does not exist
			}
			return err
		}
		primitives.CurrentHouseholdID = user.CurrentHouseholdID
		return nil
	})

	selectGroup.Go(func() (err error) {
		primitives.Devices, err = db.selectUserDevicesSimple(ctx, DeviceQueryCriteria{UserID: userID})
		return err
	})

	selectGroup.Go(func() (err error) {
		primitives.DeviceGroups, err = db.selectDeviceGroups(ctx, DeviceGroupsQueryCriteria{UserID: userID})
		return err
	})

	selectGroup.Go(func() (err error) {
		primitives.GroupsBeforeFiltration, err = db.selectUserGroupsSimple(ctx, GroupQueryCriteria{UserID: userID})
		return err
	})

	selectGroup.Go(func() (err error) {
		primitives.RoomsBeforeFiltration, err = db.selectUserRoomsSimple(ctx, RoomQueryCriteria{UserID: userID})
		return err
	})

	selectGroup.Go(func() (err error) {
		primitives.Scenarios, err = db.selectUserScenariosSimple(ctx, ScenarioQueryCriteria{UserID: userID})
		return err
	})

	selectGroup.Go(func() (err error) {
		primitives.Households, err = db.selectUserHouseholds(ctx, HouseholdQueryCriteria{UserID: userID})
		return err
	})

	selectGroup.Go(func() (err error) {
		primitives.StereopairsSimple, err = db.SelectStereopairsSimple(ctx, userID)
		return err
	})

	selectGroup.Go(func() (err error) {
		primitives.RawFavorites, err = db.selectRawFavorites(ctx, userID)
		return err
	})
	if err := selectGroup.Wait(); err != nil {
		return userInfoSimple{}, xerrors.Errorf("failed to select userinfo primitives: %w", err)
	}
	return primitives, nil
}

func (db *DBClient) SelectUserInfo(ctx context.Context, userID uint64) (model.UserInfo, error) {
	// transaction doesn't allow parallel requests
	// both on the server and sdk (failed by race condition)
	if db.HasTransaction(ctx) {
		return model.UserInfo{}, xerrors.New("SelectUserInfo doesn't work in transaction")
	}
	var selectGroup goroutines.Group
	var userInfo model.UserInfo

	selectGroup.Go(func() (err error) {
		userInfoSimple, err := db.selectUserInfoSimple(ctx, userID)
		if err != nil {
			return xerrors.Errorf("failed to select user info simple: %w", err)
		}
		res, err := userInfoSimple.toUserInfo()
		if err != nil {
			return xerrors.Errorf("failed to convert primitives to user info: %w", err)
		}
		userInfo = res
		return nil
	})

	sharedUserInfo := model.NewEmptyUserInfo()
	selectGroup.Go(func() (err error) {
		guestSharingInfos, err := db.SelectGuestSharingInfos(ctx, userID)
		if err != nil {
			return xerrors.Errorf("failed to select guest sharing infos: %w", err)
		}
		if len(guestSharingInfos) == 0 {
			return nil
		}

		var sharedUserInfoGroup goroutines.Group
		var mutex sync.RWMutex
		sharedUserInfoMerger := func(ownerID uint64, sharingInfos model.SharingInfos) func() error {
			return func() error {
				ownerInfoSimple, err := db.selectUserInfoSimple(ctx, ownerID)
				if err != nil {
					return xerrors.Errorf("failed to select user info simple: %w", err)
				}
				ownerUserInfo, err := ownerInfoSimple.toSharedUserInfo(ownerID, sharingInfos).toUserInfo()
				if err != nil {
					return xerrors.Errorf("failed to convert user info simple: %w", err)
				}
				mutex.Lock()
				sharedUserInfo.Merge(ownerUserInfo)
				mutex.Unlock()
				return nil
			}
		}
		ownerMap := guestSharingInfos.ToOwnerMap()
		for ownerID, sharingInfos := range ownerMap {
			sharedUserInfoGroup.Go(sharedUserInfoMerger(ownerID, sharingInfos))
		}
		if err := sharedUserInfoGroup.Wait(); err != nil {
			ctxlog.Warnf(ctx, db.Logger, "failed to collect owners user info: %v", err)
		}
		return nil
	})

	if err := selectGroup.Wait(); err != nil {
		return model.UserInfo{}, xerrors.Errorf("failed to select userinfo: %w", err)
	}
	userInfo.Merge(sharedUserInfo)
	return userInfo, nil
}
