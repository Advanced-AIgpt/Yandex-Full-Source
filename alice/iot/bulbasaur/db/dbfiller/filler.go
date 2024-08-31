package dbfiller

import (
	"bytes"
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/log"
)

type Filler struct {
	logger log.Logger
	client *db.DBClient
}

func NewFiller(logger log.Logger, client *db.DBClient) *Filler {
	return &Filler{logger: logger, client: client}
}

func (filler *Filler) InsertUser(ctx context.Context, user *model.TestUser) (_ *model.TestUser, err error) {
	if err := filler.client.StoreUser(ctx, user.User); err != nil {
		return nil, err
	}

	for name, room := range user.Rooms {
		if room.ID == "" {
			if room.ID, err = filler.client.CreateUserRoom(ctx, user.User, room); err != nil {
				return nil, err
			}
			user.Rooms[name] = room
		} else {
			filler.logger.Infof("Skipping insertion of room %v, id is already set", room)
		}
	}

	for name, group := range user.Groups {
		if group.ID == "" {
			if group.ID, err = filler.client.CreateUserGroup(ctx, user.User, group); err != nil {
				return nil, err
			}

			if len(group.Aliases) != 0 {
				err := filler.client.UpdateUserGroupNameAndAliases(ctx, user.User, group.ID, group.Name, group.Aliases)
				if err != nil {
					return nil, err
				}
			}

			user.Groups[name] = group
		} else {
			filler.logger.Infof("Skipping insertion of group %v, id is already set", group)
		}
	}

	return user, nil
}

func (filler *Filler) InsertDevice(ctx context.Context, user *model.User, device *model.Device) (_ *model.Device, err error) {
	storedDevice, _, err := filler.client.StoreUserDevice(ctx, *user, *device)
	if err != nil {
		return nil, err
	}
	device.ID = storedDevice.ID
	if device.Room != nil {
		if err = filler.client.UpdateUserDeviceRoom(ctx, user.ID, device.ID, device.Room.ID); err != nil {
			return nil, err
		}
	}
	if err = filler.client.UpdateUserDeviceGroups(ctx, *user, device.ID, device.GroupsIDs()); err != nil {
		return nil, err
	}
	device.Created = storedDevice.Created
	device.HouseholdID = storedDevice.HouseholdID
	return device, nil
}

func (filler *Filler) InsertRoom(ctx context.Context, user *model.User, room *model.Room) (_ *model.Room, err error) {
	if room.ID, err = filler.client.CreateUserRoom(ctx, *user, *room); err != nil {
		return nil, err
	}
	if room.HouseholdID == "" {
		currentHousehold, err := filler.client.SelectCurrentHousehold(ctx, user.ID)
		if err != nil {
			return nil, err
		}
		room.HouseholdID = currentHousehold.ID
	}
	return room, nil
}

func (filler *Filler) InsertGroup(ctx context.Context, user *model.User, group *model.Group) (_ *model.Group, err error) {
	if group.ID, err = filler.client.CreateUserGroup(ctx, *user, *group); err != nil {
		return nil, err
	}
	if group.HouseholdID == "" {
		currentHousehold, err := filler.client.SelectCurrentHousehold(ctx, user.ID)
		if err != nil {
			return nil, err
		}
		group.HouseholdID = currentHousehold.ID
	}
	return group, nil
}

func (filler *Filler) InsertScenario(ctx context.Context, user *model.User, scenario *model.Scenario) (_ *model.Scenario, err error) {
	if scenario.ID, err = filler.client.CreateScenario(ctx, user.ID, *scenario); err != nil {
		return nil, err
	}
	return scenario, nil
}

func (filler *Filler) InsertNetwork(ctx context.Context, user *model.User, network *model.Network) (_ *model.Network, err error) {
	if err = filler.client.StoreUserNetwork(ctx, user.ID, *network); err != nil {
		return nil, err
	}
	return network, nil
}

func (filler *Filler) InsertHousehold(ctx context.Context, user *model.User, household *model.Household) (_ *model.Household, err error) {
	householdID, err := filler.client.CreateUserHousehold(ctx, user.ID, *household)
	if err != nil {
		return nil, err
	}
	household.ID = householdID
	return household, nil
}

type ReplaceIDs struct {
	Rooms      map[string]string
	Groups     map[string]string
	Devices    map[string]string
	Scenarios  map[string]string
	Households map[string]string
}

