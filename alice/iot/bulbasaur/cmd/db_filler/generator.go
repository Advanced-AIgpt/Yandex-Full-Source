package main

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func generateDevices(n int) (devices []model.Device) {
	for i := 0; i < n; i++ {
		devices = append(devices, generateDevice())
	}
	return devices
}

func generateDevice() (device model.Device) {
	device = data.GenerateDevice()

	if flipCoin() {
		room := data.GenerateRoom()
		device.Room = &room
	}

	return device
}

func generateScenarios(n int, devices []model.Device) (scenarios []model.Scenario) {
	devicesWithCapabilities := make([]model.Device, 0)
	for _, device := range devices {
		if len(device.Capabilities) > 0 {
			devicesWithCapabilities = append(devicesWithCapabilities, device)
		}
	}

	for i := 0; i < n; i++ {
		count := randRange(len(devicesWithCapabilities)/5, len(devicesWithCapabilities)/2)
		devicesToUse := make([]model.Device, 0, count)
		for _, i := range pickRandomFew(count, rng(len(devicesWithCapabilities))) {
			devicesToUse = append(devicesToUse, devicesWithCapabilities[i])
		}

		scenarios = append(scenarios, data.GenerateScenario("", devicesToUse))
	}
	return scenarios
}

func generateRooms(n int) (rooms []model.Room) {
	for i := 0; i < n; i++ {
		rooms = append(rooms, data.GenerateRoom())
	}
	return rooms
}

func generateGroups(n int) (groups []model.Group) {
	for i := 0; i < n; i++ {
		groups = append(groups, data.GenerateGroup())
	}
	return groups
}

func createUser(ctx context.Context, db *db.DBClient) error {
	//generate user
	user := data.GenerateUser()
	err := db.StoreUser(ctx, user)
	if err != nil {
		return xerrors.Errorf("cannot StoreUser: %w", err)
	}
	logger.Debugf("generated User(`%d`, `%s`)", user.ID, user.Login)
	//generate devices
	devices := generateDevices(randRange(1, 50))
	for i := 0; i < len(devices); i++ {
		storedDevice, _, err := db.StoreUserDevice(ctx, user, devices[i])
		if err != nil {
			return xerrors.Errorf("cannot StoreUserDevice: %w", err)
		}
		devices[i].ID = storedDevice.ID
		logger.Debugf("\tgenerated Device(%s,\t%s)", devices[i].ID, devices[i].Type)
	}

	//generate rooms
	rooms := generateRooms(randInt(10))
	for i := 0; i < len(rooms); i++ {
		newID, err := db.CreateUserRoom(ctx, user, rooms[i])
		if err != nil {
			return xerrors.Errorf("cannot CreateUserRoom: %w", err)
		}
		rooms[i].ID = newID
		logger.Debugf("\tgenerated Room(%s)", rooms[i].ID)
	}

	//move 80% random devices to rooms
	if len(rooms) > 0 && len(devices) > 0 {
		devicesToMove := pickRandomFew(len(devices)-len(devices)/8, rng(len(devices)))
		for _, d := range devicesToMove {
			roomID := rooms[randInt(len(rooms))].ID
			if err := db.UpdateUserDeviceRoom(ctx, user.ID, devices[d].ID, roomID); err != nil {
				return xerrors.Errorf("cannot UpdateUserDeviceRoom: %w", err)
			}
			logger.Debugf("\tDevice(%s) moved to Room(%s)", devices[d].ID, roomID)
		}
	}

	//generate groups
	groups := generateGroups(randInt(10))
	for i := 0; i < len(groups); i++ {
		newID, err := db.CreateUserGroup(ctx, user, groups[i])
		if err != nil {
			return xerrors.Errorf("cannot CreateUserGroup: %w", err)
		}
		groups[i].ID = newID
		logger.Debugf("\tgenerated Group(%s)", groups[i].ID)
	}

	//move 80% random devices to groups
	if len(groups) > 0 && len(devices) > 0 {
		devicesToMove := pickRandomFew(len(devices)-len(devices)/8, rng(len(devices)))
		groupsIndexes := rng(len(groups))
		for _, d := range devicesToMove {
			var pickedGroups []int
			pickedGroups, groupsIndexes = pickRandomFewAndRemove(randInt(len(groupsIndexes)), groupsIndexes)
			if len(pickedGroups) < 1 {
				continue
			}
			var newGroups []string
			for _, g := range pickedGroups {
				newGroups = append(newGroups, groups[g].ID)
			}
			if err := db.UpdateUserDeviceGroups(ctx, user, devices[d].ID, newGroups); err != nil {
				return xerrors.Errorf("cannot UpdateUserDeviceGroups: %w", err)
			}
			logger.Debugf("\tDevice(%s) added to Groups(%s)", devices[d].ID, newGroups)
		}
	}

	//generate scenarios
	scenarios := generateScenarios(randRange(1, 50), devices)
	for i := 0; i < len(scenarios); i++ {
		newID, err := db.CreateScenario(ctx, user.ID, scenarios[i])
		if err != nil {
			return xerrors.Errorf("cannot CreateScenario: %w", err)
		}
		scenarios[i].ID = newID
		logger.Debugf("\tgenerated Scenario(%s)", scenarios[i].ID)
	}

	//delete few devices
	if len(devices) > 0 {
		deviceIndexesToDelete := pickRandomFew(randInt(len(devices)/3), rng(len(devices)))
		for _, d := range deviceIndexesToDelete {
			logger.Debugf("\tremoving Device(%s,\t%s)", devices[d].ID, devices[d].Type)
			if err := db.DeleteUserDevices(ctx, user.ID, []string{devices[d].ID}); err != nil {
				return xerrors.Errorf("cannot DeleteUserDevices: %w", err)
			}
		}
	}

	//delete few rooms
	if len(rooms) > 0 {
		roomIndexesToDelete := pickRandomFew(randInt(len(rooms)/3), rng(len(rooms)))
		for _, r := range roomIndexesToDelete {
			logger.Debugf("\tremoving Room(%s)", rooms[r].ID)
			if err := db.DeleteUserRoom(ctx, user.ID, rooms[r].ID); err != nil {
				return xerrors.Errorf("cannot DeleteUserRoom: %w", err)
			}
		}
	}

	//delete few groups
	if len(groups) > 0 {
		groupIndexesToDelete := pickRandomFew(randInt(len(groups)/3), rng(len(groups)))
		for _, g := range groupIndexesToDelete {
			logger.Debugf("\tremoving Group(%s)", groups[g].ID)
			if err := db.DeleteUserGroup(ctx, user.ID, groups[g].ID); err != nil {
				return xerrors.Errorf("cannot DeleteUserGroup: %w", err)
			}
		}
	}

	//delete few scenarios
	if len(scenarios) > 0 {
		scenarioIndexesToDelete := pickRandomFew(randInt(len(scenarios)/3), rng(len(scenarios)))
		for _, g := range scenarioIndexesToDelete {
			logger.Debugf("\tremoving Scenario(%s)", scenarios[g].ID)
			if err := db.DeleteScenario(ctx, user.ID, scenarios[g].ID); err != nil {
				return xerrors.Errorf("cannot DeleteScenario: %w", err)
			}
		}
	}

	return nil
}
