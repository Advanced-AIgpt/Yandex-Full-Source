package main

import (
	"context"
	"encoding/csv"
	"fmt"
	"io"
	"os"
	"strings"
	"sync"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/uniproxy"
	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/yt/go/ypath"
	"a.yandex-team.ru/yt/go/yt"
	"a.yandex-team.ru/yt/go/yt/ythttp"
	"cuelang.org/go/pkg/strconv"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/library/go/cli"
)

const idsPrefix string = "e2e-"

type EUser struct {
	ID, Login, Password, Token string
}

type Config struct {
	RequestID    string                `yson:"request_id"`
	UserInfoView uniproxy.UserInfoView `yson:"iot_config"`
}

var logger *zap.Logger

func initLogging() (logger *zap.Logger, stop func()) {
	encoderConfig := uberzap.NewDevelopmentEncoderConfig()
	encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
	encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
	encoder := zapcore.NewConsoleEncoder(encoderConfig)

	core := zapcore.NewCore(encoder, zapcore.AddSync(os.Stderr), uberzap.WarnLevel)
	stop = func() {
		_ = core.Sync()
	}
	logger = zap.NewWithCore(core, uberzap.AddStacktrace(uberzap.FatalLevel), uberzap.AddCaller())

	return
}

func streamConfigs(ctx context.Context, yt yt.Client) <-chan Config {
	configChan := make(chan Config)
	tablePath := ypath.Path("//home/voice/aryabinina/iot_new_offline_basket/final_configs_v2")

	go func() {
		defer close(configChan)

		tr, err := yt.ReadTable(ctx, tablePath, nil)
		if err != nil {
			logger.Fatalf("cannot read table at %s: %v", tablePath, err)
		}
		defer func() { _ = tr.Close() }()

		for tr.Next() {
			var c Config
			err = tr.Scan(&c)
			if err != nil {
				logger.Fatalf("error while reading row %s: %v", tablePath, err)
			}

			//configs with wrong data, that config
			if tools.Contains(c.RequestID, []string{
				"ffffffff-ffff-ffff-b73b-00253debbd56", //contains duplicate scenario, see huid=751637949311081793 (fixed in prod)
				"ffffffff-ffff-ffff-525c-4e28a3de92f1", //contains duplicate room, see huid=5653879425764141642 (fixed in prod)
			}) {
				continue
			}

			configChan <- c
		}

		if tr.Err() != nil {
			logger.Fatalf("cannot read table at %s: %v", tablePath, tr.Err())
		}
	}()

	return configChan
}

func streamUsers(_ context.Context, done <-chan struct{}) chan EUser {
	usersChan := make(chan EUser)

	go func() {
		defer close(usersChan)
		r := csv.NewReader(strings.NewReader(dataset))
		logger.Infof("skipping first line from dataset")
		_, err := r.Read()
		if err != nil {
			logger.Fatalf("unable to skip the first line from dataset: %v", err)
		}

		for {
			record, err := r.Read()
			if err != nil {
				switch {
				case xerrors.Is(err, io.EOF):
					break
				default:
					logger.Fatalf("unable to read row from dataset: %v", err)
				}
			}
			select {
			case usersChan <- EUser{record[0], record[1], record[2], record[3]}:
			case <-done:
				return
			}
		}
	}()

	return usersChan
}

func cleanStuff(ctx context.Context, db *db.DBClient, euser EUser) error {
	uid, _ := strconv.ParseUint(euser.ID, 10, 64)
	for _, tableName := range [...]string{
		"DeviceGroups",
		"Devices",
		//"ExternalUsers",
		"Groups",
		"Rooms",
		"Scenarios",
		//"StationOwners",
		"UserSkills",
	} {
		query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DELETE FROM
				%s
			WHERE
				huid == $huid`, db.Prefix, tableName)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(uid))),
		)
		logger.Debugf("cleaning <%s> records for user: %d", tableName, uid)
		if err := db.Write(ctx, query, params); err != nil {
			return err
		}
	}

	for _, tableName := range []string{
		"Users",
	} {
		query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DELETE FROM
				%s
			WHERE
				hid == $huid`, db.Prefix, tableName)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(uid))),
		)
		logger.Debugf("cleaning <%s> records for user: %d", tableName, uid)
		if err := db.Write(ctx, query, params); err != nil {
			return err
		}
	}

	return nil
}

