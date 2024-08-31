package main

import (
	"context"
	"fmt"
	"os"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/zap"
	uzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var logger *zap.Logger
var tableName string

type MigrationDBClient struct {
	*db.DBClient
}

type dbObject struct {
	huid uint64
	id   string
}

func (db *MigrationDBClient) getObjectsChunk(ctx context.Context, lastDBObject *dbObject) ([]dbObject, bool, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $limit AS Uint64;
		DECLARE $lastHuid AS Uint64;
		DECLARE $lastId AS String;

		$part1 = (
			SELECT huid, id FROM %s
			WHERE huid = $lastHuid AND id > $lastId AND household_id is NULL
			ORDER BY huid, id LIMIT $limit
		);

		$part2 = (
			SELECT huid, id FROM %s
			WHERE huid > $lastHuid and household_id is NULL
			ORDER BY huid, id LIMIT $limit
		);

		$union = (
			SELECT * FROM $part1
			UNION ALL
			SELECT * FROM $part2
		);

		SELECT huid, id FROM $union ORDER BY huid, id LIMIT $limit
	`, db.Prefix, tableName, tableName)

	var lastDBObjectHuid uint64
	var lastDBObjectID string

	if lastDBObject != nil {
		lastDBObjectHuid = lastDBObject.huid
		lastDBObjectID = lastDBObject.id
	}

	limit := 1000
	params := table.NewQueryParameters(
		table.ValueParam("$limit", ydb.Uint64Value(uint64(limit))),
		table.ValueParam("$lastHuid", ydb.Uint64Value(lastDBObjectHuid)),
		table.ValueParam("$lastId", ydb.StringValue([]byte(lastDBObjectID))),
	)

	dbObjects := make([]dbObject, 0)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return dbObjects, false, err
	}

	if !res.NextSet() {
		return dbObjects, false, nil
	}

	for res.NextRow() {
		var o dbObject

		res.NextItem()
		o.huid = res.OUint64()

		res.NextItem()
		o.id = string(res.OString())

		dbObjects = append(dbObjects, o)
	}

	hasMoreData := res.RowCount() == limit
	return dbObjects, hasMoreData, nil
}

func (db *MigrationDBClient) deleteHomeless(ctx context.Context, dbObjects []dbObject) error {
	if len(dbObjects) == 0 {
		return nil
	}

	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $objects AS List<Struct<
			huid: Uint64,
			id: String
		>>;

		DELETE FROM
			%s
		ON SELECT
			huid, id
		FROM AS_TABLE($objects);
	`, db.Prefix, tableName)

	deleteParams := make([]ydb.Value, 0, len(dbObjects))
	for _, o := range dbObjects {
		logger.Infof("Prepare to delete object with values (huid: %d, id: %s)", o.huid, o.id)

		deleteParams = append(deleteParams, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(o.huid)),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(o.id))),
		))
	}

	params := table.NewQueryParameters(
		table.ValueParam("$objects", ydb.ListValue(deleteParams...)),
	)

	return db.Write(ctx, query, params)
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
	deletedCount := 0
	var lastDBObject *dbObject

	for hasMoreData {
		chunk, moreData, err := client.getObjectsChunk(ctx, lastDBObject)
		if err != nil {
			return err
		}
		hasMoreData = moreData
		if len(chunk) > 0 {
			lastDBObject = &chunk[len(chunk)-1]
		}
		if err := client.deleteHomeless(ctx, chunk); err != nil {
			return err
		}
		deletedCount += len(chunk)
	}

	logger.Infof("%s cleanup done\n", tableName)
	logger.Infof("%s deleted count: %d\n", tableName, deletedCount)
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

	tableName = os.Getenv("MIGRATION_TABLE")
	if tableName == "" {
		panic("MIGRATION_TABLE env is not set")
	}

	dbcli, err := db.NewClient(context.Background(), logger, endpoint, prefix, ydb.AuthTokenCredentials{AuthToken: token}, trace)
	if err != nil {
		panic(err.Error())
	}

	msg := fmt.Sprintf("Do you really want to cleanup %s with household_id null at `%s%s`", tableName, endpoint, prefix)
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
