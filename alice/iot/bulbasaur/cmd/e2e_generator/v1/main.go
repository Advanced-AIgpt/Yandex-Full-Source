package main

import (
	"bufio"
	"context"
	"fmt"
	"os"
	"strconv"
	"strings"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log/zap"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/alice/library/go/tools"
)

const idsPrefix string = "e2e-"

func initLogging() (logger *zap.Logger, stop func()) {
	encoderConfig := uberzap.NewDevelopmentEncoderConfig()
	encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
	encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
	encoder := zapcore.NewConsoleEncoder(encoderConfig)

	core := zapcore.NewCore(encoder, zapcore.AddSync(os.Stderr), uberzap.DebugLevel)
	stop = func() {
		_ = core.Sync()
	}
	logger = zap.NewWithCore(core, uberzap.AddStacktrace(uberzap.FatalLevel), uberzap.AddCaller())

	return
}

func main() {
	logger, stop := initLogging()
	defer stop()

	endpoint := os.Getenv("YDB_ENDPOINT")
	if len(endpoint) == 0 {
		panic("YDB_ENDPOINT env is not set")
	}

	prefix := os.Getenv("YDB_PREFIX")
	if len(prefix) == 0 {
		panic("YDB_PREFIX env is not set")
	}

	token := os.Getenv("YDB_TOKEN")
	if len(token) == 0 {
		panic("YDB_TOKEN env is not set")
	}

	_, trace := os.LookupEnv("YDB_DEBUG")

	dbcli, err := db.NewClient(context.Background(), logger, endpoint, prefix, ydb.AuthTokenCredentials{AuthToken: token}, trace)
	if err != nil {
		panic(err.Error())
	}

	reader := bufio.NewReader(os.Stdin)
	fmt.Print("Enter dataset name:")
	line, _ := reader.ReadString('\n')
	datasetName := strings.Split(line, "\n")[0]

	users := getUsersFromDataset(logger, datasetName)
	ctx := context.Background()

	logger.Infof("Getting users data from DB: %s %s", endpoint, prefix)
	users = populateOriginalData(ctx, logger, dbcli, users)

	c := cli.AskForConfirmation("Starting to copy users. Continue?", logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}
	copyUsersData(ctx, logger, dbcli, users)

	logger.Info("Done")

}

type mUser struct {
	originalUID       uint64
	newUID            uint64
	newLogin          string
	newHuid           uint64
	newHousehold      *model.Household
	originalDevices   []model.Device
	originalGroups    []model.Group
	originalRooms     []model.Room
	originalScenarios []model.Scenario
}

func createmUser(originalUID uint64, newUID uint64, newLogin string) mUser {
	return mUser{
		originalUID: originalUID,
		newUID:      newUID,
		newLogin:    newLogin,
		newHuid:     tools.Huidify(newUID),
	}
}

func (m *mUser) toUser() model.User {
	return model.User{
		ID:    m.newUID,
		Login: m.newLogin,
	}
}

