package main

import (
	"context"
	"encoding/csv"
	"fmt"
	"io"
	"os"
	"strings"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/uniproxy"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
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

	core := zapcore.NewCore(encoder, zapcore.AddSync(os.Stdout), uberzap.WarnLevel)
	stop = func() {
		_ = core.Sync()
	}
	logger = zap.NewWithCore(core, uberzap.AddStacktrace(uberzap.FatalLevel), uberzap.AddCaller())

	return
}

func getUser() EUser {
	r := csv.NewReader(strings.NewReader(dataset))
	logger.Infof("skipping first line from dataset")
	_, err := r.Read()
	if err != nil {
		logger.Fatalf("unable to skip the first line from dataset: %v", err)
	}
	record, err := r.Read()
	if err != nil {
		switch {
		case xerrors.Is(err, io.EOF):
			break
		default:
			logger.Fatalf("unable to read row from dataset: %v", err)
		}
	}
	return EUser{record[0], record[1], record[2], record[3]}
}

func getConfig() Config {
	return Config{
		RequestID: "deadbeef-dead-beef-dead-beef00000075",
		UserInfoView: uniproxy.UserInfoView{
			Devices: []uniproxy.DeviceUserInfoView{
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000001",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000001",
					Name:       "Мультиварка",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000001",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.ModeCapabilityType,
							Instance:    string(model.ProgramModeInstance),
							Values:      []string{string(model.MilkPorridgeMode)},
							Retrievable: true,
							Parameters: model.ModeCapabilityParameters{
								Instance: model.ProgramModeInstance,
								Modes: []model.Mode{
									model.KnownModes[model.MilkPorridgeMode],
								},
							},
							State: model.ModeCapabilityState{
								Instance: model.ProgramModeInstance,
								Value:    model.MilkPorridgeMode,
							},
						},
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.MulticookerDeviceType,
					OriginalType: model.MulticookerDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					// wololo
					// here should be additional properties
					ID:         "e2e2e2e2-dead-beef-dead-device000002",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000002",
					Name:       "Кондиционер",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000002",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.RangeCapabilityType,
							Instance:    string(model.TemperatureRangeInstance),
							Retrievable: true,
							Parameters: model.RangeCapabilityParameters{
								Instance:     model.TemperatureRangeInstance,
								Unit:         model.UnitTemperatureCelsius,
								RandomAccess: true,
								Looped:       true,
								Range: &model.Range{
									Min:       1,
									Max:       100,
									Precision: 1,
								},
							},
							State: model.RangeCapabilityState{
								Instance: model.TemperatureRangeInstance,
								Value:    24,
							},
						},
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.AcDeviceType,
					OriginalType: model.AcDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000003",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000003",
					Name:       "Увлажнитель воздуха",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000002",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.ToggleCapabilityType,
							Instance:    string(model.IonizationToggleCapabilityInstance),
							Retrievable: true,
							Parameters: model.ToggleCapabilityParameters{
								Instance: model.IonizationToggleCapabilityInstance,
							},
							State: model.ToggleCapabilityState{
								Instance: model.IonizationToggleCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.HumidifierDeviceType,
					OriginalType: model.HumidifierDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					// wololo temperature range
					ID:         "e2e2e2e2-dead-beef-dead-device000004",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000004",
					Name:       "Обогреватель",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000002",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.RangeCapabilityType,
							Instance:    string(model.TemperatureRangeInstance),
							Retrievable: true,
							Parameters: model.RangeCapabilityParameters{
								Instance:     model.TemperatureRangeInstance,
								Unit:         model.UnitTemperatureCelsius,
								RandomAccess: true,
								Looped:       true,
								Range: &model.Range{
									Min:       1,
									Max:       100,
									Precision: 1,
								},
							},
							State: model.RangeCapabilityState{
								Instance: model.TemperatureRangeInstance,
								Value:    28,
							},
						},
						{
							Type:        model.ModeCapabilityType,
							Instance:    string(model.HeatModeInstance),
							Values:      []string{string(model.AutoMode), string(model.HighMode)},
							Retrievable: true,
							Parameters: model.ModeCapabilityParameters{
								Instance: model.HeatModeInstance,
								Modes: []model.Mode{
									model.KnownModes[model.AutoMode],
									model.KnownModes[model.AutoMode],
								},
							},
							State: model.ModeCapabilityState{
								Instance: model.HeatModeInstance,
								Value:    model.AutoMode,
							},
						},
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
					Properties: []uniproxy.PropertyUserInfoView{
						{
							Type:        model.FloatPropertyType,
							Instance:    model.PowerPropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.PowerPropertyInstance,
								Unit:     model.UnitWatt,
							},
							State: model.FloatPropertyState{
								Instance: model.PowerPropertyInstance,
								Value:    2200,
							},
						},
					},
					Type:         model.ThermostatDeviceType,
					OriginalType: model.ThermostatDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000005",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000005",
					Name:       "Торшер",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000001",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.RangeCapabilityType,
							Instance:    string(model.BrightnessRangeInstance),
							Values:      []string{},
							Retrievable: true,
							Parameters: model.RangeCapabilityParameters{
								Instance:     model.BrightnessRangeInstance,
								Unit:         model.UnitPercent,
								RandomAccess: true,
								Looped:       true,
								Range: &model.Range{
									Min:       1,
									Max:       100,
									Precision: 1,
								},
							},
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    50,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.LightDeviceType,
					OriginalType: model.LightDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000006",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000006",
					Name:       "Люстра",
					RoomID:     "",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.RangeCapabilityType,
							Instance:    string(model.BrightnessRangeInstance),
							Values:      []string{},
							Retrievable: true,
							Parameters: model.RangeCapabilityParameters{
								Instance:     model.BrightnessRangeInstance,
								Unit:         model.UnitPercent,
								RandomAccess: true,
								Looped:       true,
								Range: &model.Range{
									Min:       1,
									Max:       100,
									Precision: 1,
								},
							},
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    99,
							},
						},
					},
					Properties: []uniproxy.PropertyUserInfoView{
						{
							Type:        model.FloatPropertyType,
							Instance:    model.PowerPropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.PowerPropertyInstance,
								Unit:     model.UnitWatt,
							},
							State: model.FloatPropertyState{
								Instance: model.PowerPropertyInstance,
								Value:    120,
							},
						},
					},
					Type:         model.LightDeviceType,
					OriginalType: model.LightDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000007",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000007",
					Name:       "Лампа",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000002",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.RangeCapabilityType,
							Instance:    string(model.BrightnessRangeInstance),
							Values:      []string{},
							Retrievable: true,
							Parameters: model.RangeCapabilityParameters{
								Instance:     model.BrightnessRangeInstance,
								Unit:         model.UnitPercent,
								RandomAccess: true,
								Looped:       true,
								Range: &model.Range{
									Min:       1,
									Max:       100,
									Precision: 1,
								},
							},
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    99,
							},
						},
					},
					Properties: []uniproxy.PropertyUserInfoView{
						{
							Type:        model.FloatPropertyType,
							Instance:    model.PowerPropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.PowerPropertyInstance,
								Unit:     model.UnitWatt,
							},
							State: model.FloatPropertyState{
								Instance: model.PowerPropertyInstance,
								Value:    120,
							},
						},
					},
					Type:         model.LightDeviceType,
					OriginalType: model.LightDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
			},
			Rooms: []uniproxy.RoomUserInfoView{
				{
					ID:   "e2e2e2e2-dead-beef-dead-room00000001",
					Name: "Кухня",
				},
				{
					ID:   "e2e2e2e2-dead-beef-dead-room00000002",
					Name: "Гостиная",
				},
			},
		},
	}
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

func do(ctx context.Context, db *db.DBClient) error {
	user, config := getUser(), getConfig()

	if err := cleanStuff(ctx, db, user); err != nil {
		return xerrors.Errorf("can't clean stuff: %w", err)
	}
	if err := createStuff(ctx, db, user, config); err != nil {
		return xerrors.Errorf("can't create stuff: %w", err)
	}
	logger.Warnf("%s,%s,%s", config.RequestID, user.ID, user.Token)
	return nil
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

	dbcli, err := db.NewClient(context.Background(), logger, endpoint, prefix, ydb.AuthTokenCredentials{AuthToken: token}, trace)
	if err != nil {
		panic(err.Error())
	}

	msg := fmt.Sprintf("Do you really want to generate e2e-user for VoiceQueries ToM at `%s%s`", endpoint, prefix)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx := context.Background()
	start := time.Now()
	if err := do(ctx, dbcli); err != nil {
		logger.Fatal(err.Error())
	}
	logger.Warnf("Time elapsed: %v", time.Since(start))
}