func (d ReplaceIDs) ApplyToData(data []byte) []byte {
	for oldID, newID := range d.Rooms {
		data = bytes.ReplaceAll(data, []byte(oldID), []byte(newID))
	}
	for oldID, newID := range d.Groups {
		data = bytes.ReplaceAll(data, []byte(oldID), []byte(newID))
	}
	for oldID, newID := range d.Devices {
		data = bytes.ReplaceAll(data, []byte(oldID), []byte(newID))
	}
	for oldID, newID := range d.Scenarios {
		data = bytes.ReplaceAll(data, []byte(oldID), []byte(newID))
	}
	for oldID, newID := range d.Households {
		data = bytes.ReplaceAll(data, []byte(oldID), []byte(newID))
	}
	return data
}

func (filler *Filler) InsertUserInfo(ctx context.Context, userInfo model.UserInfo) (*model.User, *ReplaceIDs, error) {
	user := model.NewUser("alice").User
	replaces := &ReplaceIDs{
		Rooms:      map[string]string{},
		Groups:     map[string]string{},
		Devices:    map[string]string{},
		Scenarios:  map[string]string{},
		Households: map[string]string{},
	}
	err := filler.client.StoreUser(ctx, user)
	if err != nil {
		return nil, nil, err
	}
	currentHousehold, err := filler.client.SelectCurrentHousehold(ctx, user.ID)
	if err != nil {
		return nil, nil, err
	}
	for i := range userInfo.Households {
		oldID := userInfo.Households[i].ID
		if userInfo.Households[i].ID == userInfo.CurrentHouseholdID {
			currentHouseholdID := currentHousehold.ID
			currentHousehold := userInfo.Households[i]
			currentHousehold.ID = currentHouseholdID
			err = filler.client.UpdateUserHousehold(ctx, user.ID, currentHousehold)
			if err != nil {
				return nil, nil, err
			}
			replaces.Households[oldID] = currentHouseholdID
			continue
		}
		household, err := filler.InsertHousehold(ctx, &user, &userInfo.Households[i])
		if err != nil {
			return nil, nil, err
		}
		newID := household.ID
		replaces.Households[oldID] = newID
	}
	for i := range userInfo.Rooms {
		oldID := userInfo.Rooms[i].ID
		if userInfo.Rooms[i].HouseholdID != "" {
			newHouseholdID := replaces.Households[userInfo.Rooms[i].HouseholdID]
			userInfo.Rooms[i].HouseholdID = newHouseholdID
		}
		room, err := filler.InsertRoom(ctx, &user, &userInfo.Rooms[i])
		if err != nil {
			return nil, nil, err
		}
		newID := room.ID
		replaces.Rooms[oldID] = newID
	}
	for i := range userInfo.Groups {
		oldID := userInfo.Groups[i].ID
		if userInfo.Groups[i].HouseholdID != "" {
			newHouseholdID := replaces.Households[userInfo.Groups[i].HouseholdID]
			userInfo.Groups[i].HouseholdID = newHouseholdID
		}
		group, err := filler.InsertGroup(ctx, &user, &userInfo.Groups[i])
		if err != nil {
			return nil, nil, err
		}
		newID := group.ID
		replaces.Groups[oldID] = newID
	}
	for i := range userInfo.Scenarios {
		oldID := userInfo.Scenarios[i].ID
		scenario, err := filler.InsertScenario(ctx, &user, &userInfo.Scenarios[i])
		if err != nil {
			return nil, nil, err
		}
		newID := scenario.ID
		replaces.Scenarios[oldID] = newID
	}
	for i := range userInfo.Devices {
		oldID := userInfo.Devices[i].ID
		for j, group := range userInfo.Devices[i].Groups {
			userInfo.Devices[i].Groups[j].ID = replaces.Groups[group.ID]
			if userInfo.Devices[i].Groups[j].HouseholdID != "" {
				newHouseholdID := replaces.Households[userInfo.Devices[i].Groups[j].HouseholdID]
				userInfo.Devices[i].Groups[j].HouseholdID = newHouseholdID
			}
		}
		if userInfo.Devices[i].Room != nil {
			userInfo.Devices[i].Room.ID = replaces.Rooms[userInfo.Devices[i].Room.ID]
			if userInfo.Devices[i].Room.HouseholdID != "" {
				newHouseholdID := replaces.Households[userInfo.Devices[i].Room.HouseholdID]
				userInfo.Devices[i].Room.HouseholdID = newHouseholdID
			}
		}
		if userInfo.Devices[i].HouseholdID != "" {
			newHouseholdID := replaces.Households[userInfo.Devices[i].HouseholdID]
			userInfo.Devices[i].HouseholdID = newHouseholdID
		}
		device, err := filler.InsertDevice(ctx, &user, &userInfo.Devices[i])
		if err != nil {
			return nil, nil, err
		}
		newID := device.ID
		replaces.Devices[oldID] = newID
	}
	return &user, replaces, nil
}
