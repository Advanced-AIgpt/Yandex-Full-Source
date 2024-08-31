package main

import (
	"context"
	"encoding/csv"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"strings"
	"sync"
	"time"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/uniproxy"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/zap"

	"cuelang.org/go/pkg/strconv"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/yt/go/ypath"
	"a.yandex-team.ru/yt/go/yt"
	"a.yandex-team.ru/yt/go/yt/ythttp"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/library/go/cli"
)

const idsPrefix string = "e2e-"

type EUser struct {
	ID, Login, Password, Token string
}

type Config struct {
	UUID         string                `yson:"uuid"`
	StationsHash uint64                `yson:"stations_hash"`
	UserInfoView uniproxy.UserInfoView `json:"iot_config" yson:"iot_config"`
}

type rawConfig struct {
	UUID         string `yson:"uuid"`
	StationsHash uint64 `yson:"stations_hash"`
	UserInfoView string `json:"iot_config" yson:"iot_config"`
}

func (c *Config) FromRawConfig(raw rawConfig) error {
	c.UUID = raw.UUID
	c.StationsHash = raw.StationsHash
	return json.Unmarshal([]byte(raw.UserInfoView), &c.UserInfoView)
}

func generateQuasarServerActionCapability(instance model.QuasarServerActionCapabilityInstance) model.ICapability {
	quasarCap := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
	quasarCap.SetRetrievable(false)
	quasarCap.SetReportable(false)
	quasarCap.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: instance})
	return quasarCap
}

func (c Config) Key() string {
	return c.UUID + ":" + strconv.FormatUint(c.StationsHash, 10)
}

// FIXME: Due to serializing in PastTimestamp in old uniproxy.UserInfoView it loses its format and converts to string representation
// FIXME: To fix that, we can use that custom unmarshalling for that class
//func (pts *PastTimestamp) UnmarshalJSON(b []byte) error {
//	var a string
//	if err := json.Unmarshal(b, &a); err != nil {
//		return xerrors.Errorf("failed to unmarshal past timestamp: %w", err)
//	}
//	if len(a) == 0 || a == "#" {
//		*pts = 0
//		return nil
//	}
//	value, err := strconv.ParseFloat(a, 64)
//	if err != nil {
//		return xerrors.Errorf("failed to unmarshal past timestamp: %w", err)
//	}
//	*pts = PastTimestamp(value)
//	return nil
//}

var logger *zap.Logger

func initLogging() (*zap.Logger, func()) {
	encoderConfig := uberzap.NewDevelopmentEncoderConfig()
	encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
	encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
	encoder := zapcore.NewConsoleEncoder(encoderConfig)

	core := zapcore.NewCore(encoder, zapcore.AddSync(os.Stderr), uberzap.WarnLevel)
	stop := func() {
		_ = core.Sync()
	}
	return zap.NewWithCore(core, uberzap.AddStacktrace(uberzap.FatalLevel), uberzap.AddCaller()), stop
}

