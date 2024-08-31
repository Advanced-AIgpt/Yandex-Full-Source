package main

import (
	"context"
	"encoding/json"
	"fmt"
	"os"

	uzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var logger *zap.Logger

type MigrationDBClient struct {
	*db.DBClient
}

type scenarioLaunchData struct {
	Devices                      model.ScenarioDevices       `json:"devices"`
	DevicesV2                    model.ScenarioLaunchDevices `json:"launch_devices"`
	RequestedSpeakerCapabilities model.ScenarioCapabilities  `json:"requested_speaker_capabilities"`
}

type scenarioLaunch struct {
	huid       uint64
	id         string
	launchData scenarioLaunchData
}

type rawScenarioLaunchData struct {
	Devices                      json.RawMessage `json:"devices"`
	DevicesV2                    json.RawMessage `json:"launch_devices"`
	RequestedSpeakerCapabilities json.RawMessage `json:"requested_speaker_capabilities"`
}

func (db *MigrationDBClient) getUserHuidsWithScenarioLaunches(ctx context.Context) ([]uint64, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $lastHuid AS Uint64;

		SELECT DISTINCT huid FROM ScenarioLaunches WHERE huid > $lastHuid ORDER BY huid LIMIT 1000
	`, db.Prefix)

	hasMoreUids := true
	userHuids := make([]uint64, 0, 50000)
	lastHuidValue := ydb.Uint64Value(0)

	for hasMoreUids {
		params := table.NewQueryParameters(
			table.ValueParam("$lastHuid", lastHuidValue),
		)

		res, err := db.Read(ctx, query, params)
		if err != nil {
			return userHuids, err
		}

		if !res.NextSet() {
			return userHuids, nil
		}

		hasMoreUids = res.RowCount() == 1000

		var lastHuid uint64
		for res.NextRow() {
			res.NextItem()
			lastHuid = res.OUint64()
			userHuids = append(userHuids, lastHuid)
		}

		lastHuidValue = ydb.Uint64Value(lastHuid)
	}

	return userHuids, nil
}

func jsonUnmarshalLaunchData(jsonMessage []byte) (scenarioLaunchData, error) {
	var rawLaunchData rawScenarioLaunchData
	if err := json.Unmarshal(jsonMessage, &rawLaunchData); err != nil {
		return scenarioLaunchData{}, err
	}

	var scenarioDevices model.ScenarioDevices
	if err := json.Unmarshal(rawLaunchData.Devices, &scenarioDevices); err != nil {
		return scenarioLaunchData{}, err
	}

	var scenarioCapabilities model.ScenarioCapabilities
	if err := json.Unmarshal(rawLaunchData.RequestedSpeakerCapabilities, &scenarioCapabilities); err != nil {
		return scenarioLaunchData{}, err
	}

	var devicesV2 model.ScenarioLaunchDevices
	if rawLaunchData.DevicesV2 != nil {
		launchDevices, err := model.JSONUnmarshalLaunchDevices(rawLaunchData.DevicesV2)
		if err != nil {
			return scenarioLaunchData{}, err
		}
		devicesV2 = launchDevices
	}

	return scenarioLaunchData{
		Devices:                      scenarioDevices,
		DevicesV2:                    devicesV2,
		RequestedSpeakerCapabilities: scenarioCapabilities,
	}, nil
}

func (db *MigrationDBClient) getUserScenarioLaunchesForUpdate(ctx context.Context, huid uint64) ([]scenarioLaunch, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $huid AS Uint64;

		SELECT huid,id,launch_data FROM ScenarioLaunches WHERE huid=$huid;
	`, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(huid)),
	)

	launches := make([]scenarioLaunch, 0, 500)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return launches, err
	}

	if !res.NextSet() {
		return launches, nil
	}

	for res.NextRow() {
		var launch scenarioLaunch

		res.NextItem()
		launch.huid = res.OUint64()

		res.NextItem()
		launch.id = string(res.OString())

		res.NextItem()
		launch.launchData, err = jsonUnmarshalLaunchData([]byte(res.OJSON()))
		if err != nil {
			return launches, err
		}

		if len(launch.launchData.DevicesV2) > 0 {
			continue
		}

		launches = append(launches, launch)
	}

	return launches, nil
}

