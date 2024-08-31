package main

import (
	"context"
	"fmt"
	"os"

	uzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var logger *zap.Logger

type MigrationDBClient struct {
	*db.DBClient
}

type ArchivedDevice struct {
	huid uint64
	id   string
}

func (db *MigrationDBClient) GetArchivedDevicesChunk(ctx context.Context, lastDevice *ArchivedDevice) ([]ArchivedDevice, bool, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $limit AS Uint64;
		DECLARE $lastHuid AS Uint64;
		DECLARE $lastId AS String;

		$archivedDevices = (
			SELECT huid, id FROM Devices
			WHERE archived = true AND huid = $lastHuid AND id > $lastId
			ORDER BY huid, id LIMIT $limit

			UNION ALL

			SELECT huid, id FROM Devices
			WHERE archived = true AND huid > $lastHuid
			ORDER BY huid, id LIMIT $limit
		);

		SELECT huid, id FROM $archivedDevices ORDER BY huid, id LIMIT $limit
	`, db.Prefix)

	hasMoreData := true
	var lastID string
	var lastHuid uint64
	var limit = 1000

	if lastDevice != nil {
		lastID = lastDevice.id
		lastHuid = lastDevice.huid
	}

	devices := make([]ArchivedDevice, 0, limit)

	params := table.NewQueryParameters(
		table.ValueParam("$lastHuid", ydb.Uint64Value(lastHuid)),
		table.ValueParam("$lastId", ydb.StringValue([]byte(lastID))),
		table.ValueParam("$limit", ydb.Uint64Value(uint64(limit))),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return devices, false, xerrors.Errorf("failed to get archived devices: %w", err)
	}

	if !res.NextSet() {
		return devices, false, nil
	}

	hasMoreData = res.RowCount() == limit

	for res.NextRow() {
		res.NextItem()
		lastHuid = res.OUint64()

		res.NextItem()
		lastID = string(res.OString())

		devices = append(devices, ArchivedDevice{lastHuid, lastID})
	}

	return devices, hasMoreData, nil
}

func (db *MigrationDBClient) RemoveDeviceGroups(ctx context.Context, archivedDevices []ArchivedDevice) (int, bool, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $devices AS List<Struct<
			huid: Uint64,
			id: String
		>>;

		SELECT
			dg.huid, device_id, group_id
		FROM
			DeviceGroups AS dg JOIN
			(SELECT * FROM AS_TABLE($devices)) AS d ON
			dg.huid = d.huid AND dg.device_id = d.id;
	`, db.Prefix)

	selectSetParams := make([]ydb.Value, 0, len(archivedDevices))
	for _, device := range archivedDevices {
		selectSetParams = append(selectSetParams, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(device.huid)),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(device.id))),
		))
	}

	params := table.NewQueryParameters(
		table.ValueParam("$devices", ydb.ListValue(selectSetParams...)),
	)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return 0, false, err
	}

	if !res.NextSet() {
		return 0, false, nil
	}

	if res.RowCount() == 0 {
		return 0, false, nil
	}

	deleteSetParams := make([]ydb.Value, 0, res.RowCount())

	for res.NextRow() {
		res.NextItem()
		huid := res.OUint64()

		res.NextItem()
		deviceID := string(res.OString())

		res.NextItem()
		groupID := string(res.OString())

		logger.Infof("DeviceGroups entry for delete (huid: %d, device_id: %s, group_id: %s)", huid, deviceID, groupID)

		deleteSetParams = append(deleteSetParams, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(huid)),
			ydb.StructFieldValue("device_id", ydb.StringValue([]byte(deviceID))),
			ydb.StructFieldValue("group_id", ydb.StringValue([]byte(groupID))),
		))
	}

	query = fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $device_groups AS List<Struct<
			huid: Uint64,
			device_id: String,
			group_id: String
		>>;

		DELETE FROM DeviceGroups ON SELECT huid, device_id, group_id FROM AS_TABLE($device_groups);
	`, db.Prefix)

	params = table.NewQueryParameters(
		table.ValueParam("$device_groups", ydb.ListValue(deleteSetParams...)),
	)

	if err := db.Write(ctx, query, params); err != nil {
		return 0, false, err
	}

	return res.RowCount(), res.RowCount() == 1000, nil
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
	var lastDevice *ArchivedDevice
	hasMoreData := true
	processedDeviceCount := 0
	removedDeviceGroupsCount := 0

	for hasMoreData {
		chunk, moreData, err := client.GetArchivedDevicesChunk(ctx, lastDevice)
		if err != nil {
			return err
		}
		hasMoreData = moreData
		if hasMoreData {
			lastDevice = &chunk[len(chunk)-1]
		}

		hasMoreToRemove := true
		for hasMoreToRemove {
			removedCount, hasMore, err := client.RemoveDeviceGroups(ctx, chunk)
			if err != nil {
				return err
			}
			removedDeviceGroupsCount += removedCount
			hasMoreToRemove = hasMore
		}

		processedDeviceCount += len(chunk)
	}

	logger.Infof("Remove device groups for archived devices done\n")
	logger.Infof("Devices processed count: %d\n", processedDeviceCount)
	logger.Infof("DeviceGroups removed count: %d\n", removedDeviceGroupsCount)
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

	msg := fmt.Sprintf("Do you really want to clean DeviceGroups for archived devices at `%s%s`", endpoint, prefix)
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