func streamConfigs(ctx context.Context, yt yt.Client) <-chan Config {
	configChan := make(chan Config)
	tablePath := ypath.Path("//home/iot/norchine/iot-788-json")

	go func() {
		defer close(configChan)
		tr, err := yt.ReadTable(ctx, tablePath, nil)
		if err != nil {
			logger.Fatalf("cannot read table at %s: %v", tablePath, err)
		}
		defer func() { _ = tr.Close() }()
		for tr.Next() {
			if tr.Err() != nil {
				logger.Fatalf("error while getting next row %s: %v", tablePath, tr.Err())
				continue
			}
			var rc rawConfig
			err = tr.Scan(&rc)
			if err != nil {
				logger.Warnf("error while reading row %s: %v", tablePath, err)
				continue
			}
			var c Config
			if err := c.FromRawConfig(rc); err != nil {
				logger.Warnf("error while converting rawConfig to config: %v", err)
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

		for {
			record, err := r.Read()
			if len(record) == 0 {
				break
			}
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
			return fmt.Errorf(fmt.Sprintf("Failed to store room with name `%s` for user `%d`, config `%s`", room.Name, uid, config.Key()), err)
		}
		roomsMap[room.ID] = roomID
	}

	// -- store groups
	groupsMap := make(map[string]string) // map [originalGroupID]newGroupID
	for _, group := range config.UserInfoView.Groups {
		logger.Debugf("creating group `%s` for user `%d`", group.Name, uid)
		groupID, err := db.CreateUserGroup(ctx, user, model.Group{Name: group.Name})
		if err != nil {
			return fmt.Errorf(fmt.Sprintf("Failed to store group with name `%s` for user `%d`, config `%s`", group.Name, uid, config.Key()), err)
		}
		groupsMap[group.ID] = groupID
	}

	// -- store devices
	devicesMap := make(map[string]string) // map [originalDeviceID]newDeviceID
	for _, d := range config.UserInfoView.Devices {
		if !d.Type.IsSmartSpeakerOrModule() {
			continue
		}
		device := d.ToDevice()
		if d.Type.IsSmartSpeaker() {
			if len(device.Capabilities) == 0 {
				device.Capabilities = append(device.Capabilities, generateQuasarServerActionCapability(model.TextActionCapabilityInstance))
				device.Capabilities = append(device.Capabilities, generateQuasarServerActionCapability(model.PhraseActionCapabilityInstance))
			}
		}
		device.ExternalName = device.Name         // db.StoreUserDevice doesn't expect device.Name to be provided
		device.ExternalID = idsPrefix + device.ID // cause externalID is unknown from megamind.UserInfoViewPayload
		device.SkillID = model.VIRTUAL
		if device.Room != nil {
			device.Room.ID = roomsMap[device.Room.ID]
		}
		storedDevice, _, err := db.StoreUserDevice(ctx, user, device)
		if err != nil {
			return fmt.Errorf(fmt.Sprintf("Failed to store device with id `%s` for user `%d`, config `%s`", device.ID, uid, config.Key()), err)
		}
		devicesMap[device.ID] = storedDevice.ID

		// -- store devices groups
		if len(device.Groups) > 0 {
			deviceGroups := make([]string, 0)
			for _, group := range device.Groups {
				deviceGroups = append(deviceGroups, groupsMap[group.ID])
			}

			if err := db.UpdateUserDeviceGroups(ctx, user, storedDevice.ID, deviceGroups); err != nil {
				return fmt.Errorf(fmt.Sprintf("Failed to update device `%s` groups for user `%d`, config `%s`. Groups: %#v", device.ID, uid, config.Key(), deviceGroups), err)
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
			id, exist := devicesMap[sDevice.ID]
			if !exist {
				continue
			}
			sDevice.ID = id
			scenarioDevices = append(scenarioDevices, sDevice)
		}
		scenario.Devices = scenarioDevices
		if _, err := db.CreateScenario(ctx, uid, scenario); err != nil {
			return fmt.Errorf(fmt.Sprintf("Failed to store scenario `%#v` for user `%d`, config `%s`", scenario, uid, config.Key()), err)
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
			logger.Warnf("%s,%s,%s", lc.Key(), luser.ID, luser.Token)
		}(c, user)
	}

	wg.Wait()
	return nil
}

func regenerateTokens() {
	ctx := context.Background()
	usersChannel := streamUsers(ctx, nil)
	clientSecret := os.Getenv("OAUTH_CLIENT_SECRET")
	clientID := os.Getenv("OAUTH_CLIENT_ID")
	client := resty.NewWithClient(http.DefaultClient)
	for user := range usersChannel {
		payload := fmt.Sprintf("client_id=%s&client_secret=%s&grant_type=password&username=%s&password=%s", clientID, clientSecret, user.Login, user.Password)
		request := client.R().
			SetContext(ctx).
			SetBody(payload)
		response, err := request.Execute("POST", "https://oauth.yandex.ru/token")
		if err != nil {
			logger.Fatalf("failed to get new token: %v", err)
		}
		var responseStruct struct {
			Token string `json:"access_token"`
		}
		if err := json.Unmarshal(response.Body(), &responseStruct); err != nil {
			logger.Fatalf("failed to unmarshal oauth response: %v", err)
		}
		print(fmt.Sprintf("%s,%s,%s,%s\n", user.ID, user.Login, user.Password, responseStruct.Token))
		// for not overloading
		time.Sleep(time.Second)
	}
}

func main() {
	var stop func()
	logger, stop = initLogging()
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

	dbClient, err := db.NewClient(context.Background(), logger, endpoint, prefix, ydb.AuthTokenCredentials{AuthToken: token}, trace)
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
	if err := do(ctx, dbClient, ytClient); err != nil {
		logger.Fatal(err.Error())
	}
	logger.Warnf("Time elapsed: %v", time.Since(start))
}