func (db *MigrationDBClient) getAllUserDevices(ctx context.Context, huid uint64) (model.Devices, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $huid AS Uint64;

		SELECT
			id,
			name,
			aliases,
			external_id,
			external_name,
			type,
			original_type,
			skill_id,
			capabilities,
			properties,
			custom_data,
			device_info,
			room_id,
			updated,
			created
		FROM
			Devices
		WHERE
			huid == $huid`, db.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(huid)),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return nil, err
	}

	if !res.NextSet() {
		return nil, xerrors.New("result set not found")
	}

	var devices model.Devices
	for res.NextRow() {
		var device model.Device

		res.NextItem()
		device.ID = string(res.OString())
		res.NextItem()
		device.Name = string(res.OString())

		res.NextItem()
		rawJSONAliases := res.OJSON()
		device.Aliases = make([]string, 0)
		if len(rawJSONAliases) != 0 {
			var aliases []string
			if err := json.Unmarshal([]byte(rawJSONAliases), &aliases); err != nil {
				return nil, err
			}
			device.Aliases = aliases
		}

		res.NextItem()
		device.ExternalID = string(res.OString())
		res.NextItem()
		device.ExternalName = string(res.OString())
		res.NextItem()
		device.Type = model.DeviceType(res.OString())
		res.NextItem()
		device.OriginalType = model.DeviceType(res.OString())
		res.NextItem()
		device.SkillID = string(res.OString())

		// capabilities
		res.NextItem()
		var capabilities model.Capabilities
		capabilitiesRaw := res.OJSON()
		if len(capabilitiesRaw) > 0 {
			capabilities, err = model.JSONUnmarshalCapabilities(json.RawMessage(capabilitiesRaw))
			if err != nil {
				return nil, xerrors.Errorf("failed to parse `capabilities` field for device %s: %w", device.ID, err)
			}
		}
		device.Capabilities = capabilities

		// properties
		res.NextItem()
		var properties model.Properties
		propertiesRaw := res.OJSON()
		if len(propertiesRaw) > 0 {
			properties, err = model.JSONUnmarshalProperties(json.RawMessage(propertiesRaw))
			if err != nil {
				return nil, xerrors.Errorf("failed to parse `properties` field for device %s: %w", device.ID, err)
			}
		}
		device.Properties = properties

		// custom_data
		res.NextItem()
		customDataRaw := res.OJSON()
		if len(customDataRaw) > 0 {
			var customData interface{}
			if err := json.Unmarshal([]byte(customDataRaw), &customData); err != nil {
				return nil, xerrors.Errorf("failed to parse `custom_data` field for device %s: %w", device.ID, err)
			}
			device.CustomData = customData
		} else {
			device.CustomData = nil
		}

		// device_info
		res.NextItem()
		deviceInfo := res.OJSON()
		if len(deviceInfo) > 0 {
			device.DeviceInfo = &model.DeviceInfo{}
			if err := json.Unmarshal([]byte(deviceInfo), device.DeviceInfo); err != nil {
				return nil, xerrors.Errorf("failed to parse `device_info` field for device %s: %w", device.ID, err)
			}
		}

		// room_id
		res.NextItem()
		roomID := string(res.OString())
		if roomID != "" {
			device.Room = &model.Room{ID: roomID}
		}

		// updated timestamp
		res.NextItem()
		updated := res.OTimestamp()
		if updated != 0 {
			device.Updated = timestamp.FromMicro(updated)
		}

		// created Timestamp
		res.NextItem()
		created := res.OTimestamp()
		if created != 0 {
			device.Created = timestamp.FromMicro(created)
		}

		if res.Err() != nil {
			ctxlog.Errorf(ctx, db.Logger, "failed to parse YDB response row: %v", res.Err())
			return nil, xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
		}

		devices = append(devices, device)
	}

	return devices, nil
}

func (db *MigrationDBClient) fillScenarioLaunches(ctx context.Context, huid uint64, launches []scenarioLaunch) error {
	userDevices, err := db.getAllUserDevices(ctx, huid)
	if err != nil {
		return err
	}

	for i := 0; i < len(launches); i++ {
		launches[i].launchData.DevicesV2 = launches[i].launchData.Devices.MakeScenarioLaunchDevicesByActualDevices(userDevices)
	}
	return nil
}

func (db *MigrationDBClient) updateScenarioLaunches(ctx context.Context, launches []scenarioLaunch) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $values AS List<Struct<
			huid: Uint64,
			id: String,
			launch_data: JSON
		>>;

		UPDATE
			ScenarioLaunches
		ON SELECT
			huid, id, launch_data
		FROM AS_TABLE($values);
	`, db.Prefix)

	values := make([]ydb.Value, 0, len(launches))
	for _, launch := range launches {
		dataB, err := json.Marshal(launch.launchData)
		if err != nil {
			return err
		}

		values = append(values, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(launch.huid)),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(launch.id))),
			ydb.StructFieldValue("launch_data", ydb.JSONValue(string(dataB))),
		))
	}
	params := table.NewQueryParameters(
		table.ValueParam("$values", ydb.ListValue(values...)),
	)
	if err := db.Write(ctx, query, params); err != nil {
		return xerrors.Errorf("failed to update scenario launch: %w", err)
	}

	return nil
}

func initLogging() (*zap.Logger, func()) {
	encoderConfig := uzap.NewDevelopmentEncoderConfig()
	encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
	encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
	encoder := zapcore.NewConsoleEncoder(encoderConfig)

	core := zapcore.NewCore(encoder, zapcore.AddSync(os.Stderr), uzap.DebugLevel)
	stop := func() {
		_ = core.Sync()
	}
	logger = zap.NewWithCore(core, uzap.AddStacktrace(uzap.FatalLevel), uzap.AddCaller())

	return logger, stop
}

func do(ctx context.Context, client *MigrationDBClient) error {
	launchesUpdated := 0
	usersUpdated := 0

	huids, err := client.getUserHuidsWithScenarioLaunches(ctx)
	if err != nil {
		return err
	}

	for _, huid := range huids {
		launches, err := client.getUserScenarioLaunchesForUpdate(ctx, huid)
		if err != nil {
			return err
		}

		if len(launches) == 0 {
			continue
		}

		if err := client.fillScenarioLaunches(ctx, huid, launches); err != nil {
			return err
		}
		if err := client.updateScenarioLaunches(ctx, launches); err != nil {
			return err
		}

		logger.Infof("ScenarioLaunches for huid %d updated (updated launches count: %d)\n", huid, len(launches))

		usersUpdated += 1
		launchesUpdated += len(launches)
	}

	logger.Infof("ScenarioLaunches updated\n")
	logger.Infof("Users updated count: %d\n", usersUpdated)
	logger.Infof("ScenarioLaunches updated count: %d\n", launchesUpdated)
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

	msg := fmt.Sprintf("Do you really want to update launch_data for SenarioLaunches at `%s%s`", endpoint, prefix)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx := context.Background()

	if err := do(ctx, &MigrationDBClient{dbcli}); err != nil {
		logger.Fatal(err.Error())
	}
}