func getUsersFromDataset(logger *zap.Logger, datasetName string) []mUser {
	users := make([]mUser, 0)

	dataset, exists := e2eData[datasetName]
	if !exists {
		logger.Warnf("Failed to get dataset with name '%s'. Reason: not found.", datasetName)
		os.Exit(1)
	}

	for _, row := range strings.Split(dataset, "\n") { // internally, it advances token based on sperator
		splitted := strings.Split(row, ";")
		originalUID, err := strconv.ParseUint(splitted[0], 10, 64)
		if err != nil {
			logger.Warnf("Failed to get original uid from row: '%s'. Ignoring row.", row)
			continue
		}
		newUID, err := strconv.ParseUint(splitted[2], 10, 64)
		if err != nil {
			logger.Warnf("Failed to get new uid from row: '%s'. Ignoring row.", row)
			continue
		}
		newLogin := splitted[3]
		if len(newLogin) == 0 {
			logger.Warnf("Failed to get new login from row: '%s'. Ignoring row.", row)
			continue
		}

		users = append(users, createmUser(originalUID, newUID, newLogin))
	}

	logger.Infof("Found %d users:", len(users))
	for _, user := range users {
		logger.Infof("new UID: %d\t new login: %s\t original UID: %d", user.newUID, user.newLogin, user.originalUID)
	}
	logger.Info("-------------------")
	c := cli.AskForConfirmation("Continue?", logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	return users
}

func populateOriginalData(ctx context.Context, logger *zap.Logger, db *db.DBClient, users []mUser) []mUser {
	survivedUsers := make([]mUser, 0)
	for _, user := range users {
		// === Getting original data ===
		if _, err := db.SelectUser(ctx, user.originalUID); err != nil {
			logger.Errorf("Failed to get user %d from db. Reason: %s", user.originalUID, err.Error())
			continue
		}
		rooms, err := db.SelectUserRooms(ctx, user.originalUID)
		if err != nil {
			logger.Errorf("Failed to get user %d rooms. Reason: %s", user.originalUID, err.Error())
			continue
		}
		groups, err := db.SelectUserGroups(ctx, user.originalUID)
		if err != nil {
			logger.Errorf("Failed to get user %d groups. Reason: %s", user.originalUID, err.Error())
			continue
		}
		devices, err := db.SelectUserDevices(ctx, user.originalUID)
		if err != nil {
			logger.Errorf("Failed to get user %d devices. Reason: %s", user.originalUID, err.Error())
			continue
		}
		scenarios, err := db.SelectUserScenarios(ctx, user.originalUID)
		if err != nil {
			logger.Errorf("Failed to get user %d scenarios. Reason: %s", user.originalUID, err.Error())
			continue
		}

		user.originalDevices = devices
		user.originalGroups = groups
		user.originalRooms = rooms
		user.originalScenarios = scenarios

		survivedUsers = append(survivedUsers, user)
	}

	logger.Infof("%d/%d users successfully selected from db.", len(survivedUsers), len(users))

	c := cli.AskForConfirmation("Continue?", logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	if len(survivedUsers) > 0 {
		logger.Infof("Copying details:")
		for _, user := range survivedUsers {
			logger.Infof("User %d will be copied with:\t %d devices\t %d rooms\t %d groups\t %d scenarios", user.originalUID, len(user.originalDevices), len(user.originalRooms), len(user.originalGroups), len(user.originalScenarios))
		}
	}

	return survivedUsers
}

func copyUsersData(ctx context.Context, logger *zap.Logger, db *db.DBClient, users []mUser) {
	for _, newUser := range users {
		// === Storing new data ===
		// -- store user
		err := db.StoreUser(ctx, newUser.toUser())
		if err != nil {
			logger.Errorf("Failed to create user %v. Reason: %s", newUser.toUser(), err.Error())
			c := cli.AskForConfirmation("Continue?", logger)
			if !c {
				logger.Info("Bye")
				os.Exit(0)
			}
			continue
		}
		// -- store rooms
		roomsMap := make(map[string]string) // map [originalRoomID]newRoomID
		for _, room := range newUser.originalRooms {
			if roomID, err := db.CreateUserRoom(ctx, newUser.toUser(), room); err != nil {
				logger.Errorf("Failed to store room with name %s for user %d. Reason: %s", room.Name, newUser.newUID, err.Error())
				c := cli.AskForConfirmation("Continue?", logger)
				if !c {
					logger.Info("Bye")
					os.Exit(0)
				}
				continue
			} else {
				roomsMap[room.ID] = roomID
			}

		}

		// -- store groups
		groupsMap := make(map[string]string) // map [originalGroupID]newGroupID
		logger.Infof("User has %d groups. Trying to create...", len(newUser.originalGroups))
		for _, group := range newUser.originalGroups {
			if groupID, err := db.CreateUserGroup(ctx, newUser.toUser(), group); err != nil {
				logger.Errorf("Failed to store group with name %s for user %d. Reason: %s", group.Name, newUser.newUID, err.Error())
				c := cli.AskForConfirmation("Continue?", logger)
				if !c {
					logger.Info("Bye")
					os.Exit(0)
				}
				continue
			} else {
				logger.Infof("Successfully created group with id %s and name %s for user %d", groupID, group.Name, newUser.newUID)
				groupsMap[group.ID] = groupID
			}
		}

		// -- store devices
		devicesMap := make(map[string]string) // map [originalDeviceID]newDeviceID
		for _, device := range newUser.originalDevices {
			device.ID = idsPrefix + device.ID
			device.ExternalID = idsPrefix + device.ExternalID
			device.SkillID = model.VIRTUAL
			if device.Room != nil {
				device.Room.ID = roomsMap[device.Room.ID]
			}
			storedDevice, _, err := db.StoreUserDevice(ctx, newUser.toUser(), device)
			if err != nil {
				logger.Errorf("Failed to store device with id %s for user %d. Reason: %s", device.ID, newUser.newUID, err.Error())
				c := cli.AskForConfirmation("Continue?", logger)
				if !c {
					logger.Info("Bye")
					os.Exit(0)
				}
				continue
			}
			devicesMap[device.ID] = storedDevice.ID

			// -- store devices groups
			if len(device.Groups) > 0 {
				deviceGroups := make([]string, 0)
				for _, group := range device.Groups {
					deviceGroups = append(deviceGroups, groupsMap[group.ID])
				}

				if err := db.UpdateUserDeviceGroups(ctx, newUser.toUser(), storedDevice.ID, deviceGroups); err != nil {
					logger.Errorf("Failed to update device %s groups for user %d. Groups: %#v. Reason: %s", device.ID, newUser.newUID, deviceGroups, err.Error())
					logger.Errorf("Device dump: %#v", device)
					c := cli.AskForConfirmation("Continue?", logger)
					if !c {
						logger.Info("Bye")
						os.Exit(0)
					}
					continue
				}
			}

		}

		// -- store scenarios
		for _, scenario := range newUser.originalScenarios {
			scenarioDevices := make([]model.ScenarioDevice, 0)
			for _, sDevice := range scenario.Devices {
				sDevice.ID = devicesMap[sDevice.ID]
				scenarioDevices = append(scenarioDevices, sDevice)
			}
			scenario.Devices = scenarioDevices
			if _, err := db.CreateScenario(ctx, newUser.newUID, scenario); err != nil {
				logger.Errorf("create scenario with name %s for user %s (uid: %d) failed. Reason: %s", scenario.Name, newUser.newLogin, newUser.newUID, err.Error())
				c := cli.AskForConfirmation("Continue?", logger)
				if !c {
					logger.Info("Bye")
					os.Exit(0)
				}
				continue
			}
		}
	}
}