func createStuff(ctx context.Context, db *db.DBClient, euser EUser, config Config) error {
	uid, _ := strconv.ParseUint(euser.ID, 10, 64)

	// -- store user
	user := model.User{ID: uid, Login: euser.Login}
	err := db.StoreUser(ctx, user)
	if err != nil {
		return fmt.Errorf(fmt.Sprintf("Failed to create user %#v.", user), err)
	}

	// -- store rooms
	roomsMap := make(map[string]string) // map [originalRoomID]newRoomID
	for _, room := range config.UserInfoView.Rooms {
		logger.Debugf("creating room `%s` for user `%d`", room.Name, uid)
		roomID, err := db.CreateUserRoom(ctx, user, model.Room{Name: room.Name})
		if err != nil {
			return fmt.Errorf(fmt.Sprintf("Failed to store room with name `%s` for user `%d`, config `%s`", room.Name, uid, config.RequestID), err)
		}
		roomsMap[room.ID] = roomID
	}

	// -- store groups
	groupsMap := make(map[string]string) // map [originalGroupID]newGroupID
	for _, group := range config.UserInfoView.Groups {
		logger.Debugf("creating group `%s` for user `%d`", group.Name, uid)
		groupID, err := db.CreateUserGroup(ctx, user, model.Group{Name: group.Name})
		if err != nil {
			return fmt.Errorf(fmt.Sprintf("Failed to store group with name `%s` for user `%d`, config `%s`", group.Name, uid, config.RequestID), err)
		}
		groupsMap[group.ID] = groupID
	}

	// -- store devices
	devicesMap := make(map[string]string) // map [originalDeviceID]newDeviceID
	for _, d := range config.UserInfoView.Devices {
		device := d.ToDevice()
		device.ExternalName = device.Name         // db.StoreUserDevice doesn't expect device.Name to be provided
		device.ExternalID = idsPrefix + device.ID // cause externalID is unknown from megamind.UserInfoViewPayload
		device.SkillID = model.VIRTUAL
		if device.Room != nil {
			device.Room.ID = roomsMap[device.Room.ID]
		}
		storedDevice, _, err := db.StoreUserDevice(ctx, user, device)
		if err != nil {
			return fmt.Errorf(fmt.Sprintf("Failed to store device with id `%s` for user `%d`, config `%s`", device.ID, uid, config.RequestID), err)
		}
		devicesMap[device.ID] = storedDevice.ID

		// -- store devices groups
		if len(device.Groups) > 0 {
			deviceGroups := make([]string, 0)
			for _, group := range device.Groups {
				deviceGroups = append(deviceGroups, groupsMap[group.ID])
			}

			if err := db.UpdateUserDeviceGroups(ctx, user, storedDevice.ID, deviceGroups); err != nil {
				return fmt.Errorf(fmt.Sprintf("Failed to update device `%s` groups for user `%d`, config `%s`. Groups: %#v", device.ID, uid, config.RequestID, deviceGroups), err)
			}
		}
	}

	// -- store scenarios
	for _, s := range config.UserInfoView.Scenarios {
		scenario := s.ToScenario()

		// FIXME: back those days megamind.UserInfoViewPayload was missing these fields
		if len(scenario.Icon) == 0 {
			scenario.Icon = model.ScenarioIconAlarm
		}

		scenarioDevices := make([]model.ScenarioDevice, 0)
		for _, sDevice := range scenario.Devices {
			sDevice.ID = devicesMap[sDevice.ID]
			scenarioDevices = append(scenarioDevices, sDevice)
		}
		scenario.Devices = scenarioDevices
		if _, err := db.CreateScenario(ctx, uid, scenario); err != nil {
			return fmt.Errorf(fmt.Sprintf("Failed to store scenario `%#v` for user `%d`, config `%s`", scenario, uid, config.RequestID), err)
		}
	}

	return nil
}

func do(ctx context.Context, db *db.DBClient, yt yt.Client) error {
	done := make(chan struct{})
	usersChan := streamUsers(ctx, done)
	defer close(done)

	var wg sync.WaitGroup
	workers := make(chan int, 100)
	defer close(workers)
	for c := range streamConfigs(ctx, yt) {
		user := <-usersChan

		workers <- 1
		wg.Add(1)
		go func(lc Config, luser EUser) {
			defer func() { <-workers }()
			defer wg.Done()
			if err := cleanStuff(ctx, db, luser); err != nil {
				logger.Fatal(err.Error())
			}
			if err := createStuff(ctx, db, luser, lc); err != nil {
				logger.Fatal(err.Error())
			}
			logger.Warnf("%s,%s,%s", lc.RequestID, luser.ID, luser.Token)
		}(c, user)
	}

	wg.Wait()
	return nil
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

	ytClient, err := ythttp.NewClient(&yt.Config{
		Proxy:             "hahn",
		ReadTokenFromFile: true,
	})
	if err != nil {
		logger.Fatal(err.Error())
	}

	msg := fmt.Sprintf("Do you really want to generate e2e-users at `%s%s`", endpoint, prefix)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx := context.Background()
	start := time.Now()
	if err := do(ctx, dbcli, ytClient); err != nil {
		logger.Fatal(err.Error())
	}
	logger.Warnf("Time elapsed: %v", time.Since(start))
}
