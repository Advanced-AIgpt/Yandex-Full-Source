package xtestdb

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func (f *DB) InsertUsers(users ...*model.TestUser) {
	ctx := f.ctx
	for _, user := range users {
		var err error

		if err = f.client.StoreUser(ctx, user.User); err != nil {
			f.t.Fatalf("filler: failed to store user %s", user.Login)
		}

		for name, room := range user.Rooms {
			if room.ID == "" {
				room.ID, err = f.client.CreateUserRoom(ctx, user.User, room)
				if err != nil {
					f.t.Fatalf("filler: failed to create user %s room %s", user.Login, room.Name)
				}
				user.Rooms[name] = room
			}
		}

		for name, group := range user.Groups {
			if group.ID == "" {
				if group.ID, err = f.client.CreateUserGroup(ctx, user.User, group); err != nil {
					f.t.Fatalf("filler: failed to create user %s group %s", user.Login, group.Name)
				}

				if len(group.Aliases) != 0 {
					err := f.client.UpdateUserGroupNameAndAliases(ctx, user.User, group.ID, group.Name, group.Aliases)
					if err != nil {
						f.t.Fatalf("filler: failed to update user %s group %s name and aliases", user.Login, group.Name)
					}
				}

				user.Groups[name] = group
			}
		}
	}
}

func (f *DB) InsertExternalUsers(externalID string, skillID string, users ...model.User) {
	ctx := f.ctx
	for _, user := range users {
		if err := f.client.StoreExternalUser(ctx, externalID, skillID, user); err != nil {
			f.t.Fatalf("filler: failed to store external user %s", user.Login)
		}
	}
}

func (f *DB) InsertDevices(testUser *model.TestUser, devices ...*model.Device) {
	user := testUser.User
	ctx := f.ctx
	for _, device := range devices {
		storedDevice, _, err := f.client.StoreUserDevice(ctx, user, *device)
		if err != nil {
			f.t.Fatalf("filler: failed to store user %s device", user.Login)
			return
		}
		device.ID = storedDevice.ID
		if device.Room != nil {
			if err = f.client.UpdateUserDeviceRoom(ctx, user.ID, device.ID, device.Room.ID); err != nil {
				f.t.Fatalf("filler: failed to store user %s device", user.Login)
				return
			}
		}
		if err = f.client.UpdateUserDeviceGroups(ctx, user, device.ID, device.GroupsIDs()); err != nil {
			f.t.Fatalf("filler: failed to store user %s device", user.Login)
			return
		}
		device.Created = storedDevice.Created
		device.HouseholdID = storedDevice.HouseholdID
	}
}

func (f *DB) InsertStereopairs(testUser *model.TestUser, stereopairs ...*model.Stereopair) {
	user := testUser.User
	ctx := f.ctx
	for _, stereopair := range stereopairs {
		if err := f.client.StoreStereopair(ctx, user.ID, *stereopair); err != nil {
			f.t.Fatalf("filler: failed to store user %s stereopair", user.Login)
		}
	}
}

func (f *DB) InsertScenarios(testUser *model.TestUser, scenarios ...*model.Scenario) {
	user := testUser.User
	ctx := f.ctx
	for _, scenario := range scenarios {
		scenarioID, err := f.client.CreateScenario(ctx, user.ID, *scenario)
		if err != nil {
			f.t.Fatalf("filler: failed to store user %s scenario", user.Login)
			return
		}
		scenario.ID = scenarioID
	}
}
