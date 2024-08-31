package main

import (
	"context"
	"fmt"
	"os"
	"sort"

	uzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
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

type Device struct {
	huid    uint64
	id      string
	created timestamp.PastTimestamp
}

type Devices []Device

func (d Devices) Len() int {
	return len(d)
}

func (d Devices) Less(i, j int) bool {
	return d[i].created < d[j].created
}

func (d Devices) Swap(i, j int) {
	d[i], d[j] = d[j], d[i]
}

func (db *MigrationDBClient) GetDuplicateExternalIDsChunk(ctx context.Context) ([]string, bool, error) {
	duplicateStationsQuery := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $limit AS Uint64;

		SELECT
			external_id,
			count(*)
		FROM
			Devices
		WHERE
			skill_id="Q" AND
			type LIKE "devices.types.smart_speaker%%" AND
			archived=false
		GROUP BY
			external_id
		HAVING
			count(*) > 1
		LIMIT $limit
	`, db.Prefix)

	limit := 100
	params := table.NewQueryParameters(
		table.ValueParam("$limit", ydb.Uint64Value(uint64(limit))),
	)

	res, err := db.Read(ctx, duplicateStationsQuery, params)
	if err != nil {
		return []string{}, false, xerrors.Errorf("failed to find duplicate external_ids: %w", err)
	}

	if !res.NextSet() {
		return []string{}, false, nil
	}

	rowCount := res.RowCount()
	hasMoreData := rowCount == limit

	externalIDs := make([]string, 0, rowCount)
	for res.NextRow() {
		res.NextItem()
		externalID := res.OString()
		externalIDs = append(externalIDs, string(externalID))
	}

	return externalIDs, hasMoreData, nil
}

func (db *MigrationDBClient) GetDevicesByExternalIDs(ctx context.Context, externalIDs []string) (map[string]Devices, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $external_ids AS List<String>;

		SELECT
			huid,
			id,
			created,
			external_id
		FROM
			Devices VIEW devices_external_id_index
		WHERE
			external_id IN $external_ids AND
			archived=false
	`, db.Prefix)

	externalIDValues := make([]ydb.Value, 0, len(externalIDs))
	for _, id := range externalIDs {
		externalIDValues = append(externalIDValues, ydb.StringValue([]byte(id)))
	}
	params := table.NewQueryParameters(
		table.ValueParam("$external_ids", ydb.ListValue(externalIDValues...)),
	)

	devicesByExternalID := make(map[string]Devices)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return devicesByExternalID, xerrors.Errorf("failed to find devices: %w", err)
	}

	if !res.NextSet() {
		return devicesByExternalID, nil
	}

	for res.NextRow() {
		var d Device

		res.NextItem()
		d.huid = res.OUint64()

		res.NextItem()
		d.id = string(res.OString())

		res.NextItem()
		d.created = timestamp.FromMicro(res.OTimestamp())

		res.NextItem()
		externalID := string(res.OString())

		devicesByExternalID[externalID] = append(devicesByExternalID[externalID], d)
	}

	return devicesByExternalID, nil
}

type deviceDeleteChange struct {
	ID         string
	Huid       uint64
	Archived   bool
	ArchivedAt timestamp.PastTimestamp
}

func (d *deviceDeleteChange) toStructValue() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("id", ydb.StringValue([]byte(d.ID))),
		ydb.StructFieldValue("huid", ydb.Uint64Value(d.Huid)),
		ydb.StructFieldValue("archived", ydb.BoolValue(d.Archived)),
		ydb.StructFieldValue("archived_at", ydb.TimestampValue(d.ArchivedAt.YdbTimestamp())),
	)
}

func (db *MigrationDBClient) GetDevicesToArchive(duplicatedDevices map[string]Devices) []Device {
	result := make([]Device, 0)
	for _, devices := range duplicatedDevices {
		sort.Sort(devices)
		result = append(result, devices[0:len(devices)-1]...)
	}
	return result
}

func (db *MigrationDBClient) ArchiveDevices(ctx context.Context, devices []Device) error {
	timestamper := timestamp.NewTimestamper()
	changeSet := make([]deviceDeleteChange, 0, len(devices))

	ctxlog.Infof(ctx, db.Logger, "Archive chunk with %d devices", len(devices))
	for _, d := range devices {
		changeSet = append(changeSet, deviceDeleteChange{ID: d.id, Huid: d.huid, Archived: true, ArchivedAt: timestamper.CurrentTimestamp()})
		ctxlog.Infof(ctx, db.Logger, "Archive device with huid=`%d` and id=`%s`", d.huid, d.id)
	}

	if len(changeSet) == 0 {
		ctxlog.Warn(ctx, db.Logger, "Delete changeset is empty")
		return nil
	}

	updateQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $changeset AS List<Struct<
				id: String,
				huid: Uint64,
				archived: Bool,
				archived_at: Timestamp
			>>;

			UPDATE Devices ON
			SELECT * FROM AS_TABLE($changeset);`, db.Prefix)

	queryChangeSetParams := make([]ydb.Value, 0, len(changeSet))
	for _, change := range changeSet {
		queryChangeSetParams = append(queryChangeSetParams, change.toStructValue())
	}

	updateParams := table.NewQueryParameters(
		table.ValueParam("$changeset", ydb.ListValue(queryChangeSetParams...)),
	)

	err := db.Write(ctx, updateQuery, updateParams)
	if err != nil {
		return err
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
	hasMoreData := true

	archivedDevicesCount := 0
	totalDuplicatedDevicesCount := 0

	for hasMoreData {
		chunk, moreData, err := client.GetDuplicateExternalIDsChunk(ctx)
		if err != nil {
			return err
		}

		devicesByExternalIDs, err := client.GetDevicesByExternalIDs(ctx, chunk)
		if err != nil {
			return err
		}

		for _, devices := range devicesByExternalIDs {
			totalDuplicatedDevicesCount += len(devices)
		}

		devicesToArchive := client.GetDevicesToArchive(devicesByExternalIDs)
		archivedDevicesCount += len(devicesToArchive)

		err = client.ArchiveDevices(ctx, devicesToArchive)
		if err != nil {
			return err
		}

		hasMoreData = moreData
	}

	logger.Infof("Device archive done\n")
	logger.Infof("Archived devices count: %d\n", archivedDevicesCount)
	logger.Infof("Total duplicated devices count: %d\n", totalDuplicatedDevicesCount)
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

	msg := fmt.Sprintf("Do you really want to archive stations duplicated by external id at `%s%s`", endpoint, prefix)
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
